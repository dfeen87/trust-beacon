cmake_minimum_required(VERSION 3.20)

project(trust_beacon VERSION 1.0.0 LANGUAGES CXX)

add_library(trust_beacon src/production_trust_controller.cpp)
add_library(trust_beacon::trust_beacon ALIAS trust_beacon)

target_include_directories(trust_beacon
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_features(trust_beacon PUBLIC cxx_std_17)
target_compile_options(trust_beacon PRIVATE
  $<$<CXX_COMPILER_ID:Clang,GNU>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wsign-conversion>
  $<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->)

include(CTest)
if(BUILD_TESTING)
  add_executable(trust_beacon_tests tests/production_trust_controller_test.cpp)
  target_link_libraries(trust_beacon_tests PRIVATE trust_beacon::trust_beacon)
  target_compile_features(trust_beacon_tests PRIVATE cxx_std_17)
  add_test(NAME trust_beacon_tests COMMAND trust_beacon_tests)
endif()

