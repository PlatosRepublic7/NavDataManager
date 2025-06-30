#include "NavDataManager/AirportQuery.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <sstream>
#include <algorithm>

std::vector<AirportMeta> AirportQueryBuilder::execute() {
    std::vector<AirportMeta> results;
    
    // Build dynamic query
    std::ostringstream query;
    query << "SELECT icao, iata, faa, airport_name, elevation, type, "
          << "latitude, longitude, country, city, region, "
          << "transition_alt, transition_level FROM airports";
    
    std::vector<std::string> conditions;
    if (icao_filter) conditions.push_back("icao LIKE ?");
    if (country_filter) conditions.push_back("country LIKE ?");
    if (city_filter) conditions.push_back("city LIKE ?");
    if (type_filter) conditions.push_back("type = ?");
    if (min_elevation) conditions.push_back("elevation >= ?");
    if (max_elevation) conditions.push_back("elevation <= ?");
    
    if (!conditions.empty()) {
        query << " WHERE " << conditions[0];
        for (size_t i = 1; i < conditions.size(); ++i) {
            query << " AND " << conditions[i];
        }
    }
    
    if (sort_by_icao) query << " ORDER BY icao";
    if (limit > 0) query << " LIMIT " << limit;
    
    try {
        SQLite::Statement stmt(*m_db, query.str());
        
        // Bind parameters
        int param_index = 1;
        if (icao_filter) stmt.bind(param_index++, "%" + *icao_filter + "%");
        if (country_filter) stmt.bind(param_index++, "%" + *country_filter + "%");
        if (city_filter) stmt.bind(param_index++, "%" + *city_filter + "%");
        if (type_filter) stmt.bind(param_index++, *type_filter);
        if (min_elevation) stmt.bind(param_index++, *min_elevation);
        if (max_elevation) stmt.bind(param_index++, *max_elevation);
        
        while (stmt.executeStep()) {
            AirportMeta airport;
            
            if (!stmt.isColumnNull(0)) airport.icao = stmt.getColumn(0).getString();
            if (!stmt.isColumnNull(1)) airport.iata = stmt.getColumn(1).getString();
            if (!stmt.isColumnNull(2)) airport.faa = stmt.getColumn(2).getString();
            if (!stmt.isColumnNull(3)) airport.airport_name = stmt.getColumn(3).getString();
            if (!stmt.isColumnNull(4)) airport.elevation = stmt.getColumn(4).getInt();
            if (!stmt.isColumnNull(5)) airport.type = stmt.getColumn(5).getString();
            if (!stmt.isColumnNull(6)) airport.latitude = stmt.getColumn(6).getDouble();
            if (!stmt.isColumnNull(7)) airport.longitude = stmt.getColumn(7).getDouble();
            if (!stmt.isColumnNull(8)) airport.country = stmt.getColumn(8).getString();
            if (!stmt.isColumnNull(9)) airport.city = stmt.getColumn(9).getString();
            if (!stmt.isColumnNull(10)) airport.region = stmt.getColumn(10).getString();
            if (!stmt.isColumnNull(11)) airport.transition_alt = stmt.getColumn(11).getString();
            if (!stmt.isColumnNull(12)) airport.transition_level = stmt.getColumn(12).getString();
            
            results.push_back(airport);
        }
        
    } catch (const SQLite::Exception& e) {
        throw std::runtime_error("Airport query failed: " + std::string(e.what()));
    }
    
    return results;
}

std::optional<AirportMeta> AirportQueryBuilder::first() {
    limit = 1;
    auto results = execute();
    return results.empty() ? std::nullopt : std::make_optional(results[0]);
}

size_t AirportQueryBuilder::count() {
    // Build count query
    std::ostringstream query;
    query << "SELECT COUNT(*) FROM airports";
    
    std::vector<std::string> conditions;
    if (icao_filter) conditions.push_back("icao LIKE ?");
    if (country_filter) conditions.push_back("country LIKE ?");
    if (city_filter) conditions.push_back("city LIKE ?");
    if (type_filter) conditions.push_back("type = ?");
    if (min_elevation) conditions.push_back("elevation >= ?");
    if (max_elevation) conditions.push_back("elevation <= ?");
    
    if (!conditions.empty()) {
        query << " WHERE " << conditions[0];
        for (size_t i = 1; i < conditions.size(); ++i) {
            query << " AND " << conditions[i];
        }
    }
    
    try {
        SQLite::Statement stmt(*m_db, query.str());
        
        // Bind parameters (same as execute)
        int param_index = 1;
        if (icao_filter) stmt.bind(param_index++, "%" + *icao_filter + "%");
        if (country_filter) stmt.bind(param_index++, "%" + *country_filter + "%");
        if (city_filter) stmt.bind(param_index++, "%" + *city_filter + "%");
        if (type_filter) stmt.bind(param_index++, *type_filter);
        if (min_elevation) stmt.bind(param_index++, *min_elevation);
        if (max_elevation) stmt.bind(param_index++, *max_elevation);
        
        if (stmt.executeStep()) {
            return static_cast<size_t>(stmt.getColumn(0).getInt64());
        }
        
    } catch (const SQLite::Exception& e) {
        throw std::runtime_error("Airport count query failed: " + std::string(e.what()));
    }
    
    return 0;
}