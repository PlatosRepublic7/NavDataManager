#include "ParserQuery.h"
#include <iostream>
#include <sstream>
#include <SQLiteCpp/SQLiteCpp.h>


void ParserQuery::insert_airports(const std::vector<AirportMeta>& airports, bool is_custom_scenery, std::unordered_set<std::string>& airports_in_transaction) {
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
        airport.icao ? airport_stmt.bind(1, clean_icao) : airport_stmt.bind(1);
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

void ParserQuery::insert_runways(const std::vector<RunwayData>& runways) {
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

void ParserQuery::insert_taxiway_nodes(const std::vector<TaxiwayNodeData>& taxiway_nodes) {
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

void ParserQuery::insert_taxiway_edges(const std::vector<TaxiwayEdgeData>& taxiway_edges) {
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

void ParserQuery::insert_linear_features(const std::vector<LinearFeatureData>& linear_features) {
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

void ParserQuery::insert_linear_feature_nodes(const std::vector<LinearFeatureNodeData>& linear_feature_nodes) {
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

void ParserQuery::insert_startup_locations(const std::vector<StartupLocationData>& startup_locations) {
    // Prepare statements
    SQLite::Statement location_stmt(*m_db, R"(
        INSERT INTO startup_locations
        (airport_icao, latitude, longitude, heading, location_type, ramp_name)
        VALUES (?, ?, ?, ?, ?, ?)    
    )");

    SQLite::Statement aircraft_type_stmt(*m_db, R"(
        INSERT OR IGNORE INTO aircraft_types (aircraft_type_code) VALUES (?)    
    )");

    SQLite::Statement junction_stmt(*m_db, R"(
        INSERT INTO startup_location_aircraft_types (location_id, aircraft_type_id)
        SELECT ?, aircraft_type_id FROM aircraft_types WHERE aircraft_type_code = ?
    )");

    for (const auto& location : startup_locations) {
        // Insert location
        location.airport_icao ? location_stmt.bind(1, *location.airport_icao) : location_stmt.bind(1);
        location.latitude ? location_stmt.bind(2, *location.latitude) : location_stmt.bind(2);
        location.longitude ? location_stmt.bind(3, *location.longitude) : location_stmt.bind(3);
        location.heading ? location_stmt.bind(4, *location.heading) : location_stmt.bind(4);
        location.location_type ? location_stmt.bind(5, *location.location_type) : location_stmt.bind(5);
        location.ramp_name ? location_stmt.bind(6, *location.ramp_name) : location_stmt.bind(6);

        location_stmt.executeStep();
        location_stmt.reset();

        int location_id = static_cast<int>(m_db->getLastInsertRowid());

        // Parse aircraft types: 'jets|heavy|props'
        std::string aircraft_types = *location.aircraft_types;
        std::stringstream ss(aircraft_types);
        std::string aircraft_type;

        while (std::getline(ss, aircraft_type, '|')) {
            // Insert aircraft type (if new)
            aircraft_type_stmt.bind(1, aircraft_type);
            aircraft_type_stmt.executeStep();
            aircraft_type_stmt.reset();

            // Link location to aircraft type
            junction_stmt.bind(1, location_id);
            junction_stmt.bind(2, aircraft_type);
            junction_stmt.executeStep();
            junction_stmt.reset();
        }
    }
}