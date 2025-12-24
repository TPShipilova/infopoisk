#ifndef ZIPF_ANALYZER_HPP
#define ZIPF_ANALYZER_HPP

#include <vector>
#include <string>
#include <utility>
#include "hash_table.hpp"

struct ZipfAnalysis {
    std::vector<std::pair<int, int>> rank_freq_pairs;
    size_t total_tokens = 0;
    size_t unique_tokens = 0;
    double zipf_constant = 0.0;
};

class ZipfAnalyzer {
public:
    ZipfAnalysis analyze(const std::vector<std::string>& tokens);
    void save_to_csv(const ZipfAnalysis& analysis, const std::string& filename);
    std::vector<double> calculate_mandelbrot(const ZipfAnalysis& analysis,
                                             double C, double B, double alpha);

private:
    double calculate_zipf_constant(const std::vector<std::pair<std::string, int>>& sorted_pairs);
};