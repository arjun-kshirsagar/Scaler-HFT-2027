cmake_minimum_required(VERSION 3.10)
project(LimitOrderBook VERSION 1.0 LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags for performance
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra")

# Add the order book library
add_library(orderbook STATIC
    order_book.cpp
)

target_include_directories(orderbook PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Add the main executable
add_executable(order_book_test main.cpp)
target_link_libraries(order_book_test orderbook)

# Enable testing
enable_testing()
add_test(NAME OrderBookTests COMMAND order_book_test)
