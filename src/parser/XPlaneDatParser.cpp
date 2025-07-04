#include "XPlaneDatParser.h"
#include <iostream>
#include <cctype>
#include <algorithm>
#include <set>

namespace fs = std::filesystem;

XPlaneDatParser::XPlaneDatParser(bool logging) : m_logging_enabled(logging) {}

ParsedAptData XPlaneDatParser::parse_airport_dat(const fs::path& file) {
    ParsedAptData parsed_data;
    LookaheadLineReader reader(file, m_logging_enabled);
    int row_code = -100;
    
    while (reader.get_next_line()) {
        // Tokenize the current line
        auto tokens = reader.get_line_tokens();
        if (tokens.empty()) continue;
        try {
            // Check row code to determine what to do with the current line's data
            row_code = reader.get_row_code();

            switch (row_code) {
                case 1:
                case 16:
                case 17:
                case 1302:
                    reader.put_line_back();
                    process_airport_meta(reader, parsed_data);
                    break;

                // Runway case
                case 100:
                    reader.put_line_back();
                    process_runway(reader, parsed_data);
                    break;

                // Taxiway Network Node
                case 1200:
                case 1201:
                    reader.put_line_back();
                    process_taxiway_node(reader, parsed_data);
                    break;

                // Taxiway Network Edge
                case 1202:
                    reader.put_line_back();
                    process_taxiway_edge(reader, parsed_data);
                    break;

                // Linear Feature Cases
                case 120:
                    reader.put_line_back();
                    process_linear_feature(reader, parsed_data);
                    break;
            }
        } catch (const std::exception& e) {
            std::cerr << "\nError parsing " << file.string() << ": " << e.what() << std::endl;
            throw;
        }
    }
    return parsed_data;
}

void XPlaneDatParser::process_airport_meta(LookaheadLineReader& reader, ParsedAptData& data) {
    AirportMeta airport_meta;
    bool is_valid_row_code = true;
    
    while (reader.get_next_line()) {
        auto tokens = reader.get_line_tokens();
        int row_code = reader.get_row_code();
        if (tokens.empty()) continue;
        if (!is_valid_row_code) {
            try {
                // Update the context for foreign keys
                reset_airport_context(airport_meta.icao.value());

                data.airports.push_back(std::move(airport_meta));
            } catch (...) { throw; }
            reader.put_line_back();
            break;
        }
        try {
            switch (row_code) {
                case 1:
                case 16:
                case 17: {
                    if (row_code == 1) airport_meta.type = std::string("Land");
                    else if (row_code == 16) airport_meta.type = std::string("Seaplane");
                    else if (row_code == 17) airport_meta.type = std::string("Heliport");

                    // Check if there is a [H] or [S] token in the vector
                    if (tokens.size() >= 6) {
                        for (size_t i = 3; i < tokens.size(); ++i) {
                            if (tokens[i] == "[H]" || tokens[i] == "[S]" || tokens[i] == "[X]") {
                                // We found it, and now we can remove it from the vector
                                tokens.erase(tokens.begin() + i);
                                break;
                            }
                        }
                    }
                    airport_meta.elevation = std::stoi(std::string(tokens[1]));
                    airport_meta.icao = std::string(tokens[4]);

                    // We need to account for the usual case where an airport_name is multiple tokens
                    std::string s_airport_name = "";
                    for (size_t j = 5; j < tokens.size(); ++j) {
                        s_airport_name += std::string(tokens[j]) + " ";
                    }

                    // Strip whitespace from both ends
                    if (!s_airport_name.empty()) {
                        size_t start = s_airport_name.find_first_not_of(" \t\n\r\f\v");
                        if (start != std::string::npos) {
                            size_t end = s_airport_name.find_last_not_of(" \t\n\r\f\v");
                            s_airport_name = s_airport_name.substr(start, end - start + 1);
                        } else {
                            s_airport_name.clear();
                        }
                    }
                    
                    airport_meta.airport_name = s_airport_name;
                    break;
                }
                case 1302: {
                    std::string key = std::string(tokens[1]);
                    std::string value;
                    if (tokens.size() < 3) break;
                    if (tokens.size() > 3) {
                        for (size_t i = 2; i < tokens.size(); ++i) {
                            value += std::string(tokens[i]);
                            if (i < tokens.size() - 1) {
                                value += " ";
                            }
                        }
                    } else {
                        value = std::string(tokens[2]);
                    }

                    if (key == "icao_code") {
                        airport_meta.icao = value;
                    } else if (key == "faa_code") {
                        airport_meta.faa = value;
                    } else if (key == "iata_code") {
                        airport_meta.iata = value;
                    } else if (key == "city") {
                        airport_meta.city = value;
                    } else if (key == "country") {
                        airport_meta.country = value;
                    } else if (key == "state") {
                        airport_meta.state = value;
                    } else if (key == "region_code") {
                        airport_meta.region = value;
                    } else if (key == "transition_alt") {
                        airport_meta.transition_alt = value;
                    } else if (key == "transition_level") {
                        // We need to check for all kinds of incorrect strings here
                        bool is_non_numeric = std::none_of(value.begin(), value.end(), [](unsigned char c) {
                            return std::isdigit(c);
                        });

                        if (value.substr(0, 2) == "FL" || is_non_numeric) {
                            airport_meta.transition_level = value;
                        } else {
                            // We need to convert all purely numerical instances to the form "FLxxx" or "FLxx"
                            int numeric_value = std::stoi(value);
                            numeric_value = static_cast<int>(numeric_value / 100);
                            std::string converted_value = "FL" + std::to_string(numeric_value);
                            airport_meta.transition_level = converted_value;
                        }
                    } else if (key == "datum_lat") {
                        airport_meta.latitude = std::stod(value);
                    } else if (key == "datum_lon") {
                        airport_meta.longitude = std::stod(value);
                    }

                    // Ignore other keys
                    break;
                }
                default:
                    is_valid_row_code = false;
                    reader.put_line_back();
                    break;
            }
        } catch (const std::exception& e) { 
            std::ostringstream error_msg = write_parser_error(reader, tokens, e);
            throw std::runtime_error(error_msg.str());
        }
    }
}

