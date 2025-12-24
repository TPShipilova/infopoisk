#ifndef BOOLEAN_INDEX_HPP
#define BOOLEAN_INDEX_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "document.hpp"
#include "tokenizer.hpp"
#include "stemmer.hpp"
#include "binary_index_format.hpp"

class BooleanIndexBuilder {
public:
    BooleanIndexBuilder();

    void build_from_documents(const std::vector<Document>& documents);

    void save_index(const std::string& filename);

    bool load_index(const std::string& filename);

    // Получение статистики
    struct Statistics {
        size_t total_documents = 0;
        size_t total_terms = 0;
        size_t total_postings = 0;
        double avg_term_length = 0.0;
        double indexing_time_ms = 0.0;
        double avg_doc_length = 0.0;
    };

    Statistics get_statistics() const;

    // Доступ к данным индекса
    const std::vector<ForwardIndexEntry>& get_forward_index() const;
    const std::unordered_map<std::string, std::vector<uint32_t>>& get_inverted_index() const;

    // Стемминг терминов
    std::string normalize_term(const std::string& term);

private:
    Tokenizer tokenizer;
    Stemmer stemmer;

    std::vector<ForwardIndexEntry> forward_index;

    std::unordered_map<std::string, std::vector<uint32_t>> inverted_index;

    Statistics stats;

    void process_document(const Document& doc, uint32_t doc_id);
    void sort_and_unique_postings();

    std::vector<std::pair<std::string, std::vector<uint32_t>>> get_sorted_entries() const;
};

#endif