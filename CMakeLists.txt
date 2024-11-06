cmake_minimum_required(VERSION 3.18)

project(sam)

set(CMAKE_CXX_STANDARD 17)

set(SRC_DIR src)

set(SOURCES ${SRC_DIR}/reciter.cpp ${SRC_DIR}/sam.cpp
    src/WavWriter.cpp
    src/WavWriter.h
    src/SDLOutput.cpp
    src/SDLOutput.h)
set(HEADERS ${SRC_DIR}/tables.h ${SRC_DIR}/reciter.h ${SRC_DIR}/sam.h)

option(USE_SDL "Build with SDL support" OFF)

if(USE_SDL)
    find_package(SDL REQUIRED)
    if(SDL_FOUND)
        include_directories(${SDL_INCLUDE_DIR})
        add_definitions(-DUSESDL)
    else()
        message(
                STATUS
                "SDL not found, building without SDL support, despite request to use")
        set(USE_SDL OFF)
    endif()
endif()

include_directories(${SRC_DIR})

add_library(samlib STATIC ${SOURCES})

target_compile_options(samlib PRIVATE -Wall -O2)

if(USE_SDL)
    target_link_libraries(samlib PUBLIC ${SDL_LIBRARY})
endif()

add_executable(sam ${SRC_DIR}/main.cpp)

target_link_libraries(sam PRIVATE samlib)
target_compile_options(sam PRIVATE -Wall -O2)
