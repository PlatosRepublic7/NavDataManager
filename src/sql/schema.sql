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
    transition_alt INTEGER,
    transition_level INTEGER
);
CREATE TABLE IF NOT EXISTS runways (
    runway_id INTEGER PRIMARY KEY AUTOINCREMENT,
    airport_icao TEXT,
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