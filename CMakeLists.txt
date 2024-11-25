cmake_minimum_required(VERSION 3.16)

project(grade_manager LANGUAGES CXX)

set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt/lib/cmake")

# Find the Qt package
find_package(Qt6 REQUIRED COMPONENTS Widgets UiTools Sql Pdf)

set(PROJECT_SOURCES
    src/main.cpp
    src/functions/functions.cpp
    src/functions/functions.h
    src/functions/importFiles.cpp
    src/functions/printTranscript.cpp
    src/gui/mainwindow.ui
    src/gui/resultswindow.ui
    src/gui/addeditwindow.ui
    src/gui/confirmationwindow.ui
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

add_subdirectory(third_party/QXlsx)

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::UiTools Qt6::Sql QXlsx Qt6::Pdf)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
