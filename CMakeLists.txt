cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) 
project (mandaye_tec)

set (MANDEYE_VERSION "0.6-1-dev")
message(STATUS "Mandeye version is : ${MANDEYE_VERSION}")

set(SYSTEM_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
message("LASzip library")
# Livox SDK2 (precompiled, shared only)
set(LIVOX_SDK_DIR "" CACHE PATH "Path to Livox SDK2 installation")
find_path(LIVOX_SDK_INCLUDE_DIR livox_lidar_def.h
  HINTS ${LIVOX_SDK_DIR}/include
  PATHS /usr/local/include /usr/include
  REQUIRED)
include_directories(${LIVOX_SDK_INCLUDE_DIR})

# find shared .so (prefer typical names)
find_library(LIVOX_SDK_LIBRARY
  NAMES livox_lidar_sdk livox_lidar_sdk_shared
  HINTS ${LIVOX_SDK_DIR}/lib
  PATHS /usr/local/lib /usr/lib
  REQUIRED)

# hard-fail if CMake somehow picked a static archive
if (LIVOX_SDK_LIBRARY MATCHES "\\.a$")
  message(FATAL_ERROR
    "Found static Livox SDK (${LIVOX_SDK_LIBRARY}). "
    "You asked for shared-only. Install the shared lib (.so) and/or set LIVOX_SDK_DIR.")
endif()

message(STATUS "Using Livox SDK: ${LIVOX_SDK_LIBRARY}")
# LASzip library
add_subdirectory(3rd/LASzip)
include_directories(3rd/LASzip/include)

message("JSON")
#JSON
include_directories(3rd/json)

# Livox SDK2 (precompiled)
# The Livox SDK must be compiled and installed manually before building
# this project.  Provide the installation prefix via LIVOX_SDK_DIR or
# install to a standard system location such as /usr/local.
set(LIVOX_SDK_DIR "" CACHE PATH "Path to Livox SDK2 installation")
find_path(LIVOX_SDK_INCLUDE_DIR livox_lidar_def.h
        HINTS ${LIVOX_SDK_DIR}/include
        PATHS /usr/local/include /usr/include
        REQUIRED)
find_library(LIVOX_SDK_LIBRARY livox_lidar_sdk_static
        HINTS ${LIVOX_SDK_DIR}/lib
        PATHS /usr/local/lib /usr/lib
        REQUIRED)
include_directories(${LIVOX_SDK_INCLUDE_DIR})

include_directories(code/)

# NMEA and libserial
find_path(LIBSERIAL_INCLUDE_DIR SerialPort.h
        PATH_SUFFIXES include
        PATHS /usr /usr/local /usr/include/libserial)
message(${LIBSERIAL_INCLUDE_DIR})
find_library(LIBSERIAL_LIBRARY
        NAMES libserial.so
        PATH_SUFFIXES lib
        PATHS /usr/lib)
include_directories(3rd/minmea ${LIBSERIAL_INCLUDE_DIR})
add_library(minea STATIC 3rd/minmea/minmea.c)

# get git info
execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Generate the header file
configure_file(
        ${CMAKE_SOURCE_DIR}/code/compilation_constants.h.in
        ${CMAKE_BINARY_DIR}/compilation_constants.h
        @ONLY
)
# Include the generated file in the project
include_directories(${CMAKE_BINARY_DIR})
#executable
add_executable(control_program code/main.cpp code/gnss.cpp code/LivoxClient.cpp code/FileSystemClient.cpp code/save_laz.cpp
        code/utils/TimeStampReceiver.cpp code/publisher.cpp code/utils/logger.cpp
        code/utils/network.cpp)
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -latomic")

target_link_libraries(control_program pthread ${LIVOX_SDK_LIBRARY} atomic laszip ${LIBSERIAL_LIBRARY} minea zmq)

add_library(mandeye_core SHARED code/main.cpp code/gnss.cpp code/LivoxClient.cpp code/FileSystemClient.cpp code/save_laz.cpp
        code/utils/TimeStampReceiver.cpp code/publisher.cpp code/utils/logger.cpp
        code/utils/network.cpp)
target_link_libraries(mandeye_core pthread ${LIVOX_SDK_LIBRARY} atomic laszip ${LIBSERIAL_LIBRARY} minea zmq)
target_compile_definitions(mandeye_core PRIVATE MANDEYE_LIBRARY)
# Define install directories
install(TARGETS control_program
        RUNTIME DESTINATION /opt/mandeye/)


# Packaging
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "mandeye-tec")
set(CPACK_PACKAGE_VERSION "${MANDEYE_VERSION}-${GIT_HASH}-${MANDEYE_HARDWARE_HEADER}-${SYSTEM_ARCH}")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "rapidjson-dev,libserial-dev, libzmq3-dev")
include(CPack)

