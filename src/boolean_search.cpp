#include "boolean_search.hpp"
#include <chrono>
#include <algorithm>
#include <iostream>
#include <cctype>
#include <sstream>

BooleanSearch::BooleanSearch(const BooleanIndexBuilder& index) : index(index) {
    init_all_documents();
}

void BooleanSearch::init_all_documents() {
    const auto& forward_index = index.get_forward_index();
    all_documents.reserve(forward_index.size());

    for (uint32_t i = 0; i < forward_index.size(); ++i) {
        all_documents.push_back(i);
    }
}

std::vector<uint32_t> BooleanSearch::search(const std::string& query) {
    auto start_time = std::chrono::high_resolution_clock::now();

    try {
        // Токенизация запроса
        auto tokens = tokenize_query(query);

        size_t pos = 0;
        auto result = parse_expression(tokens, pos);

        auto end_time = std::chrono::high_resolution_clock::now();

        // Сохраняем статистику
        last_stats.query = query;
        last_stats.result_count = result.size();
        last_stats.processing_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();
        last_stats.terms_processed = tokens.size();

        return result;

    } catch (const std::exception& e) {
        std::cerr << "Search error: " << e.what() << std::endl;
        return {};
    }
}

std::vector<BooleanSearch::QueryToken> BooleanSearch::tokenize_query(const std::string& query) {
    std::vector<QueryToken> tokens;
    std::string current_term;

    for (size_t i = 0; i < query.length(); ++i) {
        char c = query[i];

        if (std::isspace(c)) {
            if (!current_term.empty()) {
                tokens.emplace_back(TokenType::TERM, current_term);
                current_term.clear();
            }
            continue;
        }

        // Проверяем специальные символы
        if (c == '(') {
            if (!current_term.empty()) {
                tokens.emplace_back(TokenType::TERM, current_term);
                current_term.clear();
            }
            tokens.emplace_back(TokenType::LPAREN);
        } else if (c == ')') {
            if (!current_term.empty()) {
                tokens.emplace_back(TokenType::TERM, current_term);
                current_term.clear();
            }
            tokens.emplace_back(TokenType::RPAREN);
        } else if (c == '!' || c == '-') {
            if (!current_term.empty()) {
                tokens.emplace_back(TokenType::TERM, current_term);
                current_term.clear();
            }
            tokens.emplace_back(TokenType::NOT);
        } else if (c == '&' && i + 1 < query.length() && query[i + 1] == '&') {
            if (!current_term.empty()) {
                tokens.emplace_back(TokenType::TERM, current_term);
                current_term.clear();
            }
            tokens.emplace_back(TokenType::AND);
            i++;  // Пропускаем второй символ
        } else if (c == '|' && i + 1 < query.length() && query[i + 1] == '|') {
            if (!current_term.empty()) {
                tokens.emplace_back(TokenType::TERM, current_term);
                current_term.clear();
            }
            tokens.emplace_back(TokenType::OR);
            i++;  // Пропускаем второй символ
        } else {
            // Часть термина
            current_term += c;
        }
    }

    // Добавляем последний термин, если есть
    if (!current_term.empty()) {
        tokens.emplace_back(TokenType::TERM, current_term);
    }

    // Добавляем END токен
    tokens.emplace_back(TokenType::END);

    return tokens;
}

std::vector<uint32_t> BooleanSearch::parse_expression(const std::vector<QueryToken>& tokens,
                                                      size_t& pos) {
    auto left = parse_term(tokens, pos);

    while (pos < tokens.size()) {
        auto token = tokens[pos];

        if (token.type == TokenType::OR) {
            pos++;
            auto right = parse_term(tokens, pos);
            left = union_sets(left, right);
        } else {
            break;
        }
    }

    return left;
}

std::vector<uint32_t> BooleanSearch::parse_term(const std::vector<QueryToken>& tokens,
                                                size_t& pos) {
    auto left = parse_factor(tokens, pos);

    while (pos < tokens.size()) {
        auto token = tokens[pos];

        if (token.type == TokenType::AND || token.type == TokenType::TERM) {
            // Неявное AND (пробел между терминами)
            if (token.type == TokenType::AND) {
                pos++;
            }

            auto right = parse_factor(tokens, pos);
            left = intersect_sets(left, right);
        } else {
            break;
        }
    }

    return left;
}