void XPlaneDatParser::process_runway(LookaheadLineReader& reader, ParsedAptData& data) {
    RunwayData runway_data;
    bool is_valid_row_code = true;

    while (reader.get_next_line()) {
        auto tokens = reader.get_line_tokens();
        int row_code = reader.get_row_code();
        if (tokens.empty()) continue;
        if (!is_valid_row_code) {
            reader.put_line_back();
            break;
        }
        try {
            switch (row_code) {
                case 100: {
                    // Convert Runway numbers to include a leading '0' if they're single digit
                    std::string rw_numbers[2] = { std::string(tokens[8]), std::string(tokens[17]) };
                    for (auto& rw_num : rw_numbers) {
                        bool has_suffix = (rw_num.back() == 'L' || rw_num.back() == 'C' || rw_num.back() == 'R');
                        if (rw_num.size() < 3 && has_suffix) {
                            rw_num = "0" + rw_num;
                        }
                    }

                    runway_data.airport_icao = m_current_airport_icao;
                    runway_data.width = std::stod(std::string(tokens[1]));
                    runway_data.surface = std::stoi(std::string(tokens[2]));
                    runway_data.end1_rw_number = rw_numbers[0];
                    runway_data.end1_lat = std::stod(std::string(tokens[9]));
                    runway_data.end1_lon = std::stod(std::string(tokens[10]));
                    runway_data.end1_d_threshold = std::stod(std::string(tokens[11]));
                    runway_data.end1_rw_marking_code = std::stoi(std::string(tokens[13]));
                    runway_data.end1_rw_app_light_code = std::stoi(std::string(tokens[14]));
                    runway_data.end2_rw_number = rw_numbers[1];
                    runway_data.end2_lat = std::stod(std::string(tokens[18]));
                    runway_data.end2_lon = std::stod(std::string(tokens[19]));
                    runway_data.end2_d_threshold = std::stod(std::string(tokens[20]));
                    runway_data.end2_rw_marking_code = std::stoi(std::string(tokens[22]));
                    runway_data.end2_rw_app_light_code = std::stoi(std::string(tokens[23]));

                    data.runways.push_back(std::move(runway_data));
                    break;
                }
                default:
                    is_valid_row_code = false;
                    reader.put_line_back();
                    break;
            }
        } catch (const std::exception& e) {
            std::ostringstream error_msg = write_parser_error(reader, tokens, e);
            throw std::runtime_error(error_msg.str());
        }
    }
}

