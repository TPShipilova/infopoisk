cmake_minimum_required(VERSION 3.15)
project(fashion_search_engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опция для MongoDB
option(USE_MONGODB "Build with MongoDB support" OFF)

if(USE_MONGODB)
    # Поиск MongoDB C++ Driver через pkg-config
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(MONGOCXX libmongocxx)
        if(MONGOCXX_FOUND)
            message(STATUS "Found MongoDB C++ Driver via pkg-config")
        endif()
    endif()

    # Если pkg-config не сработал, ищем вручную
    if(NOT MONGOCXX_FOUND)
        find_path(MONGOCXX_INCLUDE_DIR mongocxx/client.hpp
            PATHS
                /usr/local/include/mongocxx/v_noabi
                /opt/homebrew/include/mongocxx/v_noabi
        )

        find_path(BSONCXX_INCLUDE_DIR bsoncxx/builder/stream/document.hpp
            PATHS
                /usr/local/include/bsoncxx/v_noabi
                /opt/homebrew/include/bsoncxx/v_noabi
        )

        find_library(MONGOCXX_LIBRARY mongocxx
            PATHS
                /usr/local/lib
                /opt/homebrew/lib
        )

        find_library(BSONCXX_LIBRARY bsoncxx
            PATHS
                /usr/local/lib
                /opt/homebrew/lib
        )

        if(MONGOCXX_INCLUDE_DIR AND BSONCXX_INCLUDE_DIR AND MONGOCXX_LIBRARY AND BSONCXX_LIBRARY)
            message(STATUS "Found MongoDB C++ Driver manually")
            set(MONGOCXX_INCLUDE_DIRS ${MONGOCXX_INCLUDE_DIR} ${BSONCXX_INCLUDE_DIR})
            set(MONGOCXX_LIBRARIES ${MONGOCXX_LIBRARY} ${BSONCXX_LIBRARY})
            set(MONGOCXX_FOUND TRUE)
        else()
            message(WARNING "MongoDB C++ Driver not found. Disabling MongoDB support.")
            set(MONGOCXX_FOUND FALSE)
        endif()
    endif()
endif()

# Основной исполняемый файл
add_executable(fashion_search_engine
    src/main.cpp
    src/tokenizer.cpp
    src/zipf_analyzer.cpp
    src/stemmer.cpp
    src/boolean_index.cpp
    src/boolean_search.cpp
    src/binary_index_format.cpp
    src/search_cli.cpp
)

# Если MongoDB найден, добавляем mongo_connector
if(USE_MONGODB AND MONGOCXX_FOUND)
    target_sources(fashion_search_engine PRIVATE src/mongo_connector.cpp)
    target_include_directories(fashion_search_engine PRIVATE ${MONGOCXX_INCLUDE_DIRS})
    target_link_libraries(fashion_search_engine ${MONGOCXX_LIBRARIES})
    target_compile_definitions(fashion_search_engine PRIVATE USE_MONGODB=1)
    message(STATUS "Building with MongoDB support")
else()
    target_compile_definitions(fashion_search_engine PRIVATE USE_MONGODB=0)
    message(STATUS "Building without MongoDB support")
endif()

# Общие настройки
target_include_directories(fashion_search_engine PRIVATE include)
target_compile_features(fashion_search_engine PRIVATE cxx_std_17)

# Для macOS
if(APPLE)
    find_library(CORE_FOUNDATION CoreFoundation)
    find_library(SECURITY Security)
    find_library(ICU_I18N icui18n)
    find_library(ICU_UC icuuc)

    if(CORE_FOUNDATION)
        target_link_libraries(fashion_search_engine ${CORE_FOUNDATION})
    endif()
    if(SECURITY)
        target_link_libraries(fashion_search_engine ${SECURITY})
    endif()
    if(ICU_I18N AND ICU_UC)
        target_link_libraries(fashion_search_engine ${ICU_I18N} ${ICU_UC})
    endif()
endif()

# Потоки
find_package(Threads REQUIRED)
target_link_libraries(fashion_search_engine Threads::Threads)

# Динамическое связывание для больших структур
if(UNIX AND NOT APPLE)
    target_link_options(fashion_search_engine PRIVATE "-Wl,--allow-multiple-definition")
endif()