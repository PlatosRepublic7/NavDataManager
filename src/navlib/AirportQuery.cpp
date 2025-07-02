#include <NavDataManager/AirportQuery.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <sstream>
#include <algorithm>

std::vector<RunwayData> RunwayQueryBuilder::execute() {
    std::vector<RunwayData> results;

    // Build dynamic query
    std::ostringstream query;
    query << "SELECT airport_icao, width, surface, end1_rw_number, end1_lat, end1_lon, end1_d_threshold, end1_rw_marking_code, end1_rw_app_light_code, "
          << "end2_rw_number, end2_lat, end2_lon, end2_d_threshold, end2_rw_marking_code, end2_rw_app_light_code FROM runways";
    
    std::vector<std::string> conditions;
    if (airport_filter) conditions.push_back("airport_icao = ?");
    if (surface_filter) conditions.push_back("surface = ?");
    if (min_width_filter) conditions.push_back("width >= ?");
    if (runway_number_filter) conditions.push_back("end1_rw_number = ? OR end2_rw_number = ?");
    
    if (!conditions.empty()) {
        query << " WHERE " << conditions[0];
        for (size_t i = 1; i < conditions.size(); ++i) {
            query << " AND " << conditions[i];
        }
    }

    if (sort_by_icao) query << " ORDER BY airport_icao";
    if (limit > 0) query << " LIMIT " << limit;

    try {
        SQLite::Statement stmt(*m_db, query.str());

        // bind parameters
        int param_index = 1;
        if (airport_filter) stmt.bind(param_index++, *airport_filter);
        if (surface_filter) stmt.bind(param_index++, *surface_filter);
        if (min_width_filter) stmt.bind(param_index++, *min_width_filter);
        if (runway_number_filter) stmt.bind(param_index++, *runway_number_filter);

        while (stmt.executeStep()) {
            RunwayData runway;

            if (!stmt.isColumnNull(0)) runway.airport_icao = stmt.getColumn(0).getString();
            if (!stmt.isColumnNull(1)) runway.width = stmt.getColumn(1).getDouble();
            if (!stmt.isColumnNull(2)) runway.surface = stmt.getColumn(2).getInt();
            if (!stmt.isColumnNull(3)) runway.end1_rw_number = stmt.getColumn(3).getString();
            if (!stmt.isColumnNull(4)) runway.end1_lat = stmt.getColumn(4).getDouble();
            if (!stmt.isColumnNull(5)) runway.end1_lon = stmt.getColumn(5).getDouble();
            if (!stmt.isColumnNull(6)) runway.end1_d_threshold = stmt.getColumn(6).getDouble();
            if (!stmt.isColumnNull(7)) runway.end1_rw_marking_code = stmt.getColumn(7).getInt();
            if (!stmt.isColumnNull(8)) runway.end1_rw_app_light_code = stmt.getColumn(8).getInt();
            if (!stmt.isColumnNull(9)) runway.end2_rw_number = stmt.getColumn(9).getString();
            if (!stmt.isColumnNull(10)) runway.end2_lat = stmt.getColumn(10).getDouble();
            if (!stmt.isColumnNull(11)) runway.end2_lon = stmt.getColumn(11).getDouble();
            if (!stmt.isColumnNull(12)) runway.end2_d_threshold = stmt.getColumn(12).getDouble();
            if (!stmt.isColumnNull(13)) runway.end2_rw_marking_code = stmt.getColumn(13).getInt();
            if (!stmt.isColumnNull(14)) runway.end2_rw_app_light_code = stmt.getColumn(14).getInt();

            results.push_back(runway);
        }
    } catch (SQLite::Exception& e) {
        throw std::runtime_error("Runway query failed: " + std::string(e.what()));
    }

    return results;
}

std::optional<RunwayData> RunwayQueryBuilder::first() {
    limit = 1;
    auto results = execute();
    return results.empty() ? std::nullopt : std::make_optional(results[0]);
}

size_t RunwayQueryBuilder::count() {
    std::ostringstream query;
    query << "SELECT COUNT(*) FROM runways";

    std::vector<std::string> conditions;
    if (airport_filter) conditions.push_back("airport_icao LIKE ?");
    if (surface_filter) conditions.push_back("surface = ?");
    if (min_width_filter) conditions.push_back("width >= ?");
    if (runway_number_filter) conditions.push_back("end1_rw_number = ? OR end2_rw_number = ?");
    
    if (!conditions.empty()) {
        query << " WHERE " << conditions[0];
        for (size_t i = 1; i < conditions.size(); ++i) {
            query << " AND " << conditions[i];
        }
    }

    if (sort_by_icao) query << " ORDER BY airport_icao";
    if (limit > 0) query << " LIMIT " << limit;

    try {
        SQLite::Statement stmt(*m_db, query.str());

        // bind parameters
        int param_index = 1;
        if (airport_filter) stmt.bind(param_index++, "%" + *airport_filter + "%");
        if (surface_filter) stmt.bind(param_index++, *surface_filter);
        if (min_width_filter) stmt.bind(param_index++, *min_width_filter);
        if (runway_number_filter) stmt.bind(param_index++, *runway_number_filter);

        if (stmt.executeStep()) {
            return static_cast<size_t>(stmt.getColumn(0).getInt64());
        }
    } catch (SQLite::Exception& e) {
        throw std::runtime_error("Runway query failed: " + std::string(e.what()));
    }

    return 0;
}

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