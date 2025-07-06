#include <NavDataManager/NavDataManager.h>
#include <NavDataManager/AirportQuery.h>
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
    void insert_airports(const std::vector<AirportMeta>& airports, bool is_custom_scenery, std::unordered_set<std::string>& airports_in_transaction);
    void insert_runways(const std::vector<RunwayData>& runways);
    void insert_taxiway_nodes(const std::vector<TaxiwayNodeData>& taxiway_nodes);
    void insert_taxiway_edges(const std::vector<TaxiwayEdgeData>& taxiway_edges);
    void insert_linear_features(const std::vector<LinearFeatureData>& linear_features);
    void insert_linear_feature_nodes(const std::vector<LinearFeatureNodeData>& linear_feature_nodes);
    
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
        m_impl->m_db->exec("PRAGMA journal_mode = WAL");           // Write-Ahead Logging for better performance
        m_impl->m_db->exec("PRAGMA synchronous = NORMAL");        // Balance safety vs performance
        m_impl->m_db->exec("PRAGMA cache_size = 10000");          // 10MB cache
        m_impl->m_db->exec("PRAGMA temp_store = memory");         // Store temp data in memory
        m_impl->m_db->exec("PRAGMA mmap_size = 268435456");       // 256MB memory-mapped I/O
        m_impl->m_db->exec("PRAGMA auto_vacuum = INCREMENTAL");   // Automatically reclaim space
        m_impl->m_db->exec("PRAGMA page_size = 4096");            // Optimize page size

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
            insert_airports(parsed_data.airports, is_custom_scenery, airports_in_transaction);
            insert_runways(parsed_data.runways);
            insert_taxiway_nodes(parsed_data.taxiway_nodes);
            insert_taxiway_edges(parsed_data.taxiway_edges);
            insert_linear_features(parsed_data.linear_features);
            insert_linear_feature_nodes(parsed_data.linear_feature_nodes);
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

