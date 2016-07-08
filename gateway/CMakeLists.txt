cmake_minimum_required(VERSION 2.6)
project (GATEWAY)

#set compiler
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

# set output directory
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# set compiler flag
SET(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -Wall)
SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -Wall)

SET(CMAKE_C_FLAGS_DEBUG "$ENV{CFLAGS} -o0 -Wall  -g -ggdb")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -o0  -Wall -g -ggdb")

SET(CMAKE_C_FLAGS_RELEASE "$ENV{CFLAGS} -o3 -Wall")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -o3 -Wall")

# add system dependency 
set(LOCAL_INCLUDE_DIR /usr/local/include)
set(OPT_INCLUDE_DIR /opt/local/include)
set(LIB64_DIR /lib64)
set(USR_LIB64_DIR  /usr/lib64)
set(LOCAL_LIB64_DIR /usr/local/lib64)

include_directories(${LOCAL_INCLUDE_DIR})
include_directories(${OPT_INCLUDE_DIR})
include_directories(${LIB64_DIR})
include_directories(${USR_LIB64_DIR})
include_directories(${LOCAL_LIB64_DIR})

# manager project[gateway] directory tree
set(CACHE_DIR ${PROJECT_SOURCE_DIR}/src/cache/)
set(CONFIG_DIR ${PROJECT_SOURCE_DIR}/src/config/)
set(CONNECTION_DIR ${PROJECT_SOURCE_DIR}/src/connection/)
set(LOG_DIR ${PROJECT_SOURCE_DIR}/src/log/)
set(NETWORK_DIR ${PROJECT_SOURCE_DIR}/src/network/)
set(SERVER_DIR ${PROJECT_SOURCE_DIR}/src/server/)
set(SINGLETON_DIR ${PROJECT_SOURCE_DIR}/src/singleton/)
set(TIMER_DIR ${PROJECT_SOURCE_DIR}/src/timer/)
set(GLOBAL_DIR ${PROJECT_SOURCE_DIR}/src/global/)

include_directories(${CACHE_DIR})
include_directories(${CONFIG_DIR})
include_directories(${CONNECTION_DIR})
include_directories(${LOG_DIR})
include_directories(${NETWORK_DIR})
include_directories(${SERVER_DIR})
include_directories(${SINGLETON_DIR})
include_directories(${TIMER_DIR})
include_directories(${GLOBAL_DIR})

FILE(GLOB_RECURSE CACHE_SRC  ${CACHE_DIR}/*.cpp)
FILE(GLOB_RECURSE CONFIG_SRC  ${CONFIG_DIR}/*.cpp)
FILE(GLOB_RECURSE CONNECTION_SRC  ${CONNECTION_DIR}/*.cpp)
FILE(GLOB_RECURSE LOG_SRC  ${LOG_DIR}/*.cpp)
FILE(GLOB_RECURSE NETWORK_SRC  ${NETWORK_DIR}/*.cpp)
FILE(GLOB_RECURSE SERVER_SRC  ${SERVER_DIR}/*.cpp)
FILE(GLOB_RECURSE SINGLETON_SRC  ${SINGLETON_DIR}/*.cpp)
FILE(GLOB_RECURSE TIMER_SRC  ${TIMER_DIR}/*.cpp)
FILE(GLOB_RECURSE GLOBAL_SRC  ${GLOBAL_DIR}/*.cpp)

# add library
set(BOOST_INCLUDE /home/dba/boost/boost_1_61/include/)
include_directories(${BOOST_INCLUDE})

#find_library(BOOST_LIBRARY NAMES boost_system boost_thread PATHS /home/dba/boost/boost_1_61/lib/)
#find_library(BOOST_SYSTEM     NAMES boost_system     PATHS /home/dba/boost/boost_1_61/lib/)
#find_library(BOOST_THREAD     NAMES boost_thread     PATHS /home/dba/boost/boost_1_61/lib/)
#find_library(BOOST_FILESYSTEM NAMES boost_filesystem PATHS /home/dba/boost/boost_1_61/lib/)
#find_library(BOOST_DATETIME   NAMES boost_date_time   PATHS /home/dba/boost/boost_1_61/lib/)
link_directories( . /home/dba/boost/boost_1_61/lib/)

add_executable(Gateway  ${SERVER_SRC}
                        ${CACHE_SRC}
                        ${CONFIG_SRC}
                        ${CONNECTION_SRC}
                        ${LOG_SRC}
                        ${NETWORK_SRC}
                        ${SINGLETON_SRC}
                        ${TIMER_SRC}
                        ${GLOBAL_DIR})

#target_link_libraries(Gateway
#                      ${BOOST_SYSTEM}
#                      ${BOOST_THREAD}
#                      ${BOOST_FILESYSTEM}
#                      ${BOOST_DATETIME}
#)
target_link_libraries(Gateway
                      boost_system
                      boost_thread
                      boost_filesystem
                      boost_date_time
                      pthread
                    )

# test project
ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/test)
