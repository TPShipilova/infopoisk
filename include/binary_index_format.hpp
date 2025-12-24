#ifndef BINARY_INDEX_FORMAT_HPP
#define BINARY_INDEX_FORMAT_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <fstream>

/*
 * Бинарный формат индекса:
 *
 * Файл состоит из заголовка, прямого индекса и обратного индекса.
 *
 * 1. Заголовок (32 байта):
 *    [magic: 4 байта] = "FASH"
 *    [version: 2 байта] = 1
 *    [doc_count: 4 байта] = количество документов
 *    [term_count: 4 байта] = количество уникальных терминов
 *    [forward_offset: 8 байт] = смещение к прямому индексу
 *    [inverted_offset: 8 байт] = смещение к обратному индексу
 *    [reserved: 2 байта] = 0
 *
 * 2. Прямой индекс:
 *    [doc_count] записей, каждая:
 *      [id_len: 1 байт] - длина ID
 *      [id: id_len байт] - ID документа
 *      [url_len: 2 байта] - длина URL
 *      [url: url_len байт] - URL документа
 *      [title_len: 2 байта] - длина заголовка
 *      [title: title_len байт] - заголовок документа
 *      [doc_length: 4 байта] - количество терминов в документе
 *      [checksum: 4 байта] - контрольная сумма
 *
 * 3. Обратный индекс:
 *    [term_count] записей, каждая:
 *      [term_len: 1 байт] - длина термина
 *      [term: term_len байт] - термин (нижний регистр)
 *      [doc_count: 4 байта] - количество документов с этим термином
 *      [doc_ids: doc_count * 4 байт] - список ID документов
 */

class BinaryIndexWriter {
public:
    BinaryIndexWriter(const std::string& filename);
    ~BinaryIndexWriter();

    // Запись заголовка
    void write_header(uint32_t doc_count, uint32_t term_count);

    // Запись прямого индекса
    void write_forward_index(const std::vector<ForwardIndexEntry>& entries);

    // Запись обратного индекса
    void write_inverted_index(const std::vector<std::pair<std::string, std::vector<uint32_t>>>& entries);

    // Получение текущей позиции
    uint64_t get_position() const;

private:
    std::ofstream file;

    void write_string(const std::string& str, bool length_first = true);
    void write_uint32(uint32_t value);
    void write_uint16(uint16_t value);
    void write_uint8(uint8_t value);
    void write_uint64(uint64_t value);
};

class BinaryIndexReader {
public:
    BinaryIndexReader(const std::string& filename);
    ~BinaryIndexReader();

    // Чтение заголовка
    bool read_header(uint32_t& doc_count, uint32_t& term_count);

    // Чтение прямого индекса
    std::vector<ForwardIndexEntry> read_forward_index();

    // Чтение обратного индекса
    std::vector<std::pair<std::string, std::vector<uint32_t>>> read_inverted_index();

    // Быстрый поиск записи по термину (бинарный поиск)
    std::vector<uint32_t> find_term(const std::string& term);

    // Получение информации о документе по ID
    ForwardIndexEntry get_document_info(uint32_t doc_id);

private:
    std::ifstream file;
    uint64_t forward_offset = 0;
    uint64_t inverted_offset = 0;
    uint32_t total_docs = 0;
    uint32_t total_terms = 0;

    std::string read_string(bool length_first = true);
    uint32_t read_uint32();
    uint16_t read_uint16();
    uint8_t read_uint8();
    uint64_t read_uint64();

    // Кэш для быстрого доступа
    std::vector<ForwardIndexEntry> forward_cache;
    std::vector<std::pair<std::string, uint64_t>> term_positions;

    void build_term_index(); // Строит индекс терминов для быстрого поиска
};

#endif