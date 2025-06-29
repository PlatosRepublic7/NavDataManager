#include <NavDataManager/NavDataManager.h>
#include <iostream>

int main() {
    // Example paths (replace with actual test paths as needed)
    std::string db_directory = "test.db";
    std::string xp_root_path = "C:/X-Plane 12";

    // Create an instance of NavDataManager
    NavDataManager manager(xp_root_path, true);

    // Call scan_xp()
    manager.scan_xp();

    // Call generate_database()
    manager.generate_database(db_directory);

    // Call the parser
    manager.parse_all_dat_files();

    std::cout << "NavDataManager test completed." << std::endl;
    return 0;
}