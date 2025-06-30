#include <NavDataManager/NavDataManager.h>
#include <NavDataManager/AirportQuery.h>  // 🔧 Add this include
#include <iostream>

int main() {
    NavDataManager manager("C:/X-Plane 12", true);
    manager.scan_xp();
    manager.connect_database("nav.db");
    manager.parse_all_dat_files();

    // Builder pattern queries
    auto major_us_airports = manager.airports()
        .query()
        .country("United States")
        .max_results(50)
        .execute();
    
    std::cout << "Found " << major_us_airports.size() << " US airports" << std::endl;

    auto kewr_airport = manager.airports()
        .query()
        .icao("KEWR")
        .first();

    int kewr_elevation = kewr_airport->elevation.value();
    std::cout << "Found: " << kewr_airport->icao.value_or("N/A") << std::endl;
    std::cout << "Elevation: " << std::to_string(kewr_elevation) << std::endl; 
    
    return 0;
}