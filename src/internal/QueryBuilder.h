#pragma once
#include <vector>
#include <optional>
#include <string>
#include <map>

namespace SQLite { class Database; class Statement; }

template<typename ResultType>
class QueryBuilder {
    public:
        explicit QueryBuilder(SQLite::Database* db, const std::string& base_table)
        : m_db(db), m_base_table(base_table), m_limit(100) {}

        // Basic Filtering Methods
        QueryBuilder& where(const std::string& column, const std::string& value) {
            m_string_filters[column] = value;
            return *this;
        }

        QueryBuilder& where(const std::string& column, int value) {
            m_int_filters[column] = value;
            return *this;
        }

        QueryBuilder& where(const std::string& column, double value) {
            m_double_filters[column] = value;
            return *this;
        }

        QueryBuilder& where(const std::string& column, float value) {
            m_float_filters[column] = value;
            return *this;
        }

        QueryBuilder& where_like(const std::string& column, const std::string& pattern) {
            m_like_filters[column] = pattern;
            return *this;
        }

        QueryBuilder& order_by(const std::string& column, bool ascending=true) {
            m_order_column = column;
            m_order_ascending = ascending;
            return *this;
        }

        QueryBuilder& max_results(int limit) {
            m_limit = limit;
            return *this;
        }

        // Terminal Methods
        std::vector<ResultType> execute();
        std::optional<ResultType> first();
        size_t count();

    protected:
        // Child classes will implement these methods
        virtual std::string get_select_clause() = 0;
        virtual std::string get_from_clause() = 0;
        virtual ResultType map_row_to_result(SQLite::Statement& stmt) const = 0;

    private:
        SQLite::Database* m_db;
        std::string m_base_table;

        // Filter Storage
        std::map<std::string, std::string> m_string_filters;
        std::map<std::string, int> m_int_filters;
        std::map<std::string, double> m_double_filters;
        std::map<std::string, float> m_float_filters;
        std::map<std::string, std::string> m_like_filters;
        std::map<std::string, std::vector<std::string>> m_in_filters;

        // Ordering/Limiting
        std::optional<std::string> m_order_column;
        bool m_order_ascending = true;
        int m_limit;

        // Post-Processing

        std::string build_query() const;
        void bind_parameters(SQLite::Statement& stmt) const;
};