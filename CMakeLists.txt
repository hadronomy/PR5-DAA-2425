cmake_minimum_required(VERSION 3.14...3.22)

set(PROJECT_NAMESPACE "tsp")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(tsp
  VERSION 1.0.0
  DESCRIPTION ""
  LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(PROJECT_SHORTNAME "tsp")
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# ---- Add dependencies via CPM ----
# see https://github.com/TheLartians/CPM.cmake for more info

include(cmake/CPM.cmake)

# PackageProject.cmake will be used to make our target installable
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.8.0")

CPMAddPackage(
  NAME fmt
  GIT_TAG 10.2.1
  GITHUB_REPOSITORY fmtlib/fmt
  OPTIONS "FMT_INSTALL YES" # create an installable target
)

CPMAddPackage(
  NAME CLI11
  VERSION 2.5.0
  GITHUB_REPOSITORY CLIUtils/CLI11
  OPTIONS "CLI11_BUILD_EXAMPLES OFF" "CLI11_BUILD_TESTS OFF"
)


CPMAddPackage(
  NAME tabulate
  VERSION 1.5
  GITHUB_REPOSITORY p-ranav/tabulate
)

add_subdirectory("src")

