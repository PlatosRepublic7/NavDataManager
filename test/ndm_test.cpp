#include "gtest/gtest.h"
#include "NavDataManager.h"
#include <filesystem>

TEST(NavDataManagerTest, ConstructorCreatesDatabase) {
    std::string db_path = "test_navdata.db";
    {
        NavDataManager ndm(db_path);
        EXPECT_TRUE(std::filesystem::exists(db_path));
    }
    std::filesystem::remove(db_path);
}

TEST(NavDataManagerTest, TablesAreCreated) {
    std::string db_path = "test_navdata.db";
    {
        NavDataManager ndm(db_path);
        {
            SQLite::Database db(db_path, SQLite::OPEN_READONLY);
            bool airports_exists = false, runways_exists = false;
            SQLite::Statement query(db, "SELECT name FROM sqlite_master WHERE type='table';");
            while (query.executeStep()) {
                std::string name = query.getColumn(0);
                if (name == "airports") airports_exists = true;
                if (name == "runways") runways_exists = true;
            }
            EXPECT_TRUE(airports_exists);
            EXPECT_TRUE(runways_exists);
        }
    }
    std::filesystem::remove(db_path);
}