void XPlaneDatParser::process_taxiway_node(LookaheadLineReader& reader, ParsedAptData& data) {
    TaxiwayNodeData taxiway_node;
    bool is_valid_row_code = true;
    while (reader.get_next_line()) {
        auto tokens = reader.get_line_tokens();
        int row_code = reader.get_row_code();
        if (tokens.empty()) continue;
        if (!is_valid_row_code) {
            reader.put_line_back();
            break;
        }
        try {
            switch (row_code) {
                case 1200:
                    break;
                case 1201: {
                    taxiway_node.node_id = std::stoi(std::string(tokens[4]));
                    taxiway_node.airport_icao = m_current_airport_icao;
                    taxiway_node.latitude = std::stod(std::string(tokens[1]));
                    taxiway_node.longitude = std::stod(std::string(tokens[2]));
                    taxiway_node.node_type = std::string(tokens[3]);

                    data.taxiway_nodes.push_back(taxiway_node);
                    break;
                }
                default:
                    is_valid_row_code = false;
                    reader.put_line_back();
                    break;
            }
        } catch (const std::exception& e) {
            std::ostringstream error_msg = write_parser_error(reader, tokens, e);
            throw std::runtime_error(error_msg.str());
        }
    }
}

void XPlaneDatParser::process_taxiway_edge(LookaheadLineReader& reader, ParsedAptData& data) {
    TaxiwayEdgeData taxiway_edge;
    bool is_valid_row_code = true;
    while (reader.get_next_line()) {
        auto tokens = reader.get_line_tokens();
        int row_code = reader.get_row_code();
        if (tokens.empty()) continue;
        if (!is_valid_row_code) {
            reader.put_line_back();
            break;
        }
        try {
            switch (row_code) {
                case 1202: {
                    taxiway_edge.airport_icao = m_current_airport_icao;
                    taxiway_edge.start_node_id = std::stoi(std::string(tokens[1]));
                    taxiway_edge.end_node_id = std::stoi(std::string(tokens[2]));
                    std::string is_twoway_string = std::string(tokens[3]);
                    bool is_two_way = true;
                    if (is_twoway_string != "twoway") {
                        is_two_way = false;
                    }
                    taxiway_edge.is_two_way = is_two_way;
                    
                    std::string width_code = std::string(tokens[4]);
                    if (!width_code.empty()) {
                        char last_char = width_code.back();
                        taxiway_edge.width_class = std::string(1, last_char);
                    }
                    
                    if (tokens.size() > 5) {
                        taxiway_edge.taxiway_name = std::string(tokens[5]);
                    } else {
                        taxiway_edge.taxiway_name = std::nullopt;
                    }

                    data.taxiway_edges.push_back(taxiway_edge);
                    break;
                }
                default:
                    is_valid_row_code = false;
                    reader.put_line_back();
                    break;
            }
        } catch (const std::exception& e) {
            std::ostringstream error_msg = write_parser_error(reader, tokens, e);
            throw std::runtime_error(error_msg.str());
        }
    }
}