void NavDataManager::Impl::insert_airports(const std::vector<AirportMeta>& airports, bool is_custom_scenery, std::unordered_set<std::string>& airports_in_transaction) {
    // Helper function to get or create lookup table IDs
    auto get_or_create_country_id = [this](const std::string& country_name) -> int {
        // First try to get existing
        SQLite::Statement select_stmt(*m_db, "SELECT country_id FROM countries WHERE country_name = ?");
        select_stmt.bind(1, country_name);
        if (select_stmt.executeStep()) {
            return select_stmt.getColumn(0).getInt();
        }
        
        // Create new
        SQLite::Statement insert_stmt(*m_db, "INSERT INTO countries (country_name) VALUES (?)");
        insert_stmt.bind(1, country_name);
        insert_stmt.executeStep();
        return static_cast<int>(m_db->getLastInsertRowid());
    };
    
    auto get_or_create_region_id = [this](const std::string& region_code) -> int {
        SQLite::Statement select_stmt(*m_db, "SELECT region_id FROM regions WHERE region_code = ?");
        select_stmt.bind(1, region_code);
        if (select_stmt.executeStep()) {
            return select_stmt.getColumn(0).getInt();
        }
        
        SQLite::Statement insert_stmt(*m_db, "INSERT INTO regions (region_code) VALUES (?)");
        insert_stmt.bind(1, region_code);
        insert_stmt.executeStep();
        return static_cast<int>(m_db->getLastInsertRowid());
    };
    
    auto get_or_create_state_id = [this](const std::string& state_name, int country_id) -> int {
        SQLite::Statement select_stmt(*m_db, "SELECT state_id FROM states WHERE state_name = ? AND country_id = ?");
        select_stmt.bind(1, state_name);
        select_stmt.bind(2, country_id);
        if (select_stmt.executeStep()) {
            return select_stmt.getColumn(0).getInt();
        }
        
        SQLite::Statement insert_stmt(*m_db, "INSERT INTO states (state_name, country_id) VALUES (?, ?)");
        insert_stmt.bind(1, state_name);
        insert_stmt.bind(2, country_id);
        insert_stmt.executeStep();
        return static_cast<int>(m_db->getLastInsertRowid());
    };
    
    auto get_or_create_city_id = [this](const std::string& city_name, int state_id, int country_id) -> int {
        SQLite::Statement select_stmt(*m_db, "SELECT city_id FROM cities WHERE city_name = ? AND state_id = ? AND country_id = ?");
        select_stmt.bind(1, city_name);
        select_stmt.bind(2, state_id);
        select_stmt.bind(3, country_id);
        if (select_stmt.executeStep()) {
            return select_stmt.getColumn(0).getInt();
        }
        
        SQLite::Statement insert_stmt(*m_db, "INSERT INTO cities (city_name, state_id, country_id) VALUES (?, ?, ?)");
        insert_stmt.bind(1, city_name);
        insert_stmt.bind(2, state_id);
        insert_stmt.bind(3, country_id);
        insert_stmt.executeStep();
        return static_cast<int>(m_db->getLastInsertRowid());
    };

    // Prepare statements
    SQLite::Statement check_stmt(*m_db, "SELECT icao FROM airports WHERE icao = ?");
    SQLite::Statement delete_stmt(*m_db, "DELETE FROM airports WHERE icao = ?");
    SQLite::Statement airport_stmt(*m_db, R"(
        INSERT OR REPLACE INTO airports
        (icao, iata, faa, airport_name, elevation, type, latitude, longitude, 
         country_id, state_id, city_id, region_id, transition_alt, transition_level)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    // DEBUG: Log what airports are in this file
    if (m_logging_enabled && is_custom_scenery) {
        std::cout << "  -> Custom scenery file contains " << airports.size() << " airports: ";
        for (const auto& airport : airports) {
            if (airport.icao && !airport.icao->empty()) {
                std::cout << *airport.icao << " ";
            } else {
                std::cout << "[NULL/EMPTY] ";
            }
        }
        std::cout << std::endl;
    }
    
    // Process each airport individually
    for (const auto& airport : airports) {
        // Skip airports without ICAO (required field)
        if (!airport.icao || airport.icao->empty()) {
            continue;
        }

        // TRIM THE ICAO CODE TO REMOVE HIDDEN CHARACTERS
        std::string clean_icao = *airport.icao;
        // Remove whitespace, carriage returns, newlines from both ends
        clean_icao.erase(0, clean_icao.find_first_not_of(" \t\r\n"));
        clean_icao.erase(clean_icao.find_last_not_of(" \t\r\n") + 1);

        if (is_custom_scenery) {
            // Use cleaned ICAO for database operations
            check_stmt.bind(1, clean_icao);
            bool airport_exists_in_db = check_stmt.executeStep();
            check_stmt.reset();

            bool airport_exists = airport_exists_in_db || airports_in_transaction.count(clean_icao) > 0;

            if (airport_exists) {
                delete_stmt.bind(1, clean_icao);
                delete_stmt.executeStep();
                delete_stmt.reset();

                if (m_logging_enabled) {
                    std::cout << "  -> Replacing existing airport: " << clean_icao << std::endl;
                }
            }
        }
        
        // Resolve foreign key IDs
        std::optional<int> country_id, state_id, city_id, region_id;
        
        // Get country ID if country exists
        if (airport.country && !airport.country->empty()) {
            country_id = get_or_create_country_id(*airport.country);
        }
        
        // Get region ID if region exists
        if (airport.region && !airport.region->empty()) {
            region_id = get_or_create_region_id(*airport.region);
        }
        
        // Get state ID if state exists (requires country)
        if (airport.state && !airport.state->empty() && country_id) {
            state_id = get_or_create_state_id(*airport.state, *country_id);
        }
        
        // Get city ID if city exists (requires state and country)
        if (airport.city && !airport.city->empty() && state_id && country_id) {
            city_id = get_or_create_city_id(*airport.city, *state_id, *country_id);
        }
        
        // Bind airport data
        airport.icao ? airport_stmt.bind(1, *airport.icao) : airport_stmt.bind(1);
        airport.iata ? airport_stmt.bind(2, *airport.iata) : airport_stmt.bind(2);
        airport.faa ? airport_stmt.bind(3, *airport.faa) : airport_stmt.bind(3);
        airport.airport_name ? airport_stmt.bind(4, *airport.airport_name) : airport_stmt.bind(4);
        airport.elevation ? airport_stmt.bind(5, *airport.elevation) : airport_stmt.bind(5);
        airport.type ? airport_stmt.bind(6, *airport.type) : airport_stmt.bind(6);
        airport.latitude ? airport_stmt.bind(7, *airport.latitude) : airport_stmt.bind(7);
        airport.longitude ? airport_stmt.bind(8, *airport.longitude) : airport_stmt.bind(8);
        
        // Bind foreign key IDs
        country_id ? airport_stmt.bind(9, *country_id) : airport_stmt.bind(9);
        state_id ? airport_stmt.bind(10, *state_id) : airport_stmt.bind(10);
        city_id ? airport_stmt.bind(11, *city_id) : airport_stmt.bind(11);
        region_id ? airport_stmt.bind(12, *region_id) : airport_stmt.bind(12);
        
        // Bind transition fields
        airport.transition_alt ? airport_stmt.bind(13, *airport.transition_alt) : airport_stmt.bind(13);
        airport.transition_level ? airport_stmt.bind(14, *airport.transition_level) : airport_stmt.bind(14);
        
        airport_stmt.executeStep();
        airport_stmt.reset();

        // Track this airport as inserted in the current transaction
        airports_in_transaction.insert(*airport.icao);
    }
}

void NavDataManager::Impl::insert_runways(const std::vector<RunwayData>& runways) {
    SQLite::Statement stmt(*m_db, R"(
        INSERT OR REPLACE INTO runways
        (airport_icao, width, surface, end1_rw_number, end1_lat, end1_lon, end1_d_threshold, end1_rw_marking_code, end1_rw_app_light_code, 
         end2_rw_number, end2_lat, end2_lon, end2_d_threshold, end2_rw_marking_code, end2_rw_app_light_code)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    for (const auto& runway: runways) {
        runway.airport_icao ? stmt.bind(1, *runway.airport_icao) : stmt.bind(1);
        runway.width ? stmt.bind(2, *runway.width) : stmt.bind(2);
        runway.surface ? stmt.bind(3, *runway.surface) : stmt.bind(3);
        runway.end1_rw_number ? stmt.bind(4, *runway.end1_rw_number) : stmt.bind(4);
        runway.end1_lat ? stmt.bind(5, *runway.end1_lat) : stmt.bind(5);
        runway.end1_lon ? stmt.bind(6, *runway.end1_lon) : stmt.bind(6);
        runway.end1_d_threshold ? stmt.bind(7, *runway.end1_d_threshold) : stmt.bind(7);
        runway.end1_rw_marking_code ? stmt.bind(8, *runway.end1_rw_marking_code) : stmt.bind(8);
        runway.end1_rw_app_light_code ? stmt.bind(9, *runway.end1_rw_app_light_code) : stmt.bind(9);
        runway.end2_rw_number ? stmt.bind(10, *runway.end2_rw_number) : stmt.bind(10);
        runway.end2_lat ? stmt.bind(11, *runway.end2_lat) : stmt.bind(11);
        runway.end2_lon ? stmt.bind(12, *runway.end2_lon) : stmt.bind(12);
        runway.end2_d_threshold ? stmt.bind(13, *runway.end2_d_threshold) : stmt.bind(13);
        runway.end2_rw_marking_code ? stmt.bind(14, *runway.end2_rw_marking_code) : stmt.bind(14);
        runway.end2_rw_app_light_code ? stmt.bind(15, *runway.end2_rw_app_light_code) : stmt.bind(15);

        stmt.executeStep();
        stmt.reset();
    }
}

void NavDataManager::Impl::insert_taxiway_nodes(const std::vector<TaxiwayNodeData>& taxiway_nodes) {
    SQLite::Statement stmt(*m_db, R"(
        INSERT OR REPLACE INTO taxi_nodes
        (node_id, airport_icao, latitude, longitude, node_type)
        VALUES (?, ?, ?, ?, ?)
    )");

    for (const auto& taxi_node : taxiway_nodes) {
        taxi_node.node_id ? stmt.bind(1, *taxi_node.node_id) : stmt.bind(1);
        taxi_node.airport_icao ? stmt.bind(2, *taxi_node.airport_icao) : stmt.bind(2);
        taxi_node.latitude ? stmt.bind(3, *taxi_node.latitude) : stmt.bind(3);
        taxi_node.longitude ? stmt.bind(4, *taxi_node.longitude) : stmt.bind(4);
        taxi_node.node_type ? stmt.bind(5, *taxi_node.node_type) : stmt.bind(5);

        stmt.executeStep();
        stmt.reset();
    }
}

void NavDataManager::Impl::insert_taxiway_edges(const std::vector<TaxiwayEdgeData>& taxiway_edges) {
    SQLite::Statement stmt(*m_db, R"(
        INSERT OR REPLACE INTO taxi_edges
        (airport_icao, start_node_id, end_node_id, is_two_way, taxiway_name, width_class)
        VALUES (?, ?, ?, ?, ?, ?)
    )");

    for (const auto& taxi_edge : taxiway_edges) {
        taxi_edge.airport_icao ? stmt.bind(1, *taxi_edge.airport_icao) : stmt.bind(1);
        taxi_edge.start_node_id ? stmt.bind(2, *taxi_edge.start_node_id) : stmt.bind(2);
        taxi_edge.end_node_id ? stmt.bind(3, *taxi_edge.end_node_id) : stmt.bind(3);
        taxi_edge.is_two_way ? stmt.bind(4, *taxi_edge.is_two_way) : stmt.bind(4);
        taxi_edge.taxiway_name ? stmt.bind(5, *taxi_edge.taxiway_name) : stmt.bind(5);
        taxi_edge.width_class ? stmt.bind(6, *taxi_edge.width_class) : stmt.bind(6);

        stmt.executeStep();
        stmt.reset();
    }
}

void NavDataManager::Impl::insert_linear_features(const std::vector<LinearFeatureData>& linear_features) {
    SQLite::Statement stmt(*m_db, R"(
        INSERT OR REPLACE INTO linear_features
        (airport_icao, feature_sequence, line_type)
        VALUES(?, ?, ?)
    )");

    for (const auto& feature : linear_features) {
        feature.airport_icao ? stmt.bind(1, *feature.airport_icao) : stmt.bind(1);
        feature.feature_sequence ? stmt.bind(2, *feature.feature_sequence) : stmt.bind(2);
        feature.line_type ? stmt.bind(3, *feature.line_type) : stmt.bind(3);

        stmt.executeStep();
        stmt.reset();
    }
}

void NavDataManager::Impl::insert_linear_feature_nodes(const std::vector<LinearFeatureNodeData>& linear_feature_nodes) {
    SQLite::Statement stmt(*m_db, R"(
        INSERT OR REPLACE INTO linear_feature_nodes
        (airport_icao, feature_sequence, latitude, longitude, bezier_latitude, bezier_longitude, node_order)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )");

    for (const auto& node : linear_feature_nodes) {
        node.airport_icao ? stmt.bind(1, *node.airport_icao) : stmt.bind(1);
        node.feature_sequence ? stmt.bind(2, *node.feature_sequence) : stmt.bind(2);
        node.latitude ? stmt.bind(3, *node.latitude) : stmt.bind(3);
        node.longitude ? stmt.bind(4, *node.longitude) : stmt.bind(4);
        node.bezier_latitude ? stmt.bind(5, *node.bezier_latitude) : stmt.bind(5);
        node.bezier_longitude ? stmt.bind(6, *node.bezier_longitude) : stmt.bind(6);
        node.node_order ? stmt.bind(7, *node.node_order) : stmt.bind(7);

        stmt.executeStep();
        stmt.reset();
    }
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