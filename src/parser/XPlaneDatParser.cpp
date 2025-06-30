#include "XPlaneDatParser.h"
#include <iostream>
#include <cctype>
#include <algorithm>

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
                m_current_airport_icao = airport_meta.icao.value();

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
                            if (tokens[i] == "[H]" || tokens[i] == "[S]") {
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
            std::ostringstream error_msg;
            std::string line_data;
            std::string token_data_str = "[";
            for (const auto& token : tokens) {
                line_data += std::string(token) + " ";
                token_data_str += " \'" + std::string(token) + "\' ";
            }
            token_data_str += "]";
            error_msg << "\nParser Error at line: " << reader.get_line_number() << "\nLine Data: " << line_data << "\nTokens: " << token_data_str << "\nRuntime Error: " << e.what();
            throw std::runtime_error(error_msg.str());
        }
    }
}
