#include "zipf_analyzer.hpp"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <iostream>

ZipfAnalysis ZipfAnalyzer::analyze(const std::vector<std::string>& tokens) {
    ZipfAnalysis result;

    // Подсчет частот с использованием нашей хеш-таблицы
    HashTable<std::string, int> frequency_table(10007);

    for (const auto& token : tokens) {
        int count = 0;
        if (frequency_table.get(token, count)) {
            frequency_table.insert(token, count + 1);
        } else {
            frequency_table.insert(token, 1);
        }
    }

    // Получаем все пары и сортируем по частоте
    auto all_pairs = frequency_table.get_all();
    std::sort(all_pairs.begin(), all_pairs.end(),
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });

    // Сохраняем ранги и частоты
    result.rank_freq_pairs.reserve(all_pairs.size());
    for (size_t i = 0; i < all_pairs.size(); ++i) {
        result.rank_freq_pairs.emplace_back(i + 1, all_pairs[i].second);
    }

    // Вычисляем константы для закона Ципфа
    result.total_tokens = tokens.size();
    result.unique_tokens = all_pairs.size();
    result.zipf_constant = calculate_zipf_constant(all_pairs);

    return result;
}

double ZipfAnalyzer::calculate_zipf_constant(const std::vector<std::pair<std::string, int>>& sorted_pairs) {
    if (sorted_pairs.empty()) return 0.0;

    // F(r) = C / r^alpha
    // Для alpha = 1, C ≈ f(1) * 1

    double total_freq = 0.0;
    double total_inv_rank = 0.0;

    for (size_t i = 0; i < std::min(sorted_pairs.size(), size_t(1000)); ++i) {
        double freq = sorted_pairs[i].second;
        double rank = i + 1;

        total_freq += freq;
        total_inv_rank += 1.0 / rank;
    }

    return total_freq / total_inv_rank;
}

void ZipfAnalyzer::save_to_csv(const ZipfAnalysis& analysis, const std::string& filename) {
    std::ofstream file(filename);

    file << "rank,frequency,log_rank,log_freq,zipf_prediction\n";

    for (const auto& [rank, freq] : analysis.rank_freq_pairs) {
        double log_rank = std::log10(rank);
        double log_freq = std::log10(freq);
        double zipf_pred = std::log10(analysis.zipf_constant / rank);

        file << rank << "," << freq << ","
             << log_rank << "," << log_freq << ","
             << zipf_pred << "\n";
    }

    file.close();
}

std::vector<double> ZipfAnalyzer::calculate_mandelbrot(const ZipfAnalysis& analysis,
                                                       double C, double B, double alpha) {
    std::vector<double> predictions;
    predictions.reserve(analysis.rank_freq_pairs.size());

    for (const auto& [rank, _] : analysis.rank_freq_pairs) {
        double prediction = C / std::pow(rank + B, alpha);
        predictions.push_back(prediction);
    }

    return predictions;
}