std::vector<uint32_t> BooleanSearch::parse_factor(const std::vector<QueryToken>& tokens,
                                                  size_t& pos) {
    if (pos >= tokens.size()) {
        throw std::runtime_error("Unexpected end of query");
    }

    auto token = tokens[pos];

    if (token.type == TokenType::NOT) {
        pos++;
        auto operand = parse_factor(tokens, pos);
        return complement_set(operand);
    } else if (token.type == TokenType::LPAREN) {
        pos++;
        auto result = parse_expression(tokens, pos);

        if (pos >= tokens.size() || tokens[pos].type != TokenType::RPAREN) {
            throw std::runtime_error("Missing closing parenthesis");
        }

        pos++;
        return result;
    } else if (token.type == TokenType::TERM) {
        pos++;
        return get_postings(normalize_term(token.value));
    } else {
        throw std::runtime_error("Unexpected token in query");
    }
}

std::vector<uint32_t> BooleanSearch::intersect_sets(const std::vector<uint32_t>& a,
                                                    const std::vector<uint32_t>& b) {
    std::vector<uint32_t> result;
    result.reserve(std::min(a.size(), b.size()));

    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] == b[j]) {
            result.push_back(a[i]);
            i++;
            j++;
        } else if (a[i] < b[j]) {
            i++;
        } else {
            j++;
        }
    }

    return result;
}

std::vector<uint32_t> BooleanSearch::union_sets(const std::vector<uint32_t>& a,
                                                const std::vector<uint32_t>& b) {
    std::vector<uint32_t> result;
    result.reserve(a.size() + b.size());

    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] == b[j]) {
            result.push_back(a[i]);
            i++;
            j++;
        } else if (a[i] < b[j]) {
            result.push_back(a[i]);
            i++;
        } else {
            result.push_back(b[j]);
            j++;
        }
    }

    while (i < a.size()) {
        result.push_back(a[i]);
        i++;
    }

    while (j < b.size()) {
        result.push_back(b[j]);
        j++;
    }

    return result;
}

std::vector<uint32_t> BooleanSearch::complement_set(const std::vector<uint32_t>& a) {
    std::vector<uint32_t> result;
    result.reserve(all_documents.size() - a.size());

    size_t i = 0, j = 0;
    while (i < all_documents.size() && j < a.size()) {
        if (all_documents[i] == a[j]) {
            i++;
            j++;
        } else if (all_documents[i] < a[j]) {
            result.push_back(all_documents[i]);
            i++;
        } else {
            j++;
        }
    }

    while (i < all_documents.size()) {
        result.push_back(all_documents[i]);
        i++;
    }

    return result;
}

std::string BooleanSearch::normalize_term(const std::string& term) {
    std::string normalized = term;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return normalized;
}

std::vector<uint32_t> BooleanSearch::get_postings(const std::string& term) {
    const auto& inverted_index = index.get_inverted_index();
    auto it = inverted_index.find(term);

    if (it != inverted_index.end()) {
        return it->second;
    }

    return {};  // Термин не найден
}

std::vector<BooleanSearch::SearchResult> BooleanSearch::format_results(
    const std::vector<uint32_t>& doc_ids, size_t offset, size_t limit) const {

    std::vector<SearchResult> results;
    const auto& forward_index = index.get_forward_index();

    size_t end = std::min(offset + limit, doc_ids.size());

    for (size_t i = offset; i < end; ++i) {
        uint32_t doc_id = doc_ids[i];

        if (doc_id >= forward_index.size()) {
            continue;
        }

        const auto& doc_info = forward_index[doc_id];

        SearchResult result;
        result.doc_id = doc_id;
        result.title = doc_info.title.empty() ? "Untitled Document" : doc_info.title;
        result.url = doc_info.url;

        // Простой расчет релевантности (можно улучшить)
        result.relevance = 1.0 / (i + 1);

        results.push_back(result);
    }

    return results;
}

std::vector<std::pair<std::string, std::vector<uint32_t>>> BooleanSearch::batch_search(
    const std::vector<std::string>& queries) {

    std::vector<std::pair<std::string, std::vector<uint32_t>>> results;
    results.reserve(queries.size());

    for (const auto& query : queries) {
        auto search_result = search(query);
        results.emplace_back(query, std::move(search_result));
    }

    return results;
}

BooleanSearch::SearchStats BooleanSearch::get_last_stats() const {
    return last_stats;
}