cmake_minimum_required(VERSION 3.20)

project(trust_beacon VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(trust_beacon src/production_trust_controller.cpp)
add_library(trust_beacon::trust_beacon ALIAS trust_beacon)

target_include_directories(
  trust_beacon
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(trust_beacon PUBLIC cxx_std_17)
target_compile_options(
  trust_beacon
  PRIVATE $<$<CXX_COMPILER_ID:Clang,GNU>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wsign-conversion>
          $<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->)

set(SANITIZE "" CACHE STRING "Comma-separated sanitizers (for example address,undefined)")
if(SANITIZE)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    message(FATAL_ERROR "SANITIZE requires Clang or GCC")
  endif()
  target_compile_options(trust_beacon PUBLIC -fsanitize=${SANITIZE} -fno-omit-frame-pointer)
  target_link_options(trust_beacon PUBLIC -fsanitize=${SANITIZE})
endif()

install(
  TARGETS trust_beacon
  EXPORT trust_beaconTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(
  EXPORT trust_beaconTargets
  FILE trust_beaconTargets.cmake
  NAMESPACE trust_beacon::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/trust_beacon)
configure_package_config_file(
  cmake/trust_beaconConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/trust_beaconConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/trust_beacon)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/trust_beaconConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/trust_beaconConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/trust_beaconConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/trust_beacon)

include(CTest)
if(BUILD_TESTING)
  option(TRUST_BEACON_FETCH_CATCH2 "Download Catch2 for the test suite" OFF)
  if(TRUST_BEACON_FETCH_CATCH2)
    include(FetchContent)
    FetchContent_Declare(
      Catch2
      URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.7.1.tar.gz
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
  else()
    find_package(Catch2 3 QUIET)
  endif()

  if(Catch2_FOUND OR TARGET Catch2::Catch2WithMain)
    add_executable(trust_beacon_tests tests/production_trust_controller_test.cpp)
    target_link_libraries(trust_beacon_tests PRIVATE trust_beacon::trust_beacon Catch2::Catch2WithMain)
  else()
    message(STATUS "Catch2 not found; using the dependency-free validation runner")
    add_executable(trust_beacon_tests tests/standalone_test.cpp)
    target_link_libraries(trust_beacon_tests PRIVATE trust_beacon::trust_beacon)
  endif()
  target_compile_features(trust_beacon_tests PRIVATE cxx_std_17)
  target_compile_options(
    trust_beacon_tests
    PRIVATE $<$<CXX_COMPILER_ID:Clang,GNU>:-Wall;-Wextra;-Wpedantic>
            $<$<CXX_COMPILER_ID:MSVC>:/W4>)

  if(Catch2_FOUND OR TARGET Catch2::Catch2WithMain)
    include(Catch)
    catch_discover_tests(trust_beacon_tests)
  else()
    add_test(NAME trust_beacon_tests COMMAND trust_beacon_tests)
  endif()
endif()
