#include <NavDataManager/NavDataManager.h>
#include <NavDataManager/AirportQuery.h>
#include "ParserQuery.h"
#include "XPlaneDatParser.h"
#include "schema.h"
#include <iostream>
#include <vector>
#include <unordered_set>
#include <filesystem>
#include <string>
#include <algorithm>
#include <chrono>
#include <sqlite3.h>
#include <SQLiteCpp/Transaction.h>
#include <SQLiteCpp/Database.h>


namespace fs = std::filesystem;

struct NavDataManager::Impl {
    std::string m_data_directory;
    std::string m_xp_directory;
    fs::path m_global_airport_data_path;
    fs::path m_custom_scenery_path;
    bool m_logging_enabled;
    std::unique_ptr<SQLite::Database> m_db;
    std::vector<fs::path> m_all_apt_files;
    std::unique_ptr<XPlaneDatParser> m_parser;
    std::unique_ptr<AirportQuery> airport_query;

    Impl(const std::string& xp_root_path, bool logging)
        : m_xp_directory(xp_root_path), m_logging_enabled(logging), m_db(nullptr),
        m_parser(std::make_unique<XPlaneDatParser>(logging)) {}

    void optimize_database();

    void get_airport_dat_paths(const std::string& xp_dir);
    void apply_schema();
    void parse_all_dat_files(bool force_full_parse);
    bool check_scenery_path_in_db(const fs::path& scenery_path);
    
    void initialize_queries() {
        airport_query = std::make_unique<AirportQuery>(m_db.get());
    }
};

// Constructor
NavDataManager::NavDataManager(const std::string& xp_root_path, bool logging) 
    : m_impl(std::make_unique<Impl>(xp_root_path, logging)) {}

NavDataManager::~NavDataManager() = default;

void NavDataManager::scan_xp() {
    try {
        m_impl->get_airport_dat_paths(m_impl->m_xp_directory);
        // TODO: Add getNavDataPaths and handle similarly
    } catch (const std::exception& e) {
        std::cerr << "NavDataManager scanning failed: " << e.what() << std::endl;
        throw;
    }
}

void NavDataManager::connect_database(const std::string& db_path) {
    try {
        m_impl->m_db = std::make_unique<SQLite::Database>(
            db_path,
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
        );

        // Enable compression and optimization PRAGMAs
        m_impl->m_db->exec("PRAGMA journal_mode = WAL");          // Write-Ahead Logging for better performance
        m_impl->m_db->exec("PRAGMA synchronous = NORMAL");        // Balance safety vs performance
        m_impl->m_db->exec("PRAGMA cache_size = 20000");          // 20MB cache
        m_impl->m_db->exec("PRAGMA temp_store = memory");         // Store temp data in memory
        m_impl->m_db->exec("PRAGMA mmap_size = 268435456");       // 256MB memory-mapped I/O
        m_impl->m_db->exec("PRAGMA auto_vacuum = INCREMENTAL");   // Automatically reclaim space
        m_impl->m_db->exec("PRAGMA page_size = 65536");           // Optimize page size

        // Create tables after opening the database
        m_impl->apply_schema();

        m_impl->initialize_queries();

        if (m_impl->m_logging_enabled) {
            std::cout << "Database created at: " << db_path << std::endl;
            for (int i = 0; i < 50; ++i) {
                std::cout << "-";
            }
            std::cout << std::endl;
        }
    } catch (const SQLite::Exception& e) {
        std::cerr << "Error creating database: " << e.what() << std::endl;
        throw;
    }
}

void NavDataManager::parse_all_dat_files(bool force_full_parse) {
    m_impl->parse_all_dat_files(force_full_parse);
}

// ------ Implementation of Impl methods -----
void NavDataManager::Impl::optimize_database() {
    if (m_logging_enabled) {
        std::cout << "Optimizing database..." << std::endl;
    }
    m_db->exec("ANALYZE");
    m_db->exec("VACUUM");
    m_db->exec("PRAGMA incremental_vacuum");

    if (m_logging_enabled) {
        std::cout << "Database optimization completed." << std::endl;
        for (int i = 0; i < 50; ++i) {
                std::cout << "-";
            }
        std::cout << std::endl;
    }
}


