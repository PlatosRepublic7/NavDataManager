#include <NavDataManager/NavDataManager.h>
#include <NavDataManager/AirportQuery.h>
#include <iostream>

int main() {
    try {
        // Initialization
        NavDataManager manager("C:/X-Plane 12", true);
        manager.scan_xp();
        manager.connect_database("nav.db");
        manager.parse_all_dat_files();

        // Builder pattern queries
        auto major_us_airports = manager.airport_data()
            .airports()
            .country("United States")
            .max_results(50)
            .execute();
        
        std::cout << "Found " << major_us_airports.size() << " US airports" << std::endl;

        auto kewr_airport = manager.airport_data()
            .airports()
            .icao("KEWR")
            .first();

        int kewr_elevation = kewr_airport->elevation.value();
        std::cout << "Found: " << kewr_airport->icao.value_or("N/A") << std::endl;
        std::cout << "Airport Name: " << kewr_airport->display_name() << std::endl;
        std::cout << "Elevation: " << std::to_string(kewr_elevation) << std::endl; 
        
        std::cout << std::endl;
        std::cout << "Runways" << std::endl;

        // Runway queries
        auto kewr_runways = manager.airport_data()
            .get_runways_for_airport(kewr_airport->icao.value());

        for (const auto& runway : kewr_runways) {
            std::cout << "Numbers: " << runway.full_runway_name() << std::endl;
            std::cout << "Length (Meters): " << runway.length_meters() << std::endl;
            std::cout << "Length (Feet):   " << runway.length_feet() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unkown exception caught! " << std::endl;
        return 1;
    }
    return 0;
}