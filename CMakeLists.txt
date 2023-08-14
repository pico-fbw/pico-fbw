cmake_minimum_required(VERSION 3.13)

project(pico-fbw_configurator CXX C)
set(CMAKE_CXX_STANDARD 17)

# picolite (modified picotool) is a required dependency but compiled in
add_subdirectory(picolite)

# Required Qt component libraries
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

# Include the generated moc files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Include source files
set(SOURCES
    app.pro
    main.cpp
    resources/resources.qrc
    window.cpp
)

# Build the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    picolite
    Qt5::Core Qt5::Gui Qt5::Widgets
)
