#include <chrono>
#include <iostream>

MongoConnector::MongoConnector(const std::string& uri, const std::string& db_name) {
    try {
        mongocxx::instance instance{};
        mongocxx::client client{mongocxx::uri{uri}};

        db = client[db_name];
        documents_collection = db["documents"];

        std::cout << "Connected to MongoDB successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "MongoDB connection failed: " << e.what() << std::endl;
        throw;
    }
}

std::vector<Document> MongoConnector::fetch_documents(int limit) {
    std::vector<Document> docs;

    try {
        auto cursor = documents_collection.find({}).limit(limit);

        for (auto&& doc : cursor) {
            Document document;

            if (doc["_id"]) {
                document.id = doc["_id"].get_oid().value.to_string();
            }

            if (doc["url"]) {
                document.url = doc["url"].get_string().value.to_string();
            }

            if (doc["title"]) {
                document.title = doc["title"].get_string().value.to_string();
            }

            if (doc["content"]) {
                document.content = doc["content"].get_string().value.to_string();
            }

            if (doc["source"]) {
                document.source = doc["source"].get_string().value.to_string();
            }

            if (doc["word_count"]) {
                document.word_count = doc["word_count"].get_int32().value;
            }

            docs.push_back(document);
        }

        std::cout << "Fetched " << docs.size() << " documents from MongoDB" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error fetching documents: " << e.what() << std::endl;
    }

    return docs;
}

size_t MongoConnector::get_total_documents() {
    try {
        return documents_collection.count_documents({});
    } catch (const std::exception& e) {
        std::cerr << "Error counting documents: " << e.what() << std::endl;
        return 0;
    }
}