void NavDataManager::Impl::parse_all_dat_files(bool force_full_parse) {
    // Perhaps it is best to open a transaction here, that way we make database commits more efficient
    try {
        SQLite::Transaction transaction(*m_db);
        if (m_logging_enabled) {
            std::cout << "Preparing for parsing..." << std::endl;
        }

        // Initialize navdata_query object
        auto parser_query = std::make_unique<ParserQuery>(m_db.get(), m_logging_enabled);

        // Track airports inserted during this transaction
        std::unordered_set<std::string> airports_in_transaction;

        // Check files already in database
        int skipped_files = 0;
        std::vector<fs::path> files_to_parse;
        for (const auto& file : m_all_apt_files) {
            bool file_in_db = check_scenery_path_in_db(file);
            if (!file_in_db || force_full_parse) {
                files_to_parse.push_back(file);
            } else {
                skipped_files++;
            }
            if (m_logging_enabled) {
                if (force_full_parse) {
                    std::cout << "Force Full Parse Detected. File: [" << file.string() << "] added to parse queue." << std::endl;
                } else {
                    if (file_in_db) {
                        std::cout << "File: [" << file.string() << "] detected in database, skipping..." << std::endl;
                    } else {
                        std::cout << "Detected new file: [" << file.string() << "] added to parse queue." << std::endl;
                    }
                }
            }
        }

        if (m_logging_enabled) {
            std::cout << "Parsing apt.dat files..." << std::endl;
        }
        int curr_file_num = 0;
        auto begin_time = std::chrono::steady_clock::now();
        auto total_insertion_time = std::chrono::seconds::zero();

        // Parse all non-skipped files
        for (const auto& file: files_to_parse) {
            
            if (m_logging_enabled) {
                curr_file_num++;
                std::cout << "(" << curr_file_num << "/" << files_to_parse.size() << ")..." << std::endl;
            }
            
            ParsedAptData parsed_data = m_parser->parse_airport_dat(file);

            bool is_custom_scenery = file.string().find("Custom Scenery") != std::string::npos;
            auto begin_insertion_time = std::chrono::steady_clock::now();
            
            parser_query->insert_airports(parsed_data.airports, is_custom_scenery, airports_in_transaction);
            parser_query->insert_runways(parsed_data.runways);
            parser_query->insert_taxiway_nodes(parsed_data.taxiway_nodes);
            parser_query->insert_taxiway_edges(parsed_data.taxiway_edges);
            parser_query->insert_linear_features(parsed_data.linear_features);
            parser_query->insert_linear_feature_nodes(parsed_data.linear_feature_nodes);
            parser_query->insert_startup_locations(parsed_data.startup_locations);

            auto end_insertion_time = std::chrono::steady_clock::now();
            auto insertion_duration = std::chrono::duration_cast<std::chrono::seconds>(end_insertion_time - begin_insertion_time);
            total_insertion_time += insertion_duration;
        }
        // Handle other file types...
        auto end_time = std::chrono::steady_clock::now();
        if (m_logging_enabled) {
            auto parse_duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - begin_time);
            std::cout << "Parsing Completed in " << parse_duration.count() << " seconds." << std::endl;
            std::cout << "Total Insertion Time: " << total_insertion_time.count() << " seconds." << std::endl;
            std::cout << "Total Files Skipped: " << skipped_files << std::endl;
            for (int i = 0; i < 50; ++i) {
                std::cout << "-";
            }
            std::cout << std::endl;
        }
        // Close the transaction and commit if everything succeeded
        transaction.commit();

        // Optimize database
        optimize_database();

    } catch (const std::exception& e) {
        throw;
    }
}

bool NavDataManager::Impl::check_scenery_path_in_db(const fs::path& scenery_path) {
    SQLite::Statement select_stmt(*m_db, "SELECT * FROM scenery_paths WHERE scenery_path = ?");
    select_stmt.bind(1, scenery_path.string());
    
    // Check if it exists in the database
    if (select_stmt.executeStep()) return true;

    // If not, create a new record for the scenery path
    SQLite::Statement insert_stmt(*m_db, "INSERT INTO scenery_paths (scenery_path) VALUES (?)");
    insert_stmt.bind(1, scenery_path.string());
    insert_stmt.executeStep();
    return false;
}

