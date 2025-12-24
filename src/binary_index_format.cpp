#include "binary_index_format.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>

// Магическое число для идентификации нашего формата
const uint32_t MAGIC_NUMBER = 0x48534146;
const uint16_t VERSION = 1;

BinaryIndexWriter::BinaryIndexWriter(const std::string& filename) {
    file.open(filename, std::ios::binary | std::ios::out);
    if (!file) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
}

BinaryIndexWriter::~BinaryIndexWriter() {
    if (file.is_open()) {
        file.close();
    }
}

void BinaryIndexWriter::write_header(uint32_t doc_count, uint32_t term_count) {
    write_uint32(MAGIC_NUMBER);          // magic
    write_uint16(VERSION);               // version
    write_uint16(0);                     // flags (reserved)
    write_uint32(doc_count);             // document count
    write_uint32(term_count);            // term count

    write_uint64(0);
    write_uint64(0);
    write_uint32(0);
    write_uint32(0);
}

void BinaryIndexWriter::write_forward_index(const std::vector<ForwardIndexEntry>& entries) {
    uint64_t forward_offset = get_position();

    write_uint32(static_cast<uint32_t>(entries.size()));

    for (const auto& entry : entries) {
        // ID
        write_string(entry.id);

        // URL
        write_string(entry.url);

        // Заголовок
        write_string(entry.title);

        // Длина документа
        write_uint32(entry.doc_length);

        // Checksum
        write_uint32(entry.checksum);
    }

    uint64_t current_pos = get_position();
    file.seekp(16, std::ios::beg);
    write_uint64(forward_offset);
    file.seekp(current_pos, std::ios::beg);
}

