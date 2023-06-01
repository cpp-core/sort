cmake_minimum_required (VERSION 3.24 FATAL_ERROR)

find_package(Threads REQUIRED)

set(TESTS
  sort/basic
  )

set(LIBRARIES
  sort
  GTest::gtest
  Threads::Threads
  )

configure_tests("core" "${LIBRARIES}" ${TESTS})
