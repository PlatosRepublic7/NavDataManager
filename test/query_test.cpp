#include "gtest/gtest.h"
#include "simple_test_base.h"
#include <NavDataManager/AirportQuery.h>
#include <filesystem>

class QueryTest : public SimpleTestBase {
};

TEST_F(QueryTest, BasicAirportQueries) {
    // Test country filter
    auto us_airports = manager->airport_data()
        .airports()
        .country("United States")
        .execute();
    EXPECT_GT(us_airports.size(), 0);
    
    for (const auto& airport : us_airports) {
        EXPECT_EQ(airport.country.value_or(""), "USA United States");
    }
}

TEST_F(QueryTest, AirportQueryByICAO) {
    auto kewr = manager->airport_data()
        .airports()
        .icao("KEWR")
        .first();
    
    ASSERT_TRUE(kewr.has_value());
    EXPECT_EQ(kewr->icao.value(), "KEWR");
}

TEST_F(QueryTest, RunwayQueries) {
    auto kewr_runways = manager->airport_data()
        .get_runways_for_airport("KEWR");
    
    EXPECT_GT(kewr_runways.size(), 0);
    
    for (const auto& runway : kewr_runways) {
        EXPECT_EQ(runway.airport_icao.value(), "KEWR");
        EXPECT_GT(runway.length_meters(), 0);
        EXPECT_GT(runway.length_feet(), 0);
    }
}

TEST_F(QueryTest, MaxResultsLimit) {
    auto limited_airports = manager->airport_data()
        .airports()
        .country("United States")
        .max_results(10)
        .execute();
    
    EXPECT_LE(limited_airports.size(), 10);
}

TEST_F(QueryTest, ConvenienceMethods) {
    // Test convenience methods
    auto kewr_by_convenience = manager->airport_data().get_by_icao("KEWR");
    ASSERT_TRUE(kewr_by_convenience.has_value());
    EXPECT_EQ(kewr_by_convenience->icao.value(), "KEWR");
    
    auto us_airports_convenience = manager->airport_data().get_by_country("United States", 5);
    EXPECT_LE(us_airports_convenience.size(), 5);
    EXPECT_GT(us_airports_convenience.size(), 0);
}