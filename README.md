# NavDataManager

**NavDataManager is a modern C++ library that eliminates the complexity of parsing X-Plane's navigation data. It scans your X-Plane installation, converts the raw `.dat` files into an optimized SQLite database, and provides a powerful, fluent API for querying airport and runway information.**

Stop writing boilerplate `.dat` file parsers and start building your application. NavDataManager handles the tedious work of finding, parsing, and organizing X-Plane's navigation data, allowing you to access it through a simple and elegant interface.

## Core Features

* ✈️ **Automatic Scenery Scanning:** Intelligently scans X-Plane's `Global Scenery` and `Custom Scenery` folders to find all `apt.dat` files.
* ⚡ **High-Performance Parsing:** Efficiently parses even the largest `apt.dat` files (including the 350MB+ global file) in seconds.
* 🗃️ **Optimized SQLite Backend:** Converts raw data into a structured and indexed SQLite database for fast and efficient querying.
* ✨ **Fluent Query API:** A clean, chainable, and intuitive API for building complex queries without writing a single line of SQL.
* 🛠️ **Modern C++ & CMake:** Built with modern C++17 and a robust CMake build system for easy integration into your own projects.
* ✅ **Extensible Design:** Ready to be extended with parsers for other X-Plane data files (`nav.dat`, `fix.dat`, etc.).

## Quick Start

Here's how easy it is to find an airport and its runways:

```cpp
#include <NavDataManager/NavDataManager.h>
#include <NavDataManager/AirportQuery.h>
#include <iostream>

int main() {
    // 1. Initialize the manager with your X-Plane root directory
    NavDataManager manager("C:/X-Plane 12");

    // 2. Scan for .dat files and connect to the database
    //    This will create and populate nav.db on the first run.
    manager.scan_xp();
    manager.connect_database("nav.db");
    manager.parse_all_dat_files();

    // 3. Query for an airport using the fluent API
    auto airport = manager.airport_data()
                            .airports()
                            .icao("KEWR")
                            .first();

    if (airport) {
        std::cout << "Found Airport: " << airport->display_name() << std::endl;
        std::cout << "Elevation: " << airport->elevation.value_or(0) << " ft" << std::endl;

        // 4. Get all runways for that airport
        auto runways = manager.airport_data()
                                .runways()
                                .airport_icao(airport->icao.value())
                                .execute();

        std::cout << "\nRunways:" << std::endl;
        for (const auto& runway : runways) {
            std::cout << "-> " << runway.full_runway_name()
                      << ", Length: " << static_cast<int>(runway.length_feet()) << " ft"
                      << std::endl;
        }
    }

    return 0;
}
```

## Installation & Integration

NavDataManager is designed to be easily integrated into any CMake-based project using `FetchContent`.

In your `CMakeLists.txt`:

```cmake
include(FetchContent)

# Fetch NavDataManager from GitHub
FetchContent_Declare(
    NavDataManager
    GIT_REPOSITORY https://github.com/PlatosRepublic7/NavDataManager.git
    GIT_TAG        main # release tag will be at a later time
)

FetchContent_MakeAvailable(NavDataManager)

# Link NavDataManager to your executable
add_executable(MyXPlaneApp main.cpp)
target_link_libraries(MyXPlaneApp PRIVATE NavDataManager::NavDataManager)
```

## API Usage Examples

The power of NavDataManager lies in its flexible query builder.

### Find Airports

```cpp
// Get the first 50 major airports in the United States
auto us_airports = manager.airport_data()
                            .airports()
                            .country("United States")
                            .max_results(50)
                            .execute();

// Count all heliports
size_t heliport_count = manager.airport_data()
                                .airports()
                                .type("Heliport")
                                .count();

// Find all airports in Denver
auto denver_airports = manager.airport_data()
                                .airports()
                                .city("Denver")
                                .execute();
```

### Find Runways

```cpp
// Get all runways for a specific airport
auto kbos_runways = manager.airport_data()
                             .runways()
                             .airport_icao("KBOS")
                             .execute();

// Find all runways with a width of at least 50 meters
auto wide_runways = manager.airport_data()
                             .runways()
                             .min_width(50.0)
                             .execute();
```

## Building the Project

To build the library and its tests from source:

```bash
# 1. Clone the repository
git clone https://github.com/PlatosRepublic7/NavDataManager.git
cd NavDataManager

# 2. Configure CMake
#    -DBUILD_TESTING=ON will build the test suite
cmake -S . -B build -DBUILD_TESTING=ON

# 3. Build the project
cmake --build build

# 4. Run tests (optional)
cd build
ctest
```

## Roadmap

NavDataManager is under active development. Future plans include:

* Parser for `nav.dat` (VORs, NDBs, ILSs, etc.)
* Parser for `fix.dat` (Fixes and waypoints)
* Parser for `awy.dat` (Airways)
* Query builders for new data types
* More comprehensive test coverage

## Contributing

Contributions are welcome! If you'd like to help improve NavDataManager, please feel free to fork the repository, make your changes, and submit a pull request. For major changes, please open an issue first to discuss what you would like to change.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.