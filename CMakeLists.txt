# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
	CACHE STING "Vcpkg toolchain file") 

project ("agent" VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(RdKafka REQUIRED)
find_package(unofficial-modern-cpp-kafka REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable (agent)

# Add sources and headers
file(GLOB_RECURSE Agent_SOURCES CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.inl"
)

target_sources(agent PRIVATE ${Agent_SOURCES})

# Playground include dirs
target_include_directories(agent PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(agent PRIVATE
    RdKafka::rdkafka++ 
    unofficial::modern-cpp-kafka::modern-cpp-kafka
    nlohmann_json::nlohmann_json 
)

if(MSVC)
    target_compile_options(agent PRIVATE
        /W3
        /MP # multithreaded build
        /WX # warnings as errors
        /ZI # program database for edit and continue
    )
else()
    target_compile_options(agent PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
    )
endif()

# Set Agent as the startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT agent)

# Filters
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${Agent_SOURCES})

# Enable solution folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

