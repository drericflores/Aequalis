
cmake_minimum_required(VERSION 3.16)
project(AequalisCPP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt's code generators (required for Q_OBJECT, signals/slots)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Prefer Qt6, fall back to Qt5
find_package(Qt6 COMPONENTS Core Widgets Concurrent QUIET)
if (Qt6_FOUND)
  set(QT_PACKAGE Qt6)
else()
  find_package(Qt5 COMPONENTS Core Widgets Concurrent REQUIRED)
  set(QT_PACKAGE Qt5)
endif()

file(GLOB AEQUALIS_HEADERS
  include/Aequalis/*.hpp
)

add_executable(aequalis
  src/main.cpp
  src/MainWindow.cpp
  src/DiffModel.cpp
  src/Worker.cpp
  src/Scanner.cpp
  ${AEQUALIS_HEADERS}
)

target_include_directories(aequalis PRIVATE include)

target_link_libraries(aequalis PRIVATE ${QT_PACKAGE}::Core ${QT_PACKAGE}::Widgets ${QT_PACKAGE}::Concurrent)

if (UNIX)
  target_compile_definitions(aequalis PRIVATE AEQ_UNIX)
endif()
