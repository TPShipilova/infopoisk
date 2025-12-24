#include "boolean_index.hpp"
#include <chrono>
#include <algorithm>
#include <iostream>
#include <cstring>

BooleanIndexBuilder::BooleanIndexBuilder() {
    // Инициализация
}

void BooleanIndexBuilder::build_from_documents(const std::vector<Document>& documents) {
    auto start_time = std::chrono::high_resolution_clock::now();

    // Очищаем существующие данные
    forward_index.clear();
    inverted_index.clear();

    // Резервируем память
    forward_index.reserve(documents.size());

    stats.total_documents = documents.size();
    stats.total_terms = 0;
    stats.total_postings = 0;
    stats.avg_term_length = 0.0;
    stats.avg_doc_length = 0.0;

    // Обрабатываем каждый документ
    for (size_t i = 0; i < documents.size(); ++i) {
        process_document(documents[i], static_cast<uint32_t>(i));
    }

    // Сортируем постинги для каждого термина
    sort_and_unique_postings();

    // Собираем статистику
    size_t total_term_chars = 0;
    size_t total_doc_terms = 0;

    for (const auto& [term, postings] : inverted_index) {
        total_term_chars += term.length();
        stats.total_postings += postings.size();
    }

    for (const auto& doc : forward_index) {
        total_doc_terms += doc.doc_length;
    }

    if (!inverted_index.empty()) {
        stats.avg_term_length = static_cast<double>(total_term_chars) / inverted_index.size();
    }

    if (!forward_index.empty()) {
        stats.avg_doc_length = static_cast<double>(total_doc_terms) / forward_index.size();
    }

    stats.total_terms = inverted_index.size();

    auto end_time = std::chrono::high_resolution_clock::now();
    stats.indexing_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count();

    std::cout << "Index built: " << stats.total_documents << " documents, "
              << stats.total_terms << " unique terms, "
              << stats.total_postings << " total postings" << std::endl;
}

void BooleanIndexBuilder::process_document(const Document& doc, uint32_t doc_id) {
    // Создаем запись в прямом индексе
    ForwardIndexEntry forward_entry;
    forward_entry.id = doc.id;
    forward_entry.url = doc.url;
    forward_entry.title = doc.title;
    forward_entry.doc_length = 0;
    forward_entry.checksum = 0;  // Можно вычислить контрольную сумму

    // Токенизируем и нормализуем контент
    auto tokenization_result = tokenizer.tokenize(doc.content);

    // Для каждого токена
    std::unordered_map<std::string, uint32_t> term_frequencies;

    for (const auto& token : tokenization_result.tokens) {
        std::string term = normalize_term(token);

        if (term.length() < 2 || term.length() > 50) {
            continue;
        }

        term_frequencies[term]++;
    }

    // Добавляем термины в обратный индекс
    forward_entry.doc_length = static_cast<uint32_t>(term_frequencies.size());

    for (const auto& [term, freq] : term_frequencies) {
        inverted_index[term].push_back(doc_id);
    }

    forward_index.push_back(forward_entry);
}

std::string BooleanIndexBuilder::normalize_term(const std::string& term) {
    // Приводим к нижнему регистру
    std::string normalized = term;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Применяем стемминг
    return stemmer.stem(normalized);
}

void BooleanIndexBuilder::sort_and_unique_postings() {
    for (auto& [term, postings] : inverted_index) {
        // Сортируем и удаляем дубликаты
        std::sort(postings.begin(), postings.end());
        auto last = std::unique(postings.begin(), postings.end());
        postings.erase(last, postings.end());
    }
}

void BooleanIndexBuilder::save_index(const std::string& filename) {
    std::cout << "Saving index to " << filename << "..." << std::endl;

    BinaryIndexWriter writer(filename);

    // Записываем заголовок
    writer.write_header(static_cast<uint32_t>(forward_index.size()),
                       static_cast<uint32_t>(inverted_index.size()));

    // Подготавливаем прямые записи
    std::vector<ForwardIndexEntry> forward_entries;
    for (size_t i = 0; i < forward_index.size(); ++i) {
        forward_entries.push_back(forward_index[i]);
        // Можно вычислить checksum
        forward_entries.back().checksum = i;  // Временное значение
    }

    // Записываем прямой индекс
    writer.write_forward_index(forward_entries);

    // Подготавливаем обратный индекс для записи
    std::vector<std::pair<std::string, std::vector<uint32_t>>> inverted_entries;
    inverted_entries.reserve(inverted_index.size());

    for (const auto& [term, postings] : inverted_index) {
        inverted_entries.emplace_back(term, postings);
    }

    // Записываем обратный индекс
    writer.write_inverted_index(inverted_entries);

    std::cout << "Index saved successfully." << std::endl;
}

bool BooleanIndexBuilder::load_index(const std::string& filename) {
    std::cout << "Loading index from " << filename << "..." << std::endl;

    try {
        BinaryIndexReader reader(filename);

        uint32_t doc_count, term_count;
        if (!reader.read_header(doc_count, term_count)) {
            return false;
        }

        // Читаем прямой индекс
        forward_index = reader.read_forward_index();

        // Читаем обратный индекс
        auto inverted_entries = reader.read_inverted_index();

        // Преобразуем в unordered_map
        inverted_index.clear();
        inverted_index.reserve(inverted_entries.size());

        for (auto& entry : inverted_entries) {
            inverted_index[entry.first] = std::move(entry.second);
        }

        // Обновляем статистику
        stats.total_documents = forward_index.size();
        stats.total_terms = inverted_index.size();
        stats.total_postings = 0;

        for (const auto& [term, postings] : inverted_index) {
            stats.total_postings += postings.size();
        }

        std::cout << "Index loaded: " << stats.total_documents << " documents, "
                  << stats.total_terms << " unique terms" << std::endl;

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error loading index: " << e.what() << std::endl;
        return false;
    }
}

BooleanIndexBuilder::Statistics BooleanIndexBuilder::get_statistics() const {
    return stats;
}

const std::vector<ForwardIndexEntry>& BooleanIndexBuilder::get_forward_index() const {
    return forward_index;
}

const std::unordered_map<std::string, std::vector<uint32_t>>& BooleanIndexBuilder::get_inverted_index() const {
    return inverted_index;
}

std::vector<std::pair<std::string, std::vector<uint32_t>>> BooleanIndexBuilder::get_sorted_entries() const {
    std::vector<std::pair<std::string, std::vector<uint32_t>>> entries;
    entries.reserve(inverted_index.size());

    for (const auto& [term, postings] : inverted_index) {
        entries.emplace_back(term, postings);
    }

    std::sort(entries.begin(), entries.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    return entries;
}