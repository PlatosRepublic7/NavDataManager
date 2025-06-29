#include <NavDataManager/NavDataManager.h>
#include "XPlaneDatParser.h"
#include <iostream>
#include <vector>
#include <filesystem>
#include <string>
#include <algorithm>
#include <sqlite3.h>
#include <SQLiteCpp/Transaction.h>
#include <SQLiteCpp/Database.h>


namespace fs = std::filesystem;

struct NavDataManager::Impl {
    std::unique_ptr<SQLite::Database> m_db;
    std::string m_data_directory;
    std::string m_xp_directory;
    fs::path m_global_airport_data_path;
    fs::path m_custom_scenery_path;
    bool m_logging_enabled;
    std::vector<fs::path> m_all_apt_files;
    std::unique_ptr<XPlaneDatParser> m_parser;

    Impl(const std::string& xp_root_path, bool logging)
        : m_xp_directory(xp_root_path), m_logging_enabled(logging), m_db(nullptr),
        m_parser(std::make_unique<XPlaneDatParser>(logging)) {}

    void get_airport_dat_paths(const std::string& xp_dir);
    void create_tables();
    void parse_all_dat_files();
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

void NavDataManager::generate_database(const std::string& db_path) {
    try {
        m_impl->m_db = std::make_unique<SQLite::Database>(
            db_path,
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
        );

        // Create tables after opening the database
        m_impl->create_tables();

        if (m_impl->m_logging_enabled) {
            std::cout << "Database created at: " << db_path << std::endl;
        }
    } catch (const SQLite::Exception& e) {
        std::cerr << "Error creating database: " << e.what() << std::endl;
        throw;
    }

    // Here is where the parsing logic will begin
}

void NavDataManager::parse_all_dat_files() {
    m_impl->parse_all_dat_files();
}

void NavDataManager::Impl::parse_all_dat_files() {
    // Perhaps it is best to open a transaction here, that way we make database commits more efficient
    try {
        SQLite::Transaction transaction(*m_db);

        for (const auto& file: m_all_apt_files) {
            m_parser->parse_airport_dat(file, m_db.get());
        }
        // Handle other file types...

        // Close the transaction and commit if everything succeeded
        transaction.commit();
    } catch (const std::exception& e) {
        std::cerr << "Error during parsing: " << e.what() << std::endl;
        throw;
    }
}

// ------ Implementation of Impl methods -----

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
}

void NavDataManager::Impl::create_tables() {
    try {
        m_db->exec(R"sql(
            CREATE TABLE IF NOT EXISTS airports (
                icao TEXT PRIMARY KEY,
                iata TEXT,
                faa TEXT,
                airport_name TEXT,
                elevation INTEGER,
                type TEXT,
                latitude REAL,
                longitude REAL,
                country TEXT,
                city TEXT,
                region TEXT,
                transition_alt INTEGER,
                transition_level INTEGER
            );
            CREATE TABLE IF NOT EXISTS runways (
                runway_id INTEGER PRIMARY KEY AUTOINCREMENT,
                airport_icao TEXT,
                width REAL,
                surface INTEGER,
                end1_rw_number TEXT NOT NULL,
                end1_lat REAL,
                end1_lon REAL,
                end1_d_threshold REAL,
                end1_rw_marking_code INTEGER,
                end1_rw_app_light_code INTEGER,
                end2_rw_number TEXT NOT NULL,
                end2_lat REAL,
                end2_lon REAL,
                end2_d_threshold REAL,
                end2_rw_marking_code INTEGER,
                end2_rw_app_light_code INTEGER,

                UNIQUE (airport_icao, end1_rw_number, end2_rw_number),
                FOREIGN KEY (airport_icao) REFERENCES airports (icao)
            );
        )sql");
    } catch (const SQLite::Exception& e) {
        std::cerr << "Error creating tables: " << e.what() << std::endl;
    }
}