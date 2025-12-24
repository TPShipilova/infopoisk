#ifndef BOOLEAN_SEARCH_HPP
#define BOOLEAN_SEARCH_HPP

#include <string>
#include <vector>
#include <memory>
#include <unordered_set>
#include "boolean_index.hpp"

class BooleanSearch {
public:
    enum class TokenType {
        TERM,
        AND,
        OR,
        NOT,
        LPAREN,
        RPAREN,
        END
    };

    struct QueryToken {
        TokenType type;
        std::string value;

        QueryToken(TokenType t, const std::string& v = "") : type(t), value(v) {}
    };

    BooleanSearch(const BooleanIndexBuilder& index);

    // Выполнение поискового запроса
    std::vector<uint32_t> search(const std::string& query);

    // Пакетный поиск
    std::vector<std::pair<std::string, std::vector<uint32_t>>> batch_search(
        const std::vector<std::string>& queries);

    // Получение статистики по запросу
    struct SearchStats {
        std::string query;
        size_t result_count = 0;
        double processing_time_ms = 0.0;
        size_t terms_processed = 0;
    };

    SearchStats get_last_stats() const;

    // Форматирование результатов
    struct SearchResult {
        uint32_t doc_id;
        std::string title;
        std::string url;
        double relevance = 0.0;  // Для будущего использования
    };

    std::vector<SearchResult> format_results(const std::vector<uint32_t>& doc_ids,
                                             size_t offset = 0,
                                             size_t limit = 50) const;

private:
    const BooleanIndexBuilder& index;
    SearchStats last_stats;

    // Парсинг запроса
    std::vector<QueryToken> tokenize_query(const std::string& query);
    std::vector<uint32_t> parse_expression(const std::vector<QueryToken>& tokens,
                                           size_t& pos);
    std::vector<uint32_t> parse_term(const std::vector<QueryToken>& tokens,
                                     size_t& pos);
    std::vector<uint32_t> parse_factor(const std::vector<QueryToken>& tokens,
                                       size_t& pos);

    // Операции над множествами
    std::vector<uint32_t> intersect_sets(const std::vector<uint32_t>& a,
                                         const std::vector<uint32_t>& b);
    std::vector<uint32_t> union_sets(const std::vector<uint32_t>& a,
                                     const std::vector<uint32_t>& b);
    std::vector<uint32_t> complement_set(const std::vector<uint32_t>& a);

    // Нормализация термина
    std::string normalize_term(const std::string& term);

    // Получение постингов для термина
    std::vector<uint32_t> get_postings(const std::string& term);

    // Все документы (для операции NOT)
    std::vector<uint32_t> all_documents;

    void init_all_documents();
};

#endif