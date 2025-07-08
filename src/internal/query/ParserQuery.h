#pragma once
#include <NavDataManager/NavDataQuery.h>
#include <NavDataManager/Types.h>
#include <vector>
#include <unordered_set>

class ParserQuery : public NavDataQuery {

    public:
        ParserQuery(SQLite::Database* db, bool logging) : NavDataQuery(db), m_logging_enabled(logging) {}

        void insert_airports(const std::vector<AirportMeta>& airports, bool is_custom_scenery, std::unordered_set<std::string>& airports_in_transaction);
        void insert_runways(const std::vector<RunwayData>& runways);
        void insert_taxiway_nodes(const std::vector<TaxiwayNodeData>& taxiway_nodes);
        void insert_taxiway_edges(const std::vector<TaxiwayEdgeData>& taxiway_edges);
        void insert_linear_features(const std::vector<LinearFeatureData>& linear_features);
        void insert_linear_feature_nodes(const std::vector<LinearFeatureNodeData>& linear_feature_nodes);
        void insert_startup_locations(const std::vector<StartupLocationData>& startup_locations);
    
    private:
        bool m_logging_enabled;
    
};