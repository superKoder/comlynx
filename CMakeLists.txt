cmake_minimum_required(VERSION 3.14)
project(comlynx)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
  comlynx_test
  src/comlynx_test.cc
  src/comlynx.cc
)
target_link_libraries(
  comlynx_test
  GTest::gtest_main
  GTest::gmock_main
)
add_test(comlynx_test comlynx_test
)

include(GoogleTest)
gtest_discover_tests(comlynx_test)