void BinaryIndexWriter::write_inverted_index(const std::vector<std::pair<std::string, std::vector<uint32_t>>>& entries) {
    uint64_t inverted_offset = get_position();

    std::vector<std::pair<std::string, std::vector<uint32_t>>> sorted_entries = entries;
    std::sort(sorted_entries.begin(), sorted_entries.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    write_uint32(static_cast<uint32_t>(sorted_entries.size()));

    for (const auto& entry : sorted_entries) {
        write_string(entry.first);

        write_uint32(static_cast<uint32_t>(entry.second.size()));

        for (uint32_t doc_id : entry.second) {
            write_uint32(doc_id);
        }
    }

    // Обновляем заголовок
    uint64_t current_pos = get_position();
    file.seekp(24, std::ios::beg);
    write_uint64(inverted_offset);
    file.seekp(current_pos, std::ios::beg);
}

uint64_t BinaryIndexWriter::get_position() const {
    return file.tellp();
}

void BinaryIndexWriter::write_string(const std::string& str, bool length_first) {
    if (length_first) {
        if (str.length() > 255) {
            throw std::runtime_error("String too long for 1-byte length");
        }
        write_uint8(static_cast<uint8_t>(str.length()));
    } else {
        if (str.length() > 65535) {
            throw std::runtime_error("String too long for 2-byte length");
        }
        write_uint16(static_cast<uint16_t>(str.length()));
    }
    file.write(str.c_str(), str.length());
}

void BinaryIndexWriter::write_uint32(uint32_t value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void BinaryIndexWriter::write_uint16(uint16_t value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void BinaryIndexWriter::write_uint8(uint8_t value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void BinaryIndexWriter::write_uint64(uint64_t value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

BinaryIndexReader::BinaryIndexReader(const std::string& filename) {
    file.open(filename, std::ios::binary | std::ios::in);
    if (!file) {
        throw std::runtime_error("Cannot open file for reading: " + filename);
    }
}

BinaryIndexReader::~BinaryIndexReader() {
    if (file.is_open()) {
        file.close();
    }
}

bool BinaryIndexReader::read_header(uint32_t& doc_count, uint32_t& term_count) {
    file.seekg(0, std::ios::beg);

    uint32_t magic = read_uint32();
    if (magic != MAGIC_NUMBER) {
        std::cerr << "Invalid file format. Magic: " << std::hex << magic << std::dec << std::endl;
        return false;
    }

    uint16_t version = read_uint16();
    if (version != VERSION) {
        std::cerr << "Unsupported version: " << version << std::endl;
        return false;
    }

    read_uint16();
    total_docs = read_uint32();
    total_terms = read_uint32();
    forward_offset = read_uint64();
    inverted_offset = read_uint64();
    read_uint32();
    read_uint32();

    doc_count = total_docs;
    term_count = total_terms;

    return true;
}

std::vector<ForwardIndexEntry> BinaryIndexReader::read_forward_index() {
    if (forward_offset == 0) {
        throw std::runtime_error("Forward index offset not set");
    }

    file.seekg(forward_offset, std::ios::beg);
    uint32_t doc_count = read_uint32();

    std::vector<ForwardIndexEntry> entries;
    entries.reserve(doc_count);

    for (uint32_t i = 0; i < doc_count; ++i) {
        ForwardIndexEntry entry;

        // ID
        entry.id = read_string(true);

        // URL
        entry.url = read_string(false);

        // Title
        entry.title = read_string(false);

        // Document length
        entry.doc_length = read_uint32();

        // Checksum
        entry.checksum = read_uint32();

        entries.push_back(entry);
    }

    forward_cache = entries;
    return entries;
}

std::vector<std::pair<std::string, std::vector<uint32_t>>> BinaryIndexReader::read_inverted_index() {
    if (inverted_offset == 0) {
        throw std::runtime_error("Inverted index offset not set");
    }

    file.seekg(inverted_offset, std::ios::beg);
    uint32_t term_count = read_uint32();

    std::vector<std::pair<std::string, std::vector<uint32_t>>> entries;
    entries.reserve(term_count);

    for (uint32_t i = 0; i < term_count; ++i) {
        std::string term = read_string(true);
        uint32_t doc_count = read_uint32();

        std::vector<uint32_t> doc_ids;
        doc_ids.reserve(doc_count);

        for (uint32_t j = 0; j < doc_count; ++j) {
            doc_ids.push_back(read_uint32());
        }

        entries.emplace_back(term, std::move(doc_ids));
    }

    return entries;
}

std::vector<uint32_t> BinaryIndexReader::find_term(const std::string& term) {
    // Если индекс еще не построен, строим его
    if (term_positions.empty()) {
        build_term_index();
    }

    // Бинарный поиск по термину
    auto it = std::lower_bound(term_positions.begin(), term_positions.end(), term,
                               [](const auto& a, const std::string& b) {
                                   return a.first < b;
                               });

    if (it == term_positions.end() || it->first != term) {
        return {};  // Термин не найден
    }

    file.seekg(it->second, std::ios::beg);

    read_string(true);
    uint32_t doc_count = read_uint32();

    std::vector<uint32_t> doc_ids;
    doc_ids.reserve(doc_count);

    for (uint32_t i = 0; i < doc_count; ++i) {
        doc_ids.push_back(read_uint32());
    }

    return doc_ids;
}

ForwardIndexEntry BinaryIndexReader::get_document_info(uint32_t doc_id) {
    if (forward_cache.empty()) {
        read_forward_index();
    }

    if (doc_id >= forward_cache.size()) {
        throw std::out_of_range("Document ID out of range");
    }

    return forward_cache[doc_id];
}

void BinaryIndexReader::build_term_index() {
    if (inverted_offset == 0) {
        read_header(total_docs, total_terms);
    }

    file.seekg(inverted_offset, std::ios::beg);
    uint32_t term_count = read_uint32();

    term_positions.clear();
    term_positions.reserve(term_count);

    for (uint32_t i = 0; i < term_count; ++i) {
        uint64_t position = file.tellg();
        std::string term = read_string(true);
        term_positions.emplace_back(term, position);

        // Пропускаем данные для этого термина
        uint32_t doc_count = read_uint32();
        file.seekg(doc_count * sizeof(uint32_t), std::ios::cur);
    }
}

std::string BinaryIndexReader::read_string(bool length_first) {
    size_t length = 0;
    if (length_first) {
        length = read_uint8();
    } else {
        length = read_uint16();
    }

    std::string str(length, '\0');
    file.read(&str[0], length);
    return str;
}

uint32_t BinaryIndexReader::read_uint32() {
    uint32_t value;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

uint16_t BinaryIndexReader::read_uint16() {
    uint16_t value;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

uint8_t BinaryIndexReader::read_uint8() {
    uint8_t value;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

uint64_t BinaryIndexReader::read_uint64() {
    uint64_t value;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}