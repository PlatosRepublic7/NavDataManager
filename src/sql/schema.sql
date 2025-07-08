-- ====================================================================
-- Core Airport and Runway Data
-- ====================================================================
CREATE TABLE IF NOT EXISTS airports (
    icao TEXT PRIMARY KEY,
    iata TEXT,
    faa TEXT,
    airport_name TEXT,
    elevation INTEGER,
    type TEXT,
    latitude REAL,
    longitude REAL,
    country_id INTEGER,
    state_id INTEGER,
    city_id INTEGER,
    region_id INTEGER,
    transition_alt TEXT,
    transition_level TEXT,
    FOREIGN KEY (country_id) REFERENCES countries(country_id),
    FOREIGN KEY (state_id) REFERENCES states(state_id),
    FOREIGN KEY (city_id) REFERENCES cities(city_id),
    FOREIGN KEY (region_id) REFERENCES regions(region_id)
);

--- Country Normalization
CREATE TABLE IF NOT EXISTS countries (
    country_id INTEGER PRIMARY KEY AUTOINCREMENT,
    country_name TEXT UNIQUE NOT NULL
);

--- State Normalization
CREATE TABLE IF NOT EXISTS states (
    state_id INTEGER PRIMARY KEY AUTOINCREMENT,
    state_name TEXT NOT NULL,
    country_id INTEGER,
    UNIQUE (state_name, country_id),
    FOREIGN KEY (country_id) REFERENCES countries(country_id)
);

-- City Normalization
CREATE TABLE IF NOT EXISTS cities (
    city_id INTEGER PRIMARY KEY AUTOINCREMENT,
    city_name TEXT NOT NULL,
    state_id INTEGER,
    country_id INTEGER,
    UNIQUE(city_name, state_id, country_id),
    FOREIGN KEY (state_id) REFERENCES states(state_id),
    FOREIGN KEY (country_id) REFERENCES countries(country_id)
);

--- Region Normalization
CREATE TABLE IF NOT EXISTS regions (
    region_id INTEGER PRIMARY KEY AUTOINCREMENT,
    region_code TEXT UNIQUE NOT NULL
);


CREATE TABLE IF NOT EXISTS runways (
    runway_id INTEGER PRIMARY KEY AUTOINCREMENT,
    airport_icao TEXT NOT NULL,
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
    FOREIGN KEY (airport_icao) REFERENCES airports (icao) ON DELETE CASCADE
);

-- ====================================================================
-- Logical Taxiway Network Graph (Row Codes 1201-1202)
-- Defines the routing network for ATC and AI.
-- ====================================================================
CREATE TABLE IF NOT EXISTS taxi_nodes (
    node_id INTEGER NOT NULL, -- Using the ID from the file directly
    airport_icao TEXT NOT NULL,
    latitude REAL NOT NULL,
    longitude REAL NOT NULL,
    node_type TEXT,              -- 'junc', 'init', 'end', 'both'
    PRIMARY KEY (node_id, airport_icao),    -- composite primary key
    FOREIGN KEY (airport_icao) REFERENCES airports (icao) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS taxi_edges (
    edge_id INTEGER PRIMARY KEY AUTOINCREMENT,
    airport_icao TEXT NOT NULL,
    start_node_id INTEGER NOT NULL,
    end_node_id INTEGER NOT NULL,
    is_two_way BOOLEAN NOT NULL,
    taxiway_name TEXT,           -- The human-readable name, e.g., "A", "B1"
    width_class TEXT,            -- e.g., 'A', 'B', 'C' for aircraft size
    UNIQUE (airport_icao, start_node_id, end_node_id),
    FOREIGN KEY (airport_icao) REFERENCES airports (icao) ON DELETE CASCADE,
    FOREIGN KEY (start_node_id, airport_icao) REFERENCES taxi_nodes (node_id, airport_icao),
    FOREIGN KEY (end_node_id, airport_icao) REFERENCES taxi_nodes (node_id, airport_icao)
);

-- ====================================================================
-- Linear Feature Data (Lines) Row Codes: 111-116 and 120
-- ====================================================================

CREATE TABLE IF NOT EXISTS linear_features (
    airport_icao TEXT NOT NULL,
    feature_sequence INTEGER NOT NULL,  -- Sequential number airport
    line_type TEXT,              -- Describes the line, e.g., "ILS_hold_short"
    PRIMARY KEY (airport_icao, feature_sequence),
    FOREIGN KEY (airport_icao) REFERENCES airports (icao) ON DELETE CASCADE
); 

CREATE TABLE IF NOT EXISTS linear_feature_nodes (
    node_id INTEGER PRIMARY KEY AUTOINCREMENT,
    airport_icao TEXT NOT NULL,
    feature_sequence INTEGER NOT NULL,
    latitude REAL NOT NULL,
    longitude REAL NOT NULL,
    -- For curved segments (Bezier curves)
    bezier_latitude REAL,
    bezier_longitude REAL,
    node_order INTEGER NOT NULL, -- The sequence of nodes for the linear feature segment
    UNIQUE (airport_icao, feature_sequence, node_order),
    FOREIGN KEY (airport_icao, feature_sequence) REFERENCES linear_features (airport_icao, feature_sequence) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS startup_locations (
    location_id INTEGER PRIMARY KEY AUTOINCREMENT,
    airport_icao TEXT NOT NULL,
    latitude REAL NOT NULL,
    longitude REAL NOT NULL,
    heading REAL,
    location_type TEXT,          -- 'Gate', 'Parking', 'Cargo', etc.
    ramp_name TEXT,              -- Name of the gate/spot, e.g., "A17"
    FOREIGN KEY (airport_icao) REFERENCES airports (icao) ON DELETE CASCADE
);

-- Aircraft types that can use startup locations
CREATE TABLE IF NOT EXISTS aircraft_types (
    aircraft_type_id INTEGER PRIMARY KEY AUTOINCREMENT,
    aircraft_type_code TEXT UNIQUE NOT NULL -- 'jets' 'heavy' 'props' etc.
);

-- Junction table linking startup locations to aircraft types (many-to-many)
CREATE TABLE IF NOT EXISTS startup_location_aircraft_types (
    location_id INTEGER NOT NULL,
    aircraft_type_id INTEGER NOT NULL,
    PRIMARY KEY (location_id, aircraft_type_id),
    FOREIGN KEY (location_id) REFERENCES startup_locations (location_id) ON DELETE CASCADE,
    FOREIGN KEY (aircraft_type_id) REFERENCES aircraft_types (aircraft_type_id) ON DELETE CASCADE
);

-- ====================================================================
-- NON X-PLANE DATA
-- General data for things like path checks, configuration, etc.
-- ====================================================================
CREATE TABLE IF NOT EXISTS scenery_paths (
    scenery_path_id INTEGER PRIMARY KEY AUTOINCREMENT,
    scenery_path TEXT NOT NULL,
    created_at TEXT NOT NULL DEFAULT (datetime('now'))
);

-- Indexes for increased performance
CREATE INDEX IF NOT EXISTS idx_airports_country_id ON airports(country_id);
CREATE INDEX IF NOT EXISTS idx_airports_state_id ON airports(state_id);
CREATE INDEX IF NOT EXISTS idx_airports_city_id ON airports(city_id);
CREATE INDEX IF NOT EXISTS idx_airports_region_id ON airports(region_id);
CREATE INDEX IF NOT EXISTS idx_startup_location_aircraft_types_location ON startup_location_aircraft_types(location_id);
CREATE INDEX IF NOT EXISTS idx_startup_location_aircraft_types_aircraft ON startup_location_aircraft_types(aircraft_type_id);