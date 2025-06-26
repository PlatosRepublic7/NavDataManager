#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <SQLiteCpp/Database.h>
#include "XPlaneDatParser.h"

namespace fs = std::filesystem;

class NavDataManager {
    public:
        /**
         * @brief Constructs a NavDataManager.
         * @param xp_root_path Path to the X-Plane root.
         * @param logging Enable/Disable logging to standard output.
         */
        NavDataManager(const std::string& xp_root_path, bool logging=false);

        /**
         * @brief Scans and collects all needed .dat files within the X-Plane installation.
         * @throws fs::filesystem_error or std::exception if installation does not contain global apt.dat or Custom Scenery directory can not be found.
         */
        void scanXP();

        /**
        * @brief Generates/updates the navigation database and parses all found .dat files.
        * @param db_path Path to the SQLite database file.
        * @throws SQLite::Exception if the database cannot be created.
        * @note This method will potentially take some time to complete.
        */
        void generateDatabase(const std::string& db_path);

        void parseAllDatFiles();

    private:
        std::unique_ptr<SQLite::Database> m_db;
        std::string m_dataDirectory;
        std::string m_xpDirectory;
        fs::path m_globalAirportDataPath;
        fs::path m_customSceneryPath;
        bool m_loggingEnabled;
        std::vector<fs::path> m_allAptFiles;
        std::unique_ptr<XPlaneDatParser> m_parser;

        void getAirportDatPaths(const std::string& xp_dir);
        void createTables();
};