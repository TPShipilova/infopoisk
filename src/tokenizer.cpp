#include "tokenizer.hpp"
#include <algorithm>
#include <cctype>
#include <locale>
#include <sstream>
#include <iostream>
#include <chrono>

TokenizationResult Tokenizer::tokenize(const std::string& text) {
    auto start_time = std::chrono::high_resolution_clock::now();

    TokenizationResult result;
    std::string current_token;

    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];

        if (is_token_char(c, current_token)) {
            current_token += std::tolower(c);
        } else {
            if (!current_token.empty()) {
                process_token(current_token, result);
                current_token.clear();
            }

            if (c == '@') {
                skip_until_delimiter(text, i);
            } else if (c == '#') {
                handle_hashtag(text, i, result);
            } else if (c == '&') {
                if (text.substr(i, 4) == "&amp;") {
                    i += 4;
                } else if (text.substr(i, 5) == "&quot;") {
                    i += 5;
                }
            }
        }
    }

    if (!current_token.empty()) {
        process_token(current_token, result);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    result.processing_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count();

    return result;
}

bool Tokenizer::is_token_char(char c, const std::string& current_token) {
    if (std::isalnum(static_cast<unsigned char>(c))) {
        return true;
    }

    // Разрешаем апостроф внутри слов (don't, it's)
    if (c == '\'' && !current_token.empty() && current_token.find('\'') == std::string::npos) {
        return true;
    }

    // Разрешаем дефис внутри слов (well-known, state-of-the-art)
    if (c == '-' && !current_token.empty() && i + 1 < text.length() &&
        std::isalnum(static_cast<unsigned char>(text[i + 1]))) {
        return true;
    }

    return false;
}

void Tokenizer::process_token(std::string& token, TokenizationResult& result) {
    if (should_filter_token(token)) {
        return;
    }

    // Удаляем лишние апострофы и дефисы по краям
    cleanup_token(token);

    if (token.length() < 2 || token.length() > 50) {
        return;
    }

    result.tokens.push_back(token);
    result.total_chars += token.length();
}

bool Tokenizer::should_filter_token(const std::string& token) {
    // Фильтрация стоп-слов
    static const std::vector<std::string> stop_words = {
        "the", "and", "for", "are", "but", "not", "you", "all", "any",
        "can", "her", "was", "one", "our", "out", "day", "get", "has",
        "him", "his", "how", "man", "new", "now", "old", "see", "two",
        "who", "boy", "did", "its", "let", "put", "say", "she", "too",
        "use", "was", "way", "who", "why", "yes", "yet", "you"
    };

    if (std::find(stop_words.begin(), stop_words.end(), token) != stop_words.end()) {
        return true;
    }

    // Фильтрация по паттернам
    if (token.find("http") == 0 || token.find("www.") == 0) {
        return true;
    }

    if (std::all_of(token.begin(), token.end(), ::isdigit)) {
        return true;
    }

    return false;
}

void Tokenizer::cleanup_token(std::string& token) {
    // Удаляем апострофы/дефисы с начала и конца
    while (!token.empty() && (token.front() == '\'' || token.front() == '-')) {
        token.erase(0, 1);
    }

    while (!token.empty() && (token.back() == '\'' || token.back() == '-')) {
        token.pop_back();
    }

    // Удаляем множественные дефисы
    size_t pos;
    while ((pos = token.find("--")) != std::string::npos) {
        token.replace(pos, 2, "-");
    }
}

void Tokenizer::skip_until_delimiter(const std::string& text, size_t& i) {
    while (i < text.length() && !std::isspace(text[i])) {
        i++;
    }
}

void Tokenizer::handle_hashtag(const std::string& text, size_t& i, TokenizationResult& result) {
    std::string hashtag;
    i++;

    while (i < text.length() && (std::isalnum(text[i]) || text[i] == '_')) {
        hashtag += std::tolower(text[i]);
        i++;
    }

    if (!hashtag.empty() && hashtag.length() > 1) {
        result.tokens.push_back(hashtag);
        result.total_chars += hashtag.length();
    }
}