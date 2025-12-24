//#include <iostream>
//#include <vector>
//#include <string>
//#include <chrono>
//#include <fstream>
//#include <iomanip>
//#include "mongo_connector.cpp"
//#include "tokenizer.hpp"
//#include "zipf_analyzer.hpp"
//#include "stemmer.cpp"
//#include "hash_table.hpp"
//#include "binary_search_tree.hpp"
//
//void run_tokenization_analysis(MongoConnector& mongo, int limit = -1) {
//    std::cout << "\n=== ТОКЕНИЗАЦИЯ ===\n" << std::endl;
//
//    auto documents = mongo.fetch_documents(limit);
//
//    if (documents.empty()) {
//        std::cout << "Нет документов для обработки" << std::endl;
//        return;
//    }
//
//    Tokenizer tokenizer;
//    std::vector<std::string> all_tokens;
//    size_t total_chars = 0;
//    auto total_start = std::chrono::high_resolution_clock::now();
//
//    std::ofstream stats_file("tokenization_stats.csv");
//    stats_file << "doc_id,doc_size_chars,doc_size_kb,num_tokens,avg_token_length,time_ms\n";
//
//    int doc_count = 0;
//    for (const auto& doc : documents) {
//        auto start = std::chrono::high_resolution_clock::now();
//
//        auto result = tokenizer.tokenize(doc.content);
//
//        auto end = std::chrono::high_resolution_clock::now();
//        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//
//        // Собираем статистику по документу
//        double size_kb = doc.content.size() / 1024.0;
//        double avg_length = result.tokens.empty() ? 0 :
//                          static_cast<double>(result.total_chars) / result.tokens.size();
//
//        stats_file << doc_count << ","
//                   << doc.content.size() << ","
//                   << size_kb << ","
//                   << result.tokens.size() << ","
//                   << avg_length << ","
//                   << duration.count() << "\n";
//
//        // Собираем все токены для дальнейшего анализа
//        all_tokens.insert(all_tokens.end(), result.tokens.begin(), result.tokens.end());
//        total_chars += result.total_chars;
//
//        doc_count++;
//
//        if (doc_count % 10 == 0) {
//            std::cout << "Обработано документов: " << doc_count << std::endl;
//        }
//    }
//
//    auto total_end = std::chrono::high_resolution_clock::now();
//    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(total_end - total_start);
//
//    // Общая статистика
//    double total_size_kb = total_chars / 1024.0;
//    double avg_token_length = all_tokens.empty() ? 0 :
//                            static_cast<double>(total_chars) / all_tokens.size();
//    double tokens_per_kb = total_size_kb > 0 ? all_tokens.size() / total_size_kb : 0;
//    double ms_per_kb = total_size_kb > 0 ? total_duration.count() / total_size_kb : 0;
//
//    std::cout << "\n=== РЕЗУЛЬТАТЫ ТОКЕНИЗАЦИИ ===\n";
//    std::cout << "Всего документов: " << documents.size() << std::endl;
//    std::cout << "Всего токенов: " << all_tokens.size() << std::endl;
//    std::cout << "Уникальных токенов: " << std::set<std::string>(all_tokens.begin(), all_tokens.end()).size() << std::endl;
//    std::cout << "Средняя длина токена: " << avg_token_length << " символов" << std::endl;
//    std::cout << "Общее время обработки: " << total_duration.count() << " мс" << std::endl;
//    std::cout << "Общий объем текста: " << total_size_kb << " КБ" << std::endl;
//    std::cout << "Скорость обработки: " << tokens_per_kb << " токенов/КБ" << std::endl;
//    std::cout << "Время на КБ: " << ms_per_kb << " мс/КБ" << std::endl;
//
//    stats_file.close();
//
//    // Анализ закона Ципфа
//    std::cout << "\n=== АНАЛИЗ ЗАКОНА ЦИПФА ===\n";
//
//    ZipfAnalyzer zipf_analyzer;
//    auto zipf_result = zipf_analyzer.analyze(all_tokens);
//
//    std::cout << "Всего токенов: " << zipf_result.total_tokens << std::endl;
//    std::cout << "Уникальных токенов: " << zipf_result.unique_tokens << std::endl;
//    std::cout << "Константа Ципфа (C): " << zipf_result.zipf_constant << std::endl;
//
//    zipf_analyzer.save_to_csv(zipf_result, "zipf_analysis.csv");
//
//    // Стемминг
//    std::cout << "\n=== СТЕММИНГ ===\n";
//
//    Stemmer stemmer;
//    std::vector<std::string> stemmed_tokens;
//    stemmed_tokens.reserve(all_tokens.size());
//
//    auto stem_start = std::chrono::high_resolution_clock::now();
//
//    for (const auto& token : all_tokens) {
//        stemmed_tokens.push_back(stemmer.stem(token));
//    }
//
//    auto stem_end = std::chrono::high_resolution_clock::now();
//    auto stem_duration = std::chrono::duration_cast<std::chrono::milliseconds>(stem_end - stem_start);
//
//    // Анализ после стемминга
//    HashTable<std::string, int> original_counts(10007);
//    HashTable<std::string, int> stemmed_counts(10007);
//
//    for (const auto& token : all_tokens) {
//        int count = 0;
//        if (original_counts.get(token, count)) {
//            original_counts.insert(token, count + 1);
//        } else {
//            original_counts.insert(token, 1);
//        }
//    }
//
//    for (const auto& token : stemmed_tokens) {
//        int count = 0;
//        if (stemmed_counts.get(token, count)) {
//            stemmed_counts.insert(token, count + 1);
//        } else {
//            stemmed_counts.insert(token, 1);
//        }
//    }
//
//    std::cout << "Уникальных токенов до стемминга: " << original_counts.get_count() << std::endl;
//    std::cout << "Уникальных токенов после стемминга: " << stemmed_counts.get_count() << std::endl;
//    std::cout << "Сокращение словаря: "
//              << (1.0 - static_cast<double>(stemmed_counts.get_count()) / original_counts.get_count()) * 100
//              << "%" << std::endl;
//    std::cout << "Время стемминга: " << stem_duration.count() << " мс" << std::endl;
//
//    // Примеры стемминга
//    std::cout << "\nПримеры стемминга:\n";
//    std::vector<std::string> examples = {"fashion", "fashions", "fashionable",
//                                         "design", "designs", "designer",
//                                         "clothing", "clothes", "cloth"};
//
//    for (const auto& word : examples) {
//        std::cout << word << " -> " << stemmer.stem(word) << std::endl;
//    }
//
//    // Сохраняем результаты стемминга
//    std::ofstream stem_file("stemming_results.csv");
//    stem_file << "original,stemmed,original_count,stemmed_count\n";
//
//    auto original_pairs = original_counts.get_all();
//    for (const auto& [word, count] : original_pairs) {
//        std::string stemmed = stemmer.stem(word);
//        int stemmed_count = 0;
//        stemmed_counts.get(stemmed, stemmed_count);
//
//        stem_file << word << "," << stemmed << "," << count << "," << stemmed_count << "\n";
//    }
//
//    stem_file.close();
//}
//
//int main() {
//    try {
//        // Подключение к MongoDB
//        std::cout << "Подключение к MongoDB..." << std::endl;
//        MongoConnector mongo("mongodb://localhost:27017", "fashion-corpus");
//
//        // Запуск анализа
//        int limit = 1000; // Ограничение количества документов для теста
//        run_tokenization_analysis(mongo, limit);
//
//        // Тестирование поиска с использованием наших структур данных
//        std::cout << "\n=== ТЕСТИРОВАНИЕ ПОИСКА ===\n";
//
//        // Создаем индекс на основе бинарного дерева поиска
//        BinarySearchTree<std::string, std::vector<int>> search_index;
//
//        auto documents = mongo.fetch_documents(100);
//
//        // Простая индексация
//        for (size_t i = 0; i < documents.size(); ++i) {
//            Tokenizer tokenizer;
//            auto tokens = tokenizer.tokenize(documents[i].content);
//
//            for (const auto& token : tokens.tokens) {
//                std::vector<int> doc_ids;
//                if (search_index.get(token, doc_ids)) {
//                    doc_ids.push_back(i);
//                    search_index.insert(token, doc_ids);
//                } else {
//                    search_index.insert(token, {static_cast<int>(i)});
//                }
//            }
//        }
//
//        // Тестовый поиск
//        std::vector<std::string> test_queries = {
//            "fashion", "design", "clothing", "trend"
//        };
//
//        for (const auto& query : test_queries) {
//            std::vector<int> results;
//            if (search_index.get(query, results)) {
//                std::cout << "Запрос '" << query << "': найдено "
//                         << results.size() << " документов" << std::endl;
//            } else {
//                std::cout << "Запрос '" << query << "': не найдено" << std::endl;
//            }
//        }
//
//        // Тестирование поиска со стеммингом
//        std::cout << "\n=== ПОИСК СО СТЕММИНГОМ ===\n";
//
//        Stemmer stemmer;
//        BinarySearchTree<std::string, std::vector<int>> stemmed_index;
//
//        for (size_t i = 0; i < documents.size(); ++i) {
//            Tokenizer tokenizer;
//            auto tokens = tokenizer.tokenize(documents[i].content);
//
//            for (const auto& token : tokens.tokens) {
//                std::string stemmed = stemmer.stem(token);
//                std::vector<int> doc_ids;
//                if (stemmed_index.get(stemmed, doc_ids)) {
//                    doc_ids.push_back(i);
//                    stemmed_index.insert(stemmed, doc_ids);
//                } else {
//                    stemmed_index.insert(stemmed, {static_cast<int>(i)});
//                }
//            }
//        }
//
//        // Поиск со стеммингом
//        for (const auto& query : test_queries) {
//            std::string stemmed_query = stemmer.stem(query);
//            std::vector<int> results;
//            if (stemmed_index.get(stemmed_query, results)) {
//                std::cout << "Запрос '" << query << "' (stem: " << stemmed_query
//                         << "): найдено " << results.size() << " документов" << std::endl;
//            } else {
//                std::cout << "Запрос '" << query << "' (stem: " << stemmed_query
//                         << "): не найдено" << std::endl;
//            }
//        }
//
//    } catch (const std::exception& e) {
//        std::cerr << "Ошибка: " << e.what() << std::endl;
//        return 1;
//    }
//
//    return 0;
//}

#include <iostream>
#include <string>
#include "search_cli.hpp"

int main(int argc, char* argv[]) {
    std::cout << "=== Fashion Search Engine (Boolean Search) ===" << std::endl;
    std::cout << "Built: " << __DATE__ << " " << __TIME__ << std::endl;
    std::cout << std::string(50, '=') << std::endl;

    try {
        SearchCLI cli(argc, argv);
        return cli.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}