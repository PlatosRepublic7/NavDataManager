#pragma once
#include "Types.h"
#include <vector>
#include <optional>
#include <string>

// Forward declaration
namespace SQLite { class Database; }

class RunwayQueryBuilder {
    public:
        explicit RunwayQueryBuilder(SQLite::Database* db) : m_db(db) {}

        // Builder methods
        RunwayQueryBuilder& airport_icao(const std::string& icao) { airport_filter = icao; return *this; }
        RunwayQueryBuilder& surface_type(int surface) { surface_filter = surface; return *this; }
        RunwayQueryBuilder& min_width(double width) { min_width_filter = width; return *this; }
        RunwayQueryBuilder& runway_number(const std::string& number) { runway_number_filter = number; return *this; }
        RunwayQueryBuilder& max_results(int max) { limit = max; return *this; }

        // Terminal methods
        std::vector<RunwayData> execute();
        std::optional<RunwayData> first();
        size_t count();

    private:
        SQLite::Database* m_db = nullptr;

        // Query Parameters
        std::optional<std::string> airport_filter;
        std::optional<int> surface_filter;
        std::optional<double> min_width_filter;
        std::optional<std::string> runway_number_filter;
        int limit = 100;
        bool sort_by_icao = true;
};

class AirportQueryBuilder {
    public:
        explicit AirportQueryBuilder(SQLite::Database* db) : m_db(db) {}

        // Builder methods - return *this for chaining
        AirportQueryBuilder& icao(const std::string& filter) { icao_filter = filter; return *this; }
        AirportQueryBuilder& country(const std::string& filter) { country_filter = filter; return *this; }
        AirportQueryBuilder& city(const std::string& filter) { city_filter = filter; return *this; }
        AirportQueryBuilder& state(const std::string& filter) { state_filter = filter; return *this; }
        AirportQueryBuilder& type(const std::string& filter) { type_filter = filter; return *this; }
        AirportQueryBuilder& elevation_range(int min_ft, int max_ft) { 
            min_elevation = min_ft; 
            max_elevation = max_ft; 
            return *this; 
        }
        AirportQueryBuilder& max_results(int max) { limit = max; return *this; }
        AirportQueryBuilder& order_by_icao(bool order = true) { sort_by_icao = order; return *this; }
        AirportQueryBuilder& near(double lat, double lon, double radius_km) {
            latitude = lat;
            longitude = lon;
            radius = radius_km;
            return *this;
        }

        // Terminal methods - execute the query
        std::vector<AirportMeta> execute();
        std::optional<AirportMeta> first();
        size_t count();

    private:
        SQLite::Database* m_db = nullptr;
        
        // Query parameters
        std::optional<std::string> icao_filter;
        std::optional<std::string> country_filter;
        std::optional<std::string> city_filter;
        std::optional<std::string> state_filter;
        std::optional<std::string> type_filter;
        std::optional<int> min_elevation;
        std::optional<int> max_elevation;
        std::optional<double> latitude;
        std::optional<double> longitude;
        std::optional<double> radius;
        int limit = 100;
        bool sort_by_icao = true;
};

class AirportQuery {
    public:
        explicit AirportQuery(SQLite::Database* db) : m_db(db) {}

        // ===== AIRPORT QUERIES =====
        AirportQueryBuilder airports() { return AirportQueryBuilder(m_db); }

        // Convenience methods for airport metadata
        std::optional<AirportMeta> get_by_icao(const std::string& icao) {
            return airports().icao(icao).first();
        }

        std::vector<AirportMeta> get_by_country(const std::string& country, int limit = 100) {
            return airports().country(country).max_results(limit).execute();
        }

        std::vector<AirportMeta> get_by_state(const std::string& state, int limit = 100) {
            return airports().state(state).max_results(limit).execute();
        }

        std::vector<AirportMeta> get_near(double lat, double lon, double radius_km, int limit = 50) {
            return airports().near(lat, lon, radius_km).max_results(limit).execute();
        }

        // ===== RUNWAY QUERIES =====
        RunwayQueryBuilder runways() { return RunwayQueryBuilder(m_db); }

        // Convenience methods for runway data
        std::vector<RunwayData> get_runways_for_airport(const std::string& icao) {
            return runways().airport_icao(icao).execute();
        }

        std::vector<RunwayData> get_runways_by_surface(int surface_type, int limit = 50) {
            return runways().surface_type(surface_type).max_results(limit).execute();
        }

        // ===== COMBINED QUERIES =====
        struct AirportDetail {
            AirportMeta airport;
            std::vector<RunwayData> runways;
        };

        // std::optional<AirportDetail> get_complete_airport_data(const std::string& icao) {}

    private:
        SQLite::Database* m_db;  
};