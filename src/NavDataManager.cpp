#include "NavDataManager.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <sqlite3.h>

namespace fs = std::filesystem;

// Constructor
NavDataManager::NavDataManager(const std::string& db_directory) : m_db(db_directory, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {
    std::cout << "Database initialized at: " << db_directory << std::endl;
    m_dataDirectory = db_directory;
    createTables();
}

void NavDataManager::createTables() {
    try {
        m_db.exec(R"sql(
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

void NavDataManager::updateDatabase(const std::string& xp_root_path) {
    m_xpDirectory = xp_root_path;
    m_airportDataPath = fs::path(m_xpDirectory) / "Global Scenery" / "Global Airports" / "Earth nav data";

    if (!fs::exists(m_airportDataPath)) {
        std::cerr << "Global Airport directory does not exist: " << m_airportDataPath << std::endl;
        return;
    }

    // Here you would implement the logic to parse files and populate the database
    // For now, we just print the directory
    std::cout << "Updating database with data from: " << m_airportDataPath << std::endl;

    // Example: Iterate through files in the data directory
    // for (const auto& entry : fs::directory_iterator(m_airportDataPath)) {
    //     if (entry.is_regular_file()) {
    //         std::cout << "Found file: " << entry.path() << std::endl;
    //         // Parse and insert data into the database as needed
    //     }
    // }
}