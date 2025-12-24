#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <string>
#include <vector>

struct TokenizationResult {
    std::vector<std::string> tokens;
    size_t total_chars = 0;
    long long processing_time_ms = 0;
};

class Tokenizer {
private:
    bool is_token_char(char c, const std::string& current_token);
    void process_token(std::string& token, TokenizationResult& result);
    bool should_filter_token(const std::string& token);
    void cleanup_token(std::string& token);
    void skip_until_delimiter(const std::string& text, size_t& i);
    void handle_hashtag(const std::string& text, size_t& i, TokenizationResult& result);

public:
    TokenizationResult tokenize(const std::string& text);
};