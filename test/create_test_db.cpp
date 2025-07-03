#include <NavDataManager/NavDataManager.h>
#include <iostream>
#include <filesystem>

int main() {
    std::filesystem::path test_db = std::filesystem::temp_directory_path() / "ndm_test_database.db";
    std::filesystem::remove(test_db);
    
    NavDataManager manager("C:/X-Plane 12");
    manager.scan_xp();
    manager.connect_database(test_db.string());
    manager.parse_all_dat_files();
    
    std::cout << "Database created: " << test_db << std::endl;
    return 0;
}