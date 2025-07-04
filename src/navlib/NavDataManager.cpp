#include <NavDataManager/NavDataManager.h>
#include <NavDataManager/AirportQuery.h>
#include "XPlaneDatParser.h"
#include "schema.h"
#include <iostream>
#include <vector>
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

    void get_airport_dat_paths(const std::string& xp_dir);
    void apply_schema();
    void parse_all_dat_files();
    void insert_airports(const std::vector<AirportMeta>& airports);
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

void NavDataManager::parse_all_dat_files() {
    m_impl->parse_all_dat_files();
}

// ------ Implementation of Impl methods -----

void NavDataManager::Impl::parse_all_dat_files() {
    // Perhaps it is best to open a transaction here, that way we make database commits more efficient
    try {
        SQLite::Transaction transaction(*m_db);
        if (m_logging_enabled) {
            std::cout << "Parsing apt.dat files..." << std::endl;
        }
        int curr_file_num = 0;
        auto begin_time = std::chrono::steady_clock::now();
        auto total_insertion_time = std::chrono::seconds::zero();
        for (const auto& file: m_all_apt_files) {
            if (m_logging_enabled) {
                curr_file_num++;
                std::cout << "(" << curr_file_num << "/" << m_all_apt_files.size() << ")..." << std::endl;
            }
            
            ParsedAptData parsed_data = m_parser->parse_airport_dat(file);
            auto begin_insertion_time = std::chrono::steady_clock::now();
            insert_airports(parsed_data.airports);
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
            for (int i = 0; i < 50; ++i) {
                std::cout << "-";
            }
            std::cout << std::endl;
        }
        // Close the transaction and commit if everything succeeded
        transaction.commit();
    } catch (const std::exception& e) {
        throw;
    }
}

void NavDataManager::Impl::insert_airports(const std::vector<AirportMeta>& airports) {
    SQLite::Statement stmt(*m_db, R"(
        INSERT OR REPLACE INTO airports
        (icao, iata, faa, airport_name, elevation, type, latitude, longitude, country, city, state, region, transition_alt, transition_level)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    for (const auto& airport : airports) {
        airport.icao ? stmt.bind(1, *airport.icao) : stmt.bind(1);
        airport.iata ? stmt.bind(2, *airport.iata) : stmt.bind(2);
        airport.faa ? stmt.bind(3, *airport.faa) : stmt.bind(3);
        airport.airport_name ? stmt.bind(4, *airport.airport_name) : stmt.bind(4);
        airport.elevation ? stmt.bind(5, *airport.elevation) : stmt.bind(5);
        airport.type ? stmt.bind(6, *airport.type) : stmt.bind(6);
        airport.latitude ? stmt.bind(7, *airport.latitude): stmt.bind(7);
        airport.longitude ? stmt.bind(8, *airport.longitude) : stmt.bind(8);
        airport.country ? stmt.bind(9, *airport.country) : stmt.bind(9);
        airport.city ? stmt.bind(10, *airport.city) : stmt.bind(10);
        airport.state ? stmt.bind(11, *airport.state) : stmt.bind(11);
        airport.region ? stmt.bind(12, *airport.region) : stmt.bind(12);
        airport.transition_alt ? stmt.bind(13, *airport.transition_alt) : stmt.bind(13);
        airport.transition_level ? stmt.bind(14, *airport.transition_level) : stmt.bind(14);

        stmt.executeStep();
        stmt.reset();
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

        // Cast provided string to a path
        fs::path xp_dir_path(xp_dir);
        if (!fs::exists(xp_dir_path) || !fs::is_directory(xp_dir_path)) {
            throw std::invalid_argument("Provided path is not a valid directory: " + xp_dir);
        }

        // Loop through the main two directories and find all apt.dat files
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
                // Create a recursive iterator to walk the directory tree
                auto iterator = fs::recursive_directory_iterator(scenery_dir);
                for (const auto& entry: iterator) {
                    if (entry.is_directory() && !in_global_scenery) {
                        std::string dir_name = entry.path().filename().string();

                        // Convert directory name to lowercase and check against exclusions
                        std::transform(dir_name.begin(), dir_name.end(), dir_name.begin(), [](unsigned char c){return std::tolower(c);});
                        for (const auto& pattern : exclusion_patterns) {
                            if (dir_name.find(pattern) != std::string::npos) {
                                iterator.disable_recursion_pending();
                                break;
                            }
                        }
                    }

                    // If the entry was not skipped, check if it's our target
                    if (entry.is_regular_file() && entry.path().filename() == "apt.dat") {
                        m_all_apt_files.push_back(entry.path());
                        if (m_logging_enabled) {
                            std::cout << "  -> Located File: " << entry.path().string() << std::endl;
                        }
                    }
                }
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Filesystem error while scanning: " << e.what() << std::endl;
                throw;
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