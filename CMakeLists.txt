cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(bmtc VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)

# BMT C-API
FetchContent_Declare(bmt
    GIT_REPOSITORY https://github.com/hidegi/BMT-C-API.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(bmt)

# nlohmann/json (header-only)
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install    OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

add_executable(bmtc src/main.cpp)
set_target_properties(bmtc PROPERTIES OUTPUT_NAME bmt)

target_compile_features(bmtc PRIVATE cxx_std_17)

target_link_libraries(bmtc PRIVATE
    bmt
    nlohmann_json::nlohmann_json
)

install(TARGETS bmtc RUNTIME DESTINATION bin)
