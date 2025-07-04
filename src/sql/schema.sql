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
    country TEXT,
    city TEXT,
    state TEXT,
    region TEXT,
    transition_alt TEXT,
    transition_level TEXT
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
    FOREIGN KEY (airport_icao) REFERENCES airports (icao)
);

-- ====================================================================
-- Logical Taxiway Network Graph (Row Codes 1201-1202)
-- Defines the routing network for ATC and AI. This IS the centerline.
-- ====================================================================
CREATE TABLE IF NOT EXISTS taxi_nodes (
    node_id INTEGER NOT NULL, -- Using the ID from the file directly
    airport_icao TEXT NOT NULL,
    latitude REAL NOT NULL,
    longitude REAL NOT NULL,
    node_type TEXT,              -- 'junc', 'init', 'end', 'both'
    PRIMARY KEY (node_id, airport_icao),    -- composite primary key
    FOREIGN KEY (airport_icao) REFERENCES airports (icao)
);

CREATE TABLE IF NOT EXISTS taxi_edges (
    edge_id INTEGER PRIMARY KEY AUTOINCREMENT,
    airport_icao TEXT NOT NULL,
    start_node_id INTEGER NOT NULL,
    end_node_id INTEGER NOT NULL,
    is_two_way BOOLEAN NOT NULL,
    taxiway_name TEXT,           -- The human-readable name, e.g., "A", "B1"
    width_class TEXT,            -- e.g., 'A', 'B', 'C' for aircraft size
    FOREIGN KEY (airport_icao) REFERENCES airports (icao),
    FOREIGN KEY (start_node_id, airport_icao) REFERENCES taxi_nodes (node_id, airport_icao),
    FOREIGN KEY (end_node_id, airport_icao) REFERENCES taxi_nodes (node_id, airport_icao)
);

-- ====================================================================
-- Airport Feature Data (Signs, Lines, Startup Locations)
-- ====================================================================
CREATE TABLE IF NOT EXISTS taxiway_signs (
    sign_id INTEGER PRIMARY KEY AUTOINCREMENT,
    airport_icao TEXT NOT NULL,
    latitude REAL NOT NULL,
    longitude REAL NOT NULL,
    heading REAL,
    sign_text TEXT,              -- The text displayed on the sign
    size_class INTEGER,
    FOREIGN KEY (airport_icao) REFERENCES airports (icao)
);

-- ====================================================================
-- Linear Feature Data (Lines) Row Codes: 111-116 and 120
-- ====================================================================

CREATE TABLE IF NOT EXISTS linear_features (
    airport_icao TEXT NOT NULL,
    feature_sequence INTEGER NOT NULL,  -- Sequential number airport
    line_type TEXT,              -- Describes the line, e.g., "ILS_hold_short"
    PRIMARY KEY (airport_icao, feature_sequence),
    FOREIGN KEY (airport_icao) REFERENCES airports (icao)
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
    FOREIGN KEY (airport_icao) REFERENCES airports (icao)
);