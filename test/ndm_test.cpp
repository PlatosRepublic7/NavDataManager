#include "gtest/gtest.h"
#include <NavDataManager/NavDataManager.h>
#include <NavDataManager/AirportQuery.h>
#include <filesystem>
#include <SQLiteCpp/Exception.h>
#include <ctime>

class NavDataManagerTest: public ::testing::Test {
    protected:
        void SetUp() override {
            // Create a temporary database for each test
            auto timestamp = std::to_string(std::time(nullptr));
            temp_db_path = std::filesystem::temp_directory_path() / ("ndm_test_" + timestamp + ".db");

            std::filesystem::remove(temp_db_path);
        }

        void TearDown() override {
            // Clean up
            std::filesystem::remove(temp_db_path);
        }

        std::filesystem::path temp_db_path;
        const std::string xplane_path = "C:/X-Plane 12";
};

TEST_F(NavDataManagerTest, ThrowsOnInvalidDirectory) {
    std::string invalid_path = "C:/X-Plane 13";
    EXPECT_THROW({
        NavDataManager ndm(invalid_path);
        ndm.scan_xp();
    }, std::invalid_argument);
}

TEST_F(NavDataManagerTest, InitializesWithValidDirectory) {
    EXPECT_NO_THROW({
        NavDataManager ndm(xplane_path);
        ndm.scan_xp();
    });
}

TEST_F(NavDataManagerTest, DatabaseConnectionAndCreation) {
    NavDataManager ndm(xplane_path);
    ndm.scan_xp();
    
    EXPECT_NO_THROW({
        ndm.connect_database(temp_db_path.string());
    });
    
    EXPECT_TRUE(std::filesystem::exists(temp_db_path));
    EXPECT_GT(std::filesystem::file_size(temp_db_path), 0);
}