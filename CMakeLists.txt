
cmake_minimum_required(VERSION 3.24) 

# Cmake 3.24+ breaks extraction timestamps by default, hurray, but the option to not break
# timestamps fails in cmake <3.24, extra hurray!
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    cmake_policy(SET CMP0135 OLD)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(LANGS C CXX)
set(CMAKE_CXX_STANDARD 23)

find_program(CCACHE_PROGRAM ccache)

if(CCACHE_PROGRAM)
  foreach(lang ${LANGS})
    if(NOT DEFINED CMAKE_${lang}_COMPILER_LAUNCHER AND NOT CMAKE_${lang}_COMPILER MATCHES ".*/ccache")
      message(STATUS "Enabling ccache for ${lang}")
      set(CMAKE_${lang}_COMPILER_LAUNCHER ${CCACHE_PROGRAM} CACHE STRING "")
    endif()
  endforeach()
endif()


project(yggdrasil_cxx
    VERSION 0.0.1
    DESCRIPTION "yggdrasil - An experiment in scalable routing as an encrypted IPv6 overlay network"
    LANGUAGES ${LANGS}
  )

option(BUILD_TESTS "enable building of unit tests?" ON)

add_subdirectory(external)

add_library(yggdrasil_lib INTERFACE)
target_include_directories(yggdrasil_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(yggdrasil_lib INTERFACE fmt::fmt spdlog::spdlog uring)

add_executable(yggdrasil
  src/config.cpp
  src/event_base.cpp
  src/cryptography.cpp
  src/sockaddr.cpp
  src/main.cpp
)
target_link_libraries(yggdrasil PUBLIC yggdrasil_lib)


if(BUILD_TESTS)
  add_subdirectory(test)
endif()
  