// This method finds all apt.dat files within an X-Plane installation and assigns the paths to the
// required private member variables.
void NavDataManager::Impl::get_airport_dat_paths(const std::string& xp_dir) {
    try {
        bool in_global_scenery = false;
        std::vector<std::string> airport_scenery_extensions = {"Global Scenery", "Custom Scenery"};
        std::vector<std::string> exclusion_patterns = {"z_", "ortho", "zortho4xp_", "simheaven_", "x-plane landmarks", "uhd_", "hd_", "library"};

        fs::path xp_dir_path(xp_dir);
        if (!fs::exists(xp_dir_path) || !fs::is_directory(xp_dir_path)) {
            throw std::invalid_argument("Provided path is not a valid directory: " + xp_dir);
        }

        // Collect all apt.dat files first, then filter
        std::vector<fs::path> all_apt_files;

        for (const auto& scenery_folder : airport_scenery_extensions) {
            fs::path scenery_dir = xp_dir_path / scenery_folder;
            if (scenery_folder == "Global Scenery") {
                scenery_dir /= "Global Airports";
                scenery_dir /= "Earth nav data";
                m_global_airport_data_path = scenery_dir;
                in_global_scenery = true;
            } else {
                m_custom_scenery_path = scenery_dir;
                in_global_scenery = false;
            }

            if (m_logging_enabled) {
                std::cout << "Finding all apt.dat files..." << std::endl;
                std::cout << "Scanning " << scenery_dir.string() << "..." << std::endl;
            }

            try {
                auto iterator = fs::recursive_directory_iterator(scenery_dir);
                for (const auto& entry: iterator) {
                    if (entry.is_directory() && !in_global_scenery) {
                        std::string dir_name = entry.path().filename().string();
                        std::transform(dir_name.begin(), dir_name.end(), dir_name.begin(), [](unsigned char c){return std::tolower(c);});
                        for (const auto& pattern : exclusion_patterns) {
                            if (dir_name.find(pattern) != std::string::npos) {
                                iterator.disable_recursion_pending();
                                break;
                            }
                        }
                    }

                    if (entry.is_regular_file() && entry.path().filename() == "apt.dat") {
                        all_apt_files.push_back(entry.path());
                    }
                }
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Filesystem error while scanning: " << e.what() << std::endl;
                throw;
            }
        }

        // Filter: keep only the shortest path per scenery package
        std::map<std::string, fs::path> shortest_per_package;
        
        for (const auto& apt_file : all_apt_files) {
            // Extract scenery package name (directory right after Custom Scenery or Global Scenery)
            std::string package_name;
            bool found_scenery = false;
            
            for (auto it = apt_file.begin(); it != apt_file.end(); ++it) {
                if (found_scenery && std::next(it) != apt_file.end()) {
                    package_name = it->string();
                    break;
                }
                if (*it == "Custom Scenery" || *it == "Global Scenery") {
                    found_scenery = true;
                }
            }
            
            // If we haven't seen this package, or this path is shorter, keep it
            if (shortest_per_package.find(package_name) == shortest_per_package.end() ||
                apt_file.string().length() < shortest_per_package[package_name].string().length()) {
                
                if (shortest_per_package.find(package_name) != shortest_per_package.end() && m_logging_enabled) {
                    std::cout << "  -> Skipping subdirectory apt.dat: " << shortest_per_package[package_name].string() << std::endl;
                }
                shortest_per_package[package_name] = apt_file;
            } else if (m_logging_enabled) {
                std::cout << "  -> Skipping subdirectory apt.dat: " << apt_file.string() << std::endl;
            }
        }

        // Add the shortest paths to m_all_apt_files
        // First add Global Scenery files
        for (const auto& [package, path] : shortest_per_package) {
            // Check if this is a Global Scenery file
            if (path.string().find("Global Scenery") != std::string::npos) {
                m_all_apt_files.push_back(path);
                if (m_logging_enabled) {
                    std::cout << "  -> Located File: " << path.string() << std::endl;
                }
            }
        }

        // Then add Custom Scenery files
        for (const auto& [package, path] : shortest_per_package) {
            // Check if this is a Custom Scenery file
            if (path.string().find("Custom Scenery") != std::string::npos) {
                m_all_apt_files.push_back(path);
                if (m_logging_enabled) {
                    std::cout << "  -> Located File: " << path.string() << std::endl;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in get_airport_dat_paths: " << e.what() << std::endl;
        throw;
    }

    if (m_logging_enabled) {
        std::cout << "Found " << m_all_apt_files.size() << " apt.dat files." << std::endl;
        for (int i = 0; i < 50; ++i) {
            std::cout << "-";
        }
        std::cout << std::endl;
    }
}

void NavDataManager::Impl::apply_schema() {
    try {
        m_db->exec(navdata_schema);
    } catch (const SQLite::Exception& e) {
        std::cerr << "Error creating tables: " << e.what() << std::endl;
    }
}

AirportQuery& NavDataManager::airport_data() {
    if (!m_impl->airport_query) {
        throw std::runtime_error("Database not connected. Call connect_database() first.");
    }
    return *m_impl->airport_query;
}