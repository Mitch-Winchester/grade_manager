cmake_minimum_required(VERSION 3.16)

project(MyQtProject LANGUAGES CXX)

set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt/lib/cmake")

# Find the Qt package
find_package(Qt6 REQUIRED COMPONENTS Widgets UiTools)

add_executable(${PROJECT_NAME} src/main.cpp)

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::UiTools)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
