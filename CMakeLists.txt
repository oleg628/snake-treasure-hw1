cmake_minimum_required(VERSION 3.20)

project(SnakeTreasure VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SNAKE_BUILD_TESTS "Build unit tests" ON)

add_library(snake_lib STATIC src/game.cpp)
target_include_directories(snake_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(snake_game src/main.cpp src/terminal.cpp)
target_link_libraries(snake_game PRIVATE snake_lib)

function(snake_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

snake_enable_warnings(snake_lib)
snake_enable_warnings(snake_game)

include(CTest)
if(SNAKE_BUILD_TESTS)
    add_subdirectory(tests)
    add_test(NAME demo_smoke
             COMMAND $<TARGET_FILE:snake_game> --demo --seed 25 --max-turns 80 --quiet)
    add_test(NAME help_output
             COMMAND $<TARGET_FILE:snake_game> --help)
    set_tests_properties(demo_smoke PROPERTIES TIMEOUT 20)
endif()

install(TARGETS snake_game RUNTIME DESTINATION bin)
