#include "gtest/gtest.h"
#include "simple_test_base.h"
#include <NavDataManager/AirportQuery.h>
#include <filesystem>

class IntegrityTest : public SimpleTestBase {
};

TEST_F(IntegrityTest, AllAirportsHaveValidICAO) {
    auto airports = manager->airport_data().airports().country("United States").max_results(100).execute();
    
    for (const auto& airport : airports) {
        ASSERT_TRUE(airport.icao.has_value()) << "Airport should have ICAO code";
        EXPECT_GE(airport.icao->length(), 3) << "ICAO should be at least 3 characters";
        EXPECT_LE(airport.icao->length(), 4) << "ICAO should be at most 4 characters";
    }
}

TEST_F(IntegrityTest, RunwaysHaveValidAirportReferences) {
    auto airports = manager->airport_data().airports().country("United States").max_results(10).execute();
    
    for (const auto& airport : airports) {
        auto runways = manager->airport_data().get_runways_for_airport(airport.icao.value());
        
        for (const auto& runway : runways) {
            EXPECT_EQ(runway.airport_icao.value(), airport.icao.value()) 
                << "Runway should reference correct airport";
            EXPECT_GT(runway.length_meters(), 0) << "Runway should have positive length (METERS)";
            EXPECT_GT(runway.length_feet(), 0) << "Runway should have positive length (FEET)";
        }
    }
}

TEST_F(IntegrityTest, CoordinatesAreValid) {
    auto airports = manager->airport_data().airports().country("United States").max_results(100).execute();
    
    for (const auto& airport : airports) {
        if (airport.latitude.has_value()) {
            EXPECT_GE(airport.latitude.value(), -90.0) << "Latitude should be >= -90";
            EXPECT_LE(airport.latitude.value(), 90.0) << "Latitude should be <= 90";
        }
        
        if (airport.longitude.has_value()) {
            EXPECT_GE(airport.longitude.value(), -180.0) << "Longitude should be >= -180";
            EXPECT_LE(airport.longitude.value(), 180.0) << "Longitude should be <= 180";
        }
    }
}