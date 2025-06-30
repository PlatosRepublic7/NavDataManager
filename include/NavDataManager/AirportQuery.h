#pragma once
#include "Types.h"
#include <vector>
#include <optional>
#include <string>

// Forward declaration
namespace SQLite { class Database; }

class AirportQueryBuilder {
    public:
        // Builder methods - return *this for chaining
        AirportQueryBuilder& icao(const std::string& filter) { icao_filter = filter; return *this; }
        AirportQueryBuilder& country(const std::string& filter) { country_filter = filter; return *this; }
        AirportQueryBuilder& city(const std::string& filter) { city_filter = filter; return *this; }
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
        friend class AirportQuery;
        SQLite::Database* m_db = nullptr;
        
        // Query parameters
        std::optional<std::string> icao_filter;
        std::optional<std::string> country_filter;
        std::optional<std::string> city_filter;
        std::optional<std::string> type_filter;
        std::optional<int> min_elevation;
        std::optional<int> max_elevation;
        std::optional<double> latitude;
        std::optional<double> longitude;
        std::optional<double> radius;
        int limit = 100;
        bool sort_by_icao = true;

        AirportQueryBuilder(SQLite::Database* db) : m_db(db) {}
};

class AirportQuery {
    public:
        explicit AirportQuery(SQLite::Database* db) : m_db(db) {}

        // Start a fluent query
        AirportQueryBuilder query() { return AirportQueryBuilder(m_db); }

        // Convenience methods for common queries
        std::optional<AirportMeta> get_by_icao(const std::string& icao) {
            return query().icao(icao).first();
        }

        std::vector<AirportMeta> get_by_country(const std::string& country, int limit = 100) {
            return query().country(country).max_results(limit).execute();
        }

        std::vector<AirportMeta> get_near(double lat, double lon, double radius_km, int limit = 50) {
            return query().near(lat, lon, radius_km).max_results(limit).execute();
        }

    private:
        friend class NavDataManager;
        SQLite::Database* m_db;  
};