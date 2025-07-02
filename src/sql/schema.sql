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
-- Physical Pavement Geometry (Row Codes 110-116)
-- Defines the physical, drawable shapes of taxiways and aprons.
-- ====================================================================
CREATE TABLE IF NOT EXISTS pavement_polygons (
    pavement_id INTEGER PRIMARY KEY AUTOINCREMENT,
    airport_icao TEXT NOT NULL,
    surface_type INTEGER,      -- From the 110 header
    smoothness REAL,           -- From the 110 header
    texture_heading REAL,      -- From the 110 header
    FOREIGN KEY (airport_icao) REFERENCES airports (icao)
);

CREATE TABLE IF NOT EXISTS pavement_nodes (
    node_id INTEGER PRIMARY KEY AUTOINCREMENT,
    pavement_id INTEGER NOT NULL,
    latitude REAL NOT NULL,
    longitude REAL NOT NULL,
    -- For curved segments (Bezier curves)
    bezier_latitude REAL,
    bezier_longitude REAL,
    is_hole_boundary BOOLEAN NOT NULL DEFAULT 0, -- Differentiates outer vs inner (hole) boundaries
    node_order INTEGER NOT NULL, -- The sequence of vertices for the polygon
    FOREIGN KEY (pavement_id) REFERENCES pavement_polygons (pavement_id) ON DELETE CASCADE
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

CREATE TABLE IF NOT EXISTS linear_features (
    feature_id INTEGER PRIMARY KEY AUTOINCREMENT,
    airport_icao TEXT NOT NULL,
    line_type TEXT,              -- Describes the line, e.g., "ILS_hold_short"
    FOREIGN KEY (airport_icao) REFERENCES airports (icao)
);

CREATE TABLE IF NOT EXISTS linear_feature_nodes (
    node_id INTEGER PRIMARY KEY AUTOINCREMENT,
    feature_id INTEGER NOT NULL,
    latitude REAL NOT NULL,
    longitude REAL NOT NULL,
    node_order INTEGER NOT NULL,
    FOREIGN KEY (feature_id) REFERENCES linear_features (feature_id) ON DELETE CASCADE
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