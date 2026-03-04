cmake_minimum_required(VERSION 3.10)

project(OpenErase VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Enable strict compiler warnings to catch potential memory/pointer issues
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror=vla)
    # Note: -Werror=vla prevents Variable Length Arrays, which can cause stack overflows
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

set(CORE_SOURCES
    src/main.c
    src/cli.c
    src/shred.c
    src/wipe.c
    src/utils.c
)

add_executable(openerase ${CORE_SOURCES})

# ==========================================
# Testing Setup (Optional but highly recommended)
# ==========================================
enable_testing()

# Build the shred test
add_executable(test_shred test/test_shred.c src/shred.c src/utils.c)
add_test(NAME ShredTest COMMAND test_shred)

# Build the wipe test
add_executable(test_wipe test/test_wipe.c src/wipe.c src/utils.c)
add_test(NAME WipeTest COMMAND test_wipe)