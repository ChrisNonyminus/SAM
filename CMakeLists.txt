cmake_minimum_required(VERSION 3.18)

project(sam)

set(CMAKE_CXX_STANDARD 17)

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

include_directories(src)

add_library(
  samlib STATIC
  src/PhonemeConvertor.cpp
  src/Sam.cpp
  src/WavWriter.cpp
  src/WavWriter.h
  src/SDLOutput.cpp
  src/SDLOutput.h
  src/SampleConvertor.cpp
  src/tables.h
  src/PhonemeConvertor.h
  src/Sam.h
  src/SampleConvertor.h)

target_compile_options(samlib PRIVATE -Wall -O2)

if(USE_SDL)
  target_link_libraries(samlib PUBLIC ${SDL_LIBRARY})
endif()

add_executable(sam src/main.cpp)

target_link_libraries(sam PRIVATE samlib)
target_compile_options(sam PRIVATE -Wall -O2)
