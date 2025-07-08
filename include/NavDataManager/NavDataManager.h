#pragma once
#include <string>
#include <memory>

class AirportQuery;

class NavDataManager {
    public:
        /**
         * @brief Constructs a NavDataManager.
         * @param xp_root_path Path to the X-Plane root.
         * @param logging Enable/Disable logging to standard output.
         */
        NavDataManager(const std::string& xp_root_path, bool logging=false);
        ~NavDataManager();

        NavDataManager(const NavDataManager&) = delete;
        NavDataManager& operator=(const NavDataManager&) = delete;
        NavDataManager(NavDataManager&&) noexcept = default;
        NavDataManager& operator=(NavDataManager&&) noexcept = default;

        /**
         * @brief Scans and collects all needed .dat files within the X-Plane installation.
         * @throws fs::filesystem_error or std::exception if installation does not contain global apt.dat or Custom Scenery directory can not be found.
         */
        void scan_xp();

        /**
        * @brief Creates/Connects the navigation database.
        * @param db_path Path to the SQLite database file.
        * @throws SQLite::Exception if the database cannot be created or found.
        */
        void connect_database(const std::string& db_path);

        /**
         * @brief Parses all .dat files and performs database update.
         * @param force_full_parse Force system to parse all found files.
         * @note First time parsing of data will take upwards of 45 seconds to complete. It is an expensive operation.
         */
        void parse_all_dat_files(bool force_full_parse=false);

        AirportQuery& airport_data();

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
};