void XPlaneDatParser::process_linear_feature(LookaheadLineReader& reader, ParsedAptData& data) {
    LinearFeatureData linear_feature;
    LinearFeatureNodeData linear_feature_node;
    std::vector<LinearFeatureNodeData> linear_feature_cache;
    bool is_valid_row_code = true, feature_header_processed = false;
    bool is_taxiway_feature = false;
    int node_order = 0;
    int assigned_feature_sequence = -1;

    // Set of line type codes relating to taxiways that are important for our purposes (check apt.dat specification for more details)
    // NOTE: We are trying to capture centerlines, centerline lights, hold-short lines, runway lead-off lights
    // We are NOT trying to capture edge lighting or boundary features, as this will not add value (at the moment) to our Navigational data.
    std::set<int> taxiway_codes = {1, 4, 5, 6, 7, 51, 54, 55, 56, 57, 101, 103, 104, 105, 107, 108};
    while (reader.get_next_line()) {
        auto tokens = reader.get_line_tokens();
        int row_code = reader.get_row_code();
        if (tokens.empty()) continue;
        if (!is_valid_row_code) {
            reader.put_line_back();
            break;
        }
        try {
            switch (row_code) {
                // Linear feature header
                case 120: {
                    if (!feature_header_processed) {
                        linear_feature.airport_icao = m_current_airport_icao;
                        assigned_feature_sequence = m_current_airport_feature_sequence++;
                        linear_feature.feature_sequence = assigned_feature_sequence;

                        if (tokens.size() > 1) {
                            linear_feature.line_type = std::string(tokens[1]);
                        }
                        feature_header_processed = true;
                    } else {
                        // We've already processed the feature header
                        // This means we've likely encountered the next group of feature header + feature nodes
                        // Better to send it back to the dispatcher...
                        is_valid_row_code = false;
                        reader.put_line_back();
                    }
                    break;
                }
                case 111:
                case 112:
                case 113:
                case 114:
                case 115:
                case 116: {
                    if (!feature_header_processed) {
                        throw std::runtime_error("Found linear feature node without a header");
                    }
                    
                    int shorter_line_code = 0, longer_line_code = 0;
                    
                    // Fetch the data that will always be present
                    linear_feature_node.airport_icao = m_current_airport_icao;
                    linear_feature_node.feature_sequence = assigned_feature_sequence;
                    linear_feature_node.latitude = std::stod(std::string(tokens[1]));
                    linear_feature_node.longitude = std::stod(std::string(tokens[2]));

                    if (row_code == 112 || row_code == 114 || row_code == 116) {
                        // Bezier case
                        linear_feature_node.bezier_latitude = std::stod(std::string(tokens[3]));
                        linear_feature_node.bezier_longitude = std::stod(std::string(tokens[4]));

                        if (tokens.size() > 5) {
                            shorter_line_code = std::stoi(std::string(tokens[5]));
                            if (tokens.size() == 7) {
                                longer_line_code = std::stoi(std::string(tokens[6]));
                            }
                        }
                    } else {
                        // Non-Bezier case
                        if (tokens.size() > 3 && (row_code != 112 || row_code != 114 || row_code != 116)) {
                            shorter_line_code = std::stoi(std::string(tokens[3]));
                            if (tokens.size() == 5) {
                                longer_line_code = std::stoi(std::string(tokens[4]));
                            }
                        }
                    }

                    linear_feature_node.node_order = node_order++;

                    // Check line_code ints to determine if we have found a node that is part of our taxiway_codes set
                    if (taxiway_codes.count(shorter_line_code) || taxiway_codes.count(longer_line_code)) {
                        is_taxiway_feature = true;
                    }

                    linear_feature_cache.push_back(linear_feature_node);
                    break;
                }
                default:
                    is_valid_row_code = false;
                    reader.put_line_back();
                    break;
            }
        } catch (const std::exception& e) {
            std::ostringstream error_msg = write_parser_error(reader, tokens, e);
            throw std::runtime_error(error_msg.str());
        }
    }

    // Once we're out of the while loop, we can add the feature header and node cache to the parsed data structure
    if (is_taxiway_feature) {
        data.linear_features.push_back(linear_feature);
        for (const auto& node : linear_feature_cache) {
            data.linear_feature_nodes.push_back(node);
        }
    }
}

// Utility function for Parser errors
std::ostringstream XPlaneDatParser::write_parser_error(LookaheadLineReader& reader, std::vector<std::string_view>& tokens, const std::exception& e) {
    std::ostringstream error_msg;
    std::string line_data;
    std::string token_data_str = "[";
    for (const auto& token : tokens) {
        line_data += std::string(token) + " ";
        token_data_str += " \'" + std::string(token) + "\' ";
    }
    token_data_str += "]";
    error_msg << "\nParser Error at line: " << reader.get_line_number() << "\nLine Data: " << line_data << "\nTokens: " << token_data_str << "\nRuntime Error: " << e.what();
    return error_msg;
}