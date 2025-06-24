#include "NavDataManager.h"
#include <iostream>

int main() {
    // Example paths (replace with actual test paths as needed)
    std::string db_directory = "test_db";
    std::string xp_root_path = "C:/X-Plane 12";

    // Create an instance of NavDataManager
    NavDataManager manager(db_directory);

    // Call updateDatabase to test functionality
    manager.updateDatabase(xp_root_path);

    std::cout << "NavDataManager test completed." << std::endl;
    return 0;
}