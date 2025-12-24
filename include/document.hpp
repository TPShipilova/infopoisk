#ifndef DOCUMENT_HPP
#define DOCUMENT_HPP

#include <string>
#include <vector>

struct Document {
    std::string id;
    std::string url;
    std::string title;
    std::string content;
    std::string source;
    int word_count = 0;
    std::vector<std::string> tokens;
    std::vector<std::string> stemmed_tokens;
};

// Прямой индекс (document -> metadata)
struct ForwardIndexEntry {
    std::string id;
    std::string url;
    std::string title;
    uint32_t doc_length;  // Количество терминов
    uint64_t offset;      // Смещение в файле (для сжатых данных)
    uint32_t checksum;    // Контрольная сумма
};

// Обратный индекс (term -> [doc_ids])
struct InvertedIndexEntry {
    std::string term;
    std::vector<uint32_t> doc_ids;      // Список документов
    std::vector<uint16_t> positions;    // Позиции в документе (для будущего использования)
    std::vector<uint8_t> frequencies;   // Частоты (для будущего использования)
};

#endif