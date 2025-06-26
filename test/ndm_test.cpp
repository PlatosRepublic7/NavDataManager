#include "gtest/gtest.h"
#include "NavDataManager.h"
#include <filesystem>

TEST(NavDataManagerTest, ThrowsOnInvalidDirectory) {
    std::string invalid_path = "C:/X-Plane 13";
    EXPECT_THROW({
        NavDataManager ndm(invalid_path);
        ndm.scanXP();
    }, std::invalid_argument);
}

TEST(NavDataManagerTest, InitializesWithValidDirectory) {
    std::string valid_dir = "C:/X-Plane 12";
    EXPECT_NO_THROW({
        NavDataManager ndm(valid_dir);
        ndm.scanXP();
    });
}

TEST(NavDataManagerTest, GenerateDatabaseCreatesFilesAndTables) {
    std::filesystem::path temp_db = std::filesystem::temp_directory_path() / "ndm_test.db";
    // Ensure that the file does not exist before test
    std::filesystem::remove(temp_db);

    {
        NavDataManager ndm("C:/X-Plane 12");
        EXPECT_NO_THROW({
            ndm.generateDatabase(temp_db.string());
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
        ndm.generateDatabase(invalid_path);
    }, SQLite::Exception);
}
