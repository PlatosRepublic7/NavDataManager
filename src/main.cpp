#include "NavDataManager.h"
#include <iostream>

int main() {
    // Example paths (replace with actual test paths as needed)
    std::string db_directory = "test.db";
    std::string xp_root_path = "C:/X-Plane 12";

    // Create an instance of NavDataManager
    NavDataManager manager(xp_root_path, true);

    // Call scan()
    manager.scanXP();

    // Call generateDatabase()
    manager.generateDatabase(db_directory);

    std::cout << "NavDataManager test completed." << std::endl;
    return 0;
}