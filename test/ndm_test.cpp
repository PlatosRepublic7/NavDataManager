#include "gtest/gtest.h"
#include <NavDataManager/NavDataManager.h>
#include <filesystem>
#include <SQLiteCpp/Exception.h>

TEST(NavDataManagerTest, ThrowsOnInvalidDirectory) {
    std::string invalid_path = "C:/X-Plane 13";
    EXPECT_THROW({
        NavDataManager ndm(invalid_path);
        ndm.scan_xp();
    }, std::invalid_argument);
}

TEST(NavDataManagerTest, InitializesWithValidDirectory) {
    std::string valid_dir = "C:/X-Plane 12";
    EXPECT_NO_THROW({
        NavDataManager ndm(valid_dir);
        ndm.scan_xp();
    });
}

TEST(NavDataManagerTest, GenerateDatabaseCreatesFilesAndTables) {
    std::filesystem::path temp_db = std::filesystem::temp_directory_path() / "ndm_test.db";
    // Ensure that the file does not exist before test
    std::filesystem::remove(temp_db);

    {
        NavDataManager ndm("C:/X-Plane 12");
        EXPECT_NO_THROW({
            ndm.connect_database(temp_db.string());
        });
    }

    // Check that the file was created
    EXPECT_TRUE(std::filesystem::exists(temp_db));

    // Clean up
    std::filesystem::remove(temp_db);
}

TEST(NavDataManagerTest, GenerateDatabaseThrowsOnInvalidPath) {
    NavDataManager ndm("C:/X-Plane 12");
    std::string invalid_path = "/this/path/should/not/exist/ndm_test.db";
    EXPECT_THROW({
        ndm.connect_database(invalid_path);
    }, SQLite::Exception);
}
