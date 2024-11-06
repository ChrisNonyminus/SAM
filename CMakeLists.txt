cmake_minimum_required(VERSION 3.18)

project(sam C)

set(SRC_DIR src)

set(SOURCES ${SRC_DIR}/reciter.c ${SRC_DIR}/sam.c ${SRC_DIR}/render.c
            ${SRC_DIR}/processframes.c ${SRC_DIR}/createtransitions.c)

set(HEADERS ${SRC_DIR}/reciter.h ${SRC_DIR}/sam.h ${SRC_DIR}/render.h
            ${SRC_DIR}/processframes.h ${SRC_DIR}/createtransitions.h)

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

add_executable(sam ${SRC_DIR}/main.c)

target_link_libraries(sam PRIVATE samlib)
target_compile_options(sam PRIVATE -Wall -O2)
