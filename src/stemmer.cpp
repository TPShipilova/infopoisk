//#include "stemmer.hpp"
#include <algorithm>

Stemmer::Stemmer() {
    step1_suffixes = {
        "sses", "ies", "ss", "s"
    };

    step2_suffixes = {
        "ational", "tional", "enci", "anci", "izer", "abli", "alli",
        "entli", "eli", "ousli", "ization", "ation", "ator", "alism",
        "iveness", "fulness", "ousness", "aliti", "iviti", "biliti"
    };

    step3_suffixes = {
        "icate", "ative", "alize", "iciti", "ical", "ful", "ness"
    };

    step4_suffixes = {
        "al", "ance", "ence", "er", "ic", "able", "ible", "ant",
        "ement", "ment", "ent", "ion", "ou", "ism", "ate", "iti",
        "ous", "ive", "ize"
    };
}

std::string Stemmer::stem(const std::string& word) {
    if (word.length() < 3) return word;

    std::string result = word;

    // Шаг 1: Плюралы и притяжательные формы
    result = step1(result);

    // Шаг 2: Удаление общих суффиксов
    result = step2(result);

    // Шаг 3: Дополнительные суффиксы
    result = step3(result);

    // Шаг 4: Финальные суффиксы
    result = step4(result);

    // Шаг 5: Окончательная очистка
    result = step5(result);

    return result;
}

std::string Stemmer::step1(const std::string& word) {
    std::string result = word;

    if (ends_with(result, "sses")) {
        result = replace_suffix(result, "sses", "ss");
    } else if (ends_with(result, "ies")) {
        result = replace_suffix(result, "ies", "i");
    } else if (ends_with(result, "ss")) {
        // Оставляем как есть
    } else if (ends_with(result, "s")) {
        result = replace_suffix(result, "s", "");
    }

    return result;
}

std::string Stemmer::step2(const std::string& word) {
    std::string result = word;

    for (const auto& suffix : step2_suffixes) {
        if (ends_with(result, suffix)) {
            std::string stem = result.substr(0, result.length() - suffix.length());
            if (measure(stem) > 0) {
                if (suffix == "ational") {
                    return stem + "ate";
                } else if (suffix == "tional") {
                    return stem + "tion";
                } else if (suffix == "enci") {
                    return stem + "ence";
                } else if (suffix == "anci") {
                    return stem + "ance";
                } else if (suffix == "izer") {
                    return stem + "ize";
                }
            }
        }
    }

    return result;
}

std::string Stemmer::step3(const std::string& word) {
    std::string result = word;

    for (const auto& suffix : step3_suffixes) {
        if (ends_with(result, suffix)) {
            std::string stem = result.substr(0, result.length() - suffix.length());
            if (measure(stem) > 0) {
                if (suffix == "icate") {
                    return stem + "ic";
                } else if (suffix == "ative") {
                    return stem;
                } else if (suffix == "alize") {
                    return stem + "al";
                } else if (suffix == "iciti") {
                    return stem + "ic";
                }
            }
        }
    }

    return result;
}

std::string Stemmer::step4(const std::string& word) {
    std::string result = word;

    for (const auto& suffix : step4_suffixes) {
        if (ends_with(result, suffix)) {
            std::string stem = result.substr(0, result.length() - suffix.length());
            if (measure(stem) > 1) {
                if (suffix == "ion") {
                    // Удаляем только если перед suffix стоит s или t
                    char c = stem.back();
                    if (c == 's' || c == 't') {
                        return stem;
                    }
                } else {
                    return stem;
                }
            }
        }
    }

    return result;
}

std::string Stemmer::step5(const std::string& word) {
    std::string result = word;

    // Удаляем конечные 'e'
    if (ends_with(result, "e")) {
        std::string stem = result.substr(0, result.length() - 1);
        if (measure(stem) > 1) {
            return stem;
        }
        if (measure(stem) == 1 && !ends_with_cvc(stem)) {
            return stem;
        }
    }

    // Удаляем двойные 'l'
    if (ends_with(result, "ll") && measure(result) > 1) {
        return result.substr(0, result.length() - 1);
    }

    return result;
}

bool Stemmer::ends_with(const std::string& word, const std::string& suffix) {
    if (word.length() < suffix.length()) return false;
    return word.substr(word.length() - suffix.length()) == suffix;
}

std::string Stemmer::replace_suffix(const std::string& word,
                                    const std::string& old_suffix,
                                    const std::string& new_suffix) {
    return word.substr(0, word.length() - old_suffix.length()) + new_suffix;
}

int Stemmer::measure(const std::string& stem) {
    // Подсчет VC паттернов (гласные и согласные)
    int count = 0;
    bool last_was_vowel = false;

    for (char c : stem) {
        bool is_vowel = is_vowel_char(c);

        if (!last_was_vowel && is_vowel) {
            count++;
        }

        last_was_vowel = is_vowel;
    }

    return count;
}

bool Stemmer::is_vowel_char(char c) {
    c = std::tolower(c);
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
}

bool Stemmer::ends_with_cvc(const std::string& word) {
    if (word.length() < 3) return false;

    char c1 = word[word.length() - 3];
    char c2 = word[word.length() - 2];
    char c3 = word[word.length() - 1];

    return !is_vowel_char(c1) && is_vowel_char(c2) && !is_vowel_char(c3) &&
           c3 != 'w' && c3 != 'x' && c3 != 'y';
}