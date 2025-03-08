project(${CMAKE_PROJECT_NAME} VERSION ${CMAKE_PROJECT_VERSION} LANGUAGES CXX)

set(PROJECT_NAME ${CMAKE_PROJECT_NAME})

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")
message(STATUS "Found sourcess: ${sources}")

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

flex_target(LEXER ${CMAKE_SOURCE_DIR}/include/parser/tsp_lexer.l ${CMAKE_CURRENT_BINARY_DIR}/tsp_lexer.cc 
            DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/tsp_lexer.h)
bison_target(PARSER ${CMAKE_SOURCE_DIR}/include/parser/tsp_parser.y ${CMAKE_CURRENT_BINARY_DIR}/tsp_parser.cc
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/tsp_parser.h)
add_flex_bison_dependency(LEXER PARSER)

add_executable(${PROJECT_NAME}
  ${sources}
  ${FLEX_LEXER_OUTPUTS}
  ${BISON_PARSER_OUTPUTS}
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
    ENABLE_EXPORTS TRUE 
    CXX_STANDARD ${CMAKE_CXX_STANDARD}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE 
    -fsized-deallocation -D__cpp_concepts=202002 -Wno-builtin-macro-redefined
    -Wno-reorder-init-list
  )
  target_link_libraries(
    ${PROJECT_NAME} 
    PRIVATE 
      stdc++exp fmt::fmt CLI11 tabulate
  )
else()
  message(FATAL_ERROR "Unsupported compiler use Clang instead")
endif()

configure_file(${CMAKE_SOURCE_DIR}/include/config.h.in ${CMAKE_SOURCE_DIR}/include/config.h)

target_include_directories(
    ${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    ${CMAKE_CURRENT_BINARY_DIR}
)
