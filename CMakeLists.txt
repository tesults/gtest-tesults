cmake_minimum_required(VERSION 3.14)
project(tesults-gtest VERSION 1.0.2 LANGUAGES CXX)

find_package(GTest CONFIG REQUIRED)
find_package(tesults CONFIG REQUIRED)

add_library(tesults_gtest INTERFACE)
target_include_directories(tesults_gtest INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(tesults_gtest INTERFACE cxx_std_17)
target_link_libraries(tesults_gtest INTERFACE GTest::gtest tesults::tesults)

if(BUILD_EXAMPLES)
    add_executable(tesults_gtest_example examples/main.cpp)
    target_link_libraries(tesults_gtest_example PRIVATE tesults_gtest GTest::gtest_main)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS tesults_gtest EXPORT tesults-gtestTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set(TESULTS_GTEST_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/tesults-gtest")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/tesults-gtestConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/tesults-gtestConfig.cmake"
    INSTALL_DESTINATION "${TESULTS_GTEST_CMAKE_DIR}"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/tesults-gtestConfigVersion.cmake"
    VERSION       ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(EXPORT tesults-gtestTargets
    FILE      tesults-gtestTargets.cmake
    NAMESPACE tesults_gtest::
    DESTINATION "${TESULTS_GTEST_CMAKE_DIR}"
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/tesults-gtestConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/tesults-gtestConfigVersion.cmake"
    DESTINATION "${TESULTS_GTEST_CMAKE_DIR}"
)
