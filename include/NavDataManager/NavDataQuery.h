#pragma once
#include <NavDataManager/Types.h>
#include <vector>
#include <optional>
#include <string>

namespace SQLite { class Database; class Statement; }

class NavDataQuery {
    public:
        explicit NavDataQuery(SQLite::Database* db) : m_db(db) {}
        virtual ~NavDataQuery() = default;
    protected:
        SQLite::Database* m_db;
};