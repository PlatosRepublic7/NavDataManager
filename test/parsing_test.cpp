#include "gtest/gtest.h"
#include <NavDataManager/NavDataManager.h>
#include <NavDataManager/AirportQuery.h>
#include <filesystem>
#include <chrono>

class ParsingTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_db_path = std::filesystem::temp_directory_path() / 
                      ("parsing_test_" + std::to_string(std::time(nullptr)) + ".db");
        std::filesystem::remove(temp_db_path);
        
        // Initialize manager
        manager = std::make_unique<NavDataManager>("C:/X-Plane 12");
        manager->scan_xp();
        manager->connect_database(temp_db_path.string());
    }
    
    void TearDown() override {
        manager.reset();
        std::filesystem::remove(temp_db_path);
    }
    
    std::filesystem::path temp_db_path;
    std::unique_ptr<NavDataManager> manager;
};

TEST_F(ParsingTest, ParsesAptDatFiles) {
    EXPECT_NO_THROW({
        manager->parse_all_dat_files();
    });
}

TEST_F(ParsingTest, DatabaseHasExpectedTables) {
    manager->parse_all_dat_files();
    
    // Check for US airports specifically
    auto us_airports = manager->airport_data()
        .airports()
        .country("United States")
        .execute();
    EXPECT_GT(us_airports.size(), 0) << "Should find US airports";
}

TEST_F(ParsingTest, ParsedDataIsAccurate) {
    manager->parse_all_dat_files();
    
    // Test known airport (KEWR - Newark)
    auto kewr = manager->airport_data()
        .airports()
        .icao("KEWR")
        .first();
    
    ASSERT_TRUE(kewr.has_value()) << "KEWR should be found";
    EXPECT_EQ(kewr->icao.value(), "KEWR");
    EXPECT_EQ(kewr->country.value_or(""), "USA United States");
    EXPECT_NEAR(kewr->elevation.value_or(0), 17, 10) << "KEWR elevation should be ~17ft";
    
    // Test runway data
    auto runways = manager->airport_data().get_runways_for_airport("KEWR");
    EXPECT_GE(runways.size(), 2) << "KEWR should have multiple runways";
}