#include "search_cli.hpp"
#include "boolean_index.hpp"
#include "boolean_search.hpp"
#include "file_connector.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>

SearchCLI::SearchCLI(int argc, char* argv[]) {
    if (!parse_arguments(argc, argv)) {
        std::exit(1);
    }
}

bool SearchCLI::parse_arguments(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Error: No arguments provided\n" << std::endl;
        show_help();
        return false;
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            show_help();
            return false;
        } else if (arg == "-i" || arg == "--interactive") {
            config.interactive = true;
        } else if (arg == "-b" || arg == "--build") {
            config.build_index = true;
        } else if (arg == "-s" || arg == "--stats") {
            config.show_stats = true;
        } else if (arg == "-f" || arg == "--file") {
            if (i + 1 < argc) {
                config.query_file = argv[++i];
            } else {
                std::cerr << "Error: Missing filename after --file" << std::endl;
                return false;
            }
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                config.output_file = argv[++i];
            } else {
                std::cerr << "Error: Missing filename after --output" << std::endl;
                return false;
            }
        } else if (arg == "-l" || arg == "--limit") {
            if (i + 1 < argc) {
                config.limit_results = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: Missing number after --limit" << std::endl;
                return false;
            }
        } else if (arg == "--index") {
            if (i + 1 < argc) {
                config.index_file = argv[++i];
            } else {
                std::cerr << "Error: Missing filename after --index" << std::endl;
                return false;
            }
        } else if (arg[0] == '-') {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            return false;
        } else {
            std::string query;
            for (int j = i; j < argc; ++j) {
                if (j > i) query += " ";
                query += argv[j];
            }
            config.query_file = query;
            break;
        }
    }

    return true;
}

