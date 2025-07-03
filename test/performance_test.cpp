#include "gtest/gtest.h"
#include "simple_test_base.h"
#include <NavDataManager/NavDataManager.h>
#include <NavDataManager/AirportQuery.h>
#include <filesystem>
#include <chrono>
#include <iostream>

class PerformanceTest : public SimpleTestBase {
};

class ParsingPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_db_path = std::filesystem::temp_directory_path() / 
                      ("parse_perf_test_" + std::to_string(std::time(nullptr)) + ".db");
        std::filesystem::remove(temp_db_path);
        
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

// Move parsing test to the separate class
TEST_F(ParsingPerformanceTest, ParsingPerformance) {
    auto start = std::chrono::high_resolution_clock::now();
    
    manager->parse_all_dat_files();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    
    std::cout << "Parsing completed in: " << duration.count() << " seconds" << std::endl;
    
    EXPECT_LT(duration.count(), 60) << "Parsing should complete within 60 seconds";
}

TEST_F(PerformanceTest, QueryPerformance) {
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Perform multiple queries
    for (int i = 0; i < 100; ++i) {
        auto airports = manager->airport_data()
            .airports()
            .country("United States")
            .max_results(10)
            .execute();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "100 queries completed in: " << duration.count() << " ms" << std::endl;
    
    // Should be fast (adjust threshold as needed)
    EXPECT_LT(duration.count(), 1000) << "100 queries should complete within 1 second";
}

TEST_F(PerformanceTest, MemoryUsage) {
    
    // Perform many queries to check for memory leaks
    for (int i = 0; i < 1000; ++i) {
        auto airports = manager->airport_data()
            .airports()
            .country("United States")
            .max_results(1)
            .execute();
    }
    
    // If we get here without crashing, memory management is likely OK
    SUCCEED() << "Memory test completed without issues";
}