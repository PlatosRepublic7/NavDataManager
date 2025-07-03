#ifndef SIMPLE_TEST_BASE_H
#define SIMPLE_TEST_BASE_H

#include "gtest/gtest.h"
#include <NavDataManager/NavDataManager.h>
#include <filesystem>
#include <memory>

class SimpleTestBase : public ::testing::Test {
protected:
    void SetUp() override {
        auto master_db = std::filesystem::temp_directory_path() / "ndm_test_database.db";
        test_db_path = std::filesystem::temp_directory_path() / ("test_copy_" + std::to_string(std::time(nullptr)) + ".db");
        std::filesystem::copy_file(master_db, test_db_path);
        
        manager = std::make_unique<NavDataManager>("C:/X-Plane 12");
        manager->scan_xp();
        manager->connect_database(test_db_path.string());
    }
    
    void TearDown() override {
        manager.reset();
        std::filesystem::remove(test_db_path);
    }
    
    std::filesystem::path test_db_path;
    std::unique_ptr<NavDataManager> manager;
};

#endif