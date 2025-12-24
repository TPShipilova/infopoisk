#ifndef SEARCH_CLI_HPP
#define SEARCH_CLI_HPP

#include <string>
#include <vector>

class SearchCLI {
public:
    struct Config {
        std::string index_file = "fashion_index.bin";
        std::string query_file;
        std::string output_file;
        bool interactive = false;
        bool build_index = false;
        bool show_stats = false;
        int limit_results = 50;
    };

    SearchCLI(int argc, char* argv[]);

    int run();

    // Показать справку
    static void show_help();

private:
    Config config;

    // Обработка аргументов командной строки
    bool parse_arguments(int argc, char* argv[]);

    // Режимы работы
    int run_interactive();
    int run_batch();
    int run_build_index();
    int run_show_stats();

    void print_results(const std::vector<uint32_t>& doc_ids,
                       const std::string& query = "");
    void save_results(const std::vector<uint32_t>& doc_ids,
                      const std::string& query,
                      const std::string& filename);
};

#endif