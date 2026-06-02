cmake_minimum_required(VERSION 3.14)
project(gtest-tesults VERSION 1.0.1 LANGUAGES CXX)

find_package(GTest CONFIG REQUIRED)
find_package(tesults CONFIG REQUIRED)

add_library(gtest_tesults INTERFACE)
target_include_directories(gtest_tesults INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(gtest_tesults INTERFACE cxx_std_17)
target_link_libraries(gtest_tesults INTERFACE GTest::gtest tesults::tesults)

if(BUILD_EXAMPLES)
    add_executable(gtest_tesults_example examples/main.cpp)
    target_link_libraries(gtest_tesults_example PRIVATE gtest_tesults GTest::gtest_main)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS gtest_tesults EXPORT gtest-tesultsTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set(GTEST_TESULTS_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/gtest-tesults")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/gtest-tesultsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/gtest-tesultsConfig.cmake"
    INSTALL_DESTINATION "${GTEST_TESULTS_CMAKE_DIR}"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/gtest-tesultsConfigVersion.cmake"
    VERSION       ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(EXPORT gtest-tesultsTargets
    FILE      gtest-tesultsTargets.cmake
    NAMESPACE gtest_tesults::
    DESTINATION "${GTEST_TESULTS_CMAKE_DIR}"
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/gtest-tesultsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/gtest-tesultsConfigVersion.cmake"
    DESTINATION "${GTEST_TESULTS_CMAKE_DIR}"
)