int SearchCLI::run() {
    try {
        if (config.build_index) {
            return run_build_index();
        } else if (config.show_stats) {
            return run_show_stats();
        } else if (config.interactive) {
            return run_interactive();
        } else if (!config.query_file.empty()) {
            return run_batch();
        } else {
            std::cerr << "Error: No operation specified\n" << std::endl;
            show_help();
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

int SearchCLI::run_build_index() {
    std::cout << "Building index..." << std::endl;

    FileConnector connector("fashion_data_compact.json");
    auto documents = connector.fetch_documents();

    if (documents.empty()) {
        std::cerr << "No documents found. Please ensure data file exists." << std::endl;
        return 1;
    }

    std::cout << "Loaded " << documents.size() << " documents" << std::endl;

    // Строим индекс
    BooleanIndexBuilder index_builder;
    index_builder.build_from_documents(documents);

    index_builder.save_index(config.index_file);

    auto stats = index_builder.get_statistics();
    std::cout << "\nIndex Statistics:" << std::endl;
    std::cout << "  Documents: " << stats.total_documents << std::endl;
    std::cout << "  Unique terms: " << stats.total_terms << std::endl;
    std::cout << "  Total postings: " << stats.total_postings << std::endl;
    std::cout << "  Avg term length: " << std::fixed << std::setprecision(2)
              << stats.avg_term_length << " chars" << std::endl;
    std::cout << "  Avg doc length: " << stats.avg_doc_length << " terms" << std::endl;
    std::cout << "  Indexing time: " << stats.indexing_time_ms << " ms" << std::endl;

    return 0;
}

int SearchCLI::run_show_stats() {
    std::cout << "Loading index: " << config.index_file << std::endl;

    BooleanIndexBuilder index_builder;
    if (!index_builder.load_index(config.index_file)) {
        std::cerr << "Failed to load index" << std::endl;
        return 1;
    }

    auto stats = index_builder.get_statistics();
    std::cout << "\nIndex Statistics:" << std::endl;
    std::cout << "  Documents: " << stats.total_documents << std::endl;
    std::cout << "  Unique terms: " << stats.total_terms << std::endl;
    std::cout << "  Total postings: " << stats.total_postings << std::endl;
    std::cout << "  Avg term length: " << std::fixed << std::setprecision(2)
              << stats.avg_term_length << " chars" << std::endl;
    std::cout << "  Avg doc length: " << stats.avg_doc_length << " terms" << std::endl;
    std::cout << "  Indexing time: " << stats.indexing_time_ms << " ms" << std::endl;

    return 0;
}

int SearchCLI::run_interactive() {
    std::cout << "Loading index: " << config.index_file << std::endl;

    BooleanIndexBuilder index_builder;
    if (!index_builder.load_index(config.index_file)) {
        std::cerr << "Failed to load index" << std::endl;
        return 1;
    }

    BooleanSearch searcher(index_builder);

    std::cout << "\n=== Boolean Search Interactive Mode ===" << std::endl;
    std::cout << "Index loaded: " << index_builder.get_statistics().total_documents
              << " documents" << std::endl;
    std::cout << "Type 'quit' or 'exit' to quit" << std::endl;
    std::cout << "Supported operators: AND (&&), OR (||), NOT (!), parentheses" << std::endl;
    std::cout << "Example: fashion AND (design || trend) !shoes" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    while (true) {
        std::cout << "\nQuery: ";
        std::string query;
        std::getline(std::cin, query);

        if (query.empty()) {
            continue;
        }

        if (query == "quit" || query == "exit") {
            break;
        }

        if (query == "help") {
            std::cout << "\nBoolean Search Syntax:" << std::endl;
            std::cout << "  fashion design          - implicit AND" << std::endl;
            std::cout << "  fashion && design       - explicit AND" << std::endl;
            std::cout << "  fashion || design       - OR" << std::endl;
            std::cout << "  !shoes                  - NOT" << std::endl;
            std::cout << "  (fashion || style) && design - parentheses" << std::endl;
            continue;
        }

        auto start_time = std::chrono::high_resolution_clock::now();
        auto results = searcher.search(query);
        auto end_time = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();

        std::cout << "\nFound " << results.size() << " results in "
                  << duration << " ms" << std::endl;

        if (!results.empty()) {
            auto formatted = searcher.format_results(results, 0, config.limit_results);

            for (size_t i = 0; i < formatted.size(); ++i) {
                const auto& result = formatted[i];
                std::cout << "\n" << (i + 1) << ". " << result.title << std::endl;
                std::cout << "    URL: " << result.url << std::endl;
                std::cout << "    Doc ID: " << result.doc_id << std::endl;
            }

            if (results.size() > config.limit_results) {
                std::cout << "\n... and " << (results.size() - config.limit_results)
                          << " more results (use --limit to show more)" << std::endl;
            }
        }

        if (!config.output_file.empty()) {
            save_results(results, query, config.output_file);
        }
    }

    return 0;
}

int SearchCLI::run_batch() {
    std::cout << "Loading index: " << config.index_file << std::endl;

    BooleanIndexBuilder index_builder;
    if (!index_builder.load_index(config.index_file)) {
        std::cerr << "Failed to load index" << std::endl;
        return 1;
    }

    BooleanSearch searcher(index_builder);
    std::vector<std::string> queries;

    // Проверяем, является ли query_file именем файла или самим запросом
    std::ifstream file(config.query_file);
    if (file.good()) {
        // Это файл
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                queries.push_back(line);
            }
        }
        file.close();
    } else {
        // Это сам запрос
        queries.push_back(config.query_file);
    }

    std::cout << "Processing " << queries.size() << " queries..." << std::endl;

    auto batch_start = std::chrono::high_resolution_clock::now();
    auto batch_results = searcher.batch_search(queries);
    auto batch_end = std::chrono::high_resolution_clock::now();

    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        batch_end - batch_start).count();

    for (size_t i = 0; i < batch_results.size(); ++i) {
        const auto& [query, results] = batch_results[i];
        std::cout << "\nQuery " << (i + 1) << ": \"" << query << "\"" << std::endl;
        std::cout << "  Results: " << results.size() << std::endl;

        if (!results.empty() && config.limit_results > 0) {
            auto formatted = searcher.format_results(results, 0,
                                                     std::min(config.limit_results, 5));

            for (size_t j = 0; j < formatted.size(); ++j) {
                std::cout << "    " << (j + 1) << ". " << formatted[j].title << std::endl;
            }

            if (results.size() > formatted.size()) {
                std::cout << "    ... and " << (results.size() - formatted.size())
                          << " more" << std::endl;
            }
        }
    }

    std::cout << "\nBatch processing completed in " << total_time << " ms" << std::endl;
    std::cout << "Average time per query: "
              << (queries.empty() ? 0 : total_time / queries.size()) << " ms" << std::endl;

    if (!config.output_file.empty()) {
        std::ofstream outfile(config.output_file);
        if (outfile) {
            for (const auto& [query, results] : batch_results) {
                outfile << "Query: " << query << "\n";
                outfile << "Results: " << results.size() << "\n";

                auto formatted = searcher.format_results(results, 0, results.size());
                for (const auto& result : formatted) {
                    outfile << "  - " << result.title << " (" << result.url << ")\n";
                }

                outfile << "\n";
            }
            outfile.close();
            std::cout << "Results saved to: " << config.output_file << std::endl;
        }
    }

    return 0;
}

void SearchCLI::save_results(const std::vector<uint32_t>& doc_ids,
                             const std::string& query,
                             const std::string& filename) {
    std::ofstream outfile(filename, std::ios::app);
    if (outfile) {
        outfile << "Query: " << query << "\n";
        outfile << "Results: " << doc_ids.size() << "\n";
        outfile << "Timestamp: " << std::chrono::system_clock::now().time_since_epoch().count()
                << "\n\n";
        outfile.close();
    }
}

void SearchCLI::show_help() {
    std::cout << "Boolean Search Engine - Command Line Interface\n" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  fashion_search_engine [OPTIONS] [QUERY]\n" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help              Show this help message" << std::endl;
    std::cout << "  -i, --interactive       Run in interactive mode" << std::endl;
    std::cout << "  -b, --build             Build index from data file" << std::endl;
    std::cout << "  -s, --stats             Show index statistics" << std::endl;
    std::cout << "  -f, --file FILE         Read queries from file" << std::endl;
    std::cout << "  -o, --output FILE       Save results to file" << std::endl;
    std::cout << "  -l, --limit N           Limit results to N (default: 50)" << std::endl;
    std::cout << "  --index FILE            Specify index file (default: fashion_index.bin)" << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  Build index:            fashion_search_engine --build" << std::endl;
    std::cout << "  Interactive search:     fashion_search_engine --interactive" << std::endl;
    std::cout << "  Single query:           fashion_search_engine \"fashion AND design\"" << std::endl;
    std::cout << "  Batch search:           fashion_search_engine --file queries.txt" << std::endl;
    std::cout << "  Show stats:             fashion_search_engine --stats" << std::endl;
}