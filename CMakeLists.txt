cmake_minimum_required(VERSION 3.6)
project(PubSubImproved)

set(CMAKE_CXX_STANDARD 14)

find_package(Boost)
IF (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
endif()

set(SOURCE_FILES_CTR
        ./src/SimpleCenter.cpp
        ./main/mainCenter.cpp)
set(SOURCE_FILES_PUB
        ./src/SimplePublisher.cpp
        ./main/mainPub.cpp)
set(SOURCE_FILES_SUB
        ./src/SimpleSubscriber.cpp
        ./main/mainSub.cpp)

add_executable(center ${SOURCE_FILES_CTR})
add_executable(publisher ${SOURCE_FILES_PUB})
add_executable(subscriber ${SOURCE_FILES_SUB})

target_link_libraries(center zmq pthread)
target_link_libraries(publisher zmq pthread)
target_link_libraries(subscriber zmq pthread)