#ifndef HASH_TABLE_HPP
#define HASH_TABLE_HPP

#include <vector>
#include <string>
#include <functional>
#include <iostream>

template<typename K, typename V>
class HashTable {
private:
    struct Entry {
        K key;
        V value;
        bool occupied;
        bool deleted;

        Entry() : occupied(false), deleted(false) {}
    };

    std::vector<Entry> table;
    size_t size;
    size_t count;
    float load_factor_threshold;

    size_t hash(const K& key) const {
        std::hash<K> hasher;
        return hasher(key) % table.size();
    }

    size_t probe(size_t index, size_t i) const {
        // Квадратичное пробирование
        return (index + i * i) % table.size();
    }

    void rehash() {
        std::vector<Entry> old_table = table;
        table.clear();
        table.resize(size * 2, Entry());
        size = table.size();
        count = 0;

        for (const auto& entry : old_table) {
            if (entry.occupied && !entry.deleted) {
                insert(entry.key, entry.value);
            }
        }
    }

public:
    HashTable(size_t initial_size = 101)
        : table(initial_size), size(initial_size),
          count(0), load_factor_threshold(0.75f) {}

    void insert(const K& key, const V& value) {
        if (float(count) / size > load_factor_threshold) {
            rehash();
        }

        size_t index = hash(key);
        size_t i = 0;

        while (i < size) {
            size_t idx = probe(index, i);

            if (!table[idx].occupied || table[idx].deleted) {
                table[idx].key = key;
                table[idx].value = value;
                table[idx].occupied = true;
                table[idx].deleted = false;
                count++;
                return;
            }

            if (table[idx].key == key) {
                table[idx].value = value;
                return;
            }

            i++;
        }

        rehash();
        insert(key, value);
    }

    bool get(const K& key, V& value) const {
        size_t index = hash(key);
        size_t i = 0;

        while (i < size) {
            size_t idx = probe(index, i);

            if (!table[idx].occupied) {
                return false;
            }

            if (table[idx].occupied && !table[idx].deleted && table[idx].key == key) {
                value = table[idx].value;
                return true;
            }

            i++;
        }

        return false;
    }

    bool contains(const K& key) const {
        V dummy;
        return get(key, dummy);
    }

    bool remove(const K& key) {
        size_t index = hash(key);
        size_t i = 0;

        while (i < size) {
            size_t idx = probe(index, i);

            if (!table[idx].occupied) {
                return false;
            }

            if (table[idx].occupied && !table[idx].deleted && table[idx].key == key) {
                table[idx].deleted = true;
                count--;
                return true;
            }

            i++;
        }

        return false;
    }

    size_t get_count() const { return count; }
    size_t get_size() const { return size; }

    std::vector<std::pair<K, V>> get_all() const {
        std::vector<std::pair<K, V>> result;
        result.reserve(count);

        for (const auto& entry : table) {
            if (entry.occupied && !entry.deleted) {
                result.emplace_back(entry.key, entry.value);
            }
        }

        return result;
    }

    void clear() {
        for (auto& entry : table) {
            entry.occupied = false;
            entry.deleted = false;
        }
        count = 0;
    }
};

#endif