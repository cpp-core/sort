cmake_minimum_required (VERSION 3.24 FATAL_ERROR)
project (sort VERSION 0.1 LANGUAGES C CXX ASM)

# Make including this file idempotent
#
if(TARGET sort OR TARGET sort::sort)
  return()
endif()

# Setup symlinks for this architecture
#
# set(SORT_ARCH_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/words/${CMAKE_SYSTEM_PROCESSOR})
# if (EXISTS ${SORT_ARCH_INCLUDE_DIR})
#   message("-- sort: Creating architecture symlink to ${CMAKE_SYSTEM_PROCESSOR}")
#   execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
#     ${SORT_ARCH_INCLUDE_DIR}
#     ${CMAKE_SOURCE_DIR}/include/words/arch)
# endif()

# set(SORT_ARCH_SRC_DIR ${CMAKE_SOURCE_DIR}/src/words/${CMAKE_SYSTEM_PROCESSOR})
# if (EXISTS ${SORT_ARCH_SRC_DIR})
#   message("-- sort: Creating architecture symlink ${CMAKE_SYSTEM_PROCESSOR}")
#   execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
#     ${SORT_ARCH_SRC_DIR}
#     ${CMAKE_SOURCE_DIR}/src/words/arch)
# endif()

# If this is the current project.
#
if(PROJECT_NAME STREQUAL CMAKE_PROJECT_NAME)
  # Laod the cmake recipes and utilities.
  #
  include(${CMAKE_CURRENT_LIST_DIR}/cmake/load_cmake_helpers.cmake)

  # Options for generating tests and documentation
  #
  option(SORT_TEST "Generate the tests." ON)
  option(SORT_DOCS "Generate the docs." OFF)

  # compile_commands.json
  #
  symlink_compile_commands()

  # Convenience targets for git operations
  #
  make_submodule_pull_target()

  # Setup compilation parameters
  #
  set(CMAKE_CXX_STANDARD 23)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
  
  add_compile_options(-Wall)
  add_compile_options(-g)
  add_compile_options(-O)
  
  # add_compile_options(-fno-omit-frame-pointer)
  # add_compile_options(-fsanitize=address)
  # add_link_options(-fsanitize=address)

else()
  option(SORT_TEST "Generate the tests." OFF)
  option(SORT_DOCS "Generate the docs." OFF)
endif()

# Put executables in the top-level binary directory
#
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Output configuration
#
message("-- sort: Configuration: ${CMAKE_CURRENT_SOURCE_DIR}")
message("-- sort: Included from: ${CMAKE_SOURCE_DIR}")
message("-- sort: Install prefix: ${CMAKE_INSTALL_PREFIX}")
message("-- sort: test ${SORT_TEST}")
message("-- sort: docs ${SORT_DOCS}")

# Add our dependencies
#
add_fmt()
add_record()
add_timer()
add_boost_sort()
find_package(Threads REQUIRED)

# Build the library
#
set(SOURCES
  placeholder
  )

set(ASM_SOURCES
  )

set(FILES)
foreach(SOURCE ${SOURCES})
  list(APPEND FILES "src/core/sort/${SOURCE}.cpp")
endforeach()

foreach(SOURCE ${ASM_SOURCES})
  set(FILE "src/words/${CMAKE_SYSTEM_PROCESSOR}/${SOURCE}.s")
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}")
    list(APPEND FILES ${FILE})
  endif()
endforeach()

add_library(sort ${FILES})
add_library(sort::sort ALIAS sort)

# Define the header files for this library.
#
file(GLOB_RECURSE PUBLIC_INCLUDE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/*.h)
target_sources(sort INTERFACE FILE_SET HEADERS BASE_DIRS include FILES ${PUBLIC_INCLUDE_FILES})

target_include_directories(sort PUBLIC ${GMP_INCLUDE_DIRS})
target_link_libraries(sort PUBLIC record::record timer::timer fmt::fmt)

foreach(prog
    example0
    example1
    sort0
    # sort1
    sort2
    sort3
    sort4
    sort5
    sort6
    )
  add_executable(${prog} src/tools/${prog}.cpp)
  target_link_libraries(${prog} sort::sort Threads::Threads boost_sort)
endforeach()

# Optionally configure the tests
#
if(SORT_TEST)
 add_gtest()
 add_subdirectory(test)
endif()

# Optionally configure the documentation
#
# if(SORT_DOCS)
#   add_subdirectory(docs)
# endif()

# Installation
#
# install_config(sort)
