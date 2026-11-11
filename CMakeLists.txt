cmake_minimum_required(VERSION 3.16)
project(DocGenerator VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(WIN32 AND DEFINED ENV{MSYSTEM})
    list(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")
endif()

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Xml PrintSupport)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/QXlsx/QXlsx/CMakeLists.txt")
    add_subdirectory(third_party/QXlsx/QXlsx)
else()
    message(FATAL_ERROR "QXlsx not found in third_party/QXlsx")
endif()

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/XlsxReader.cpp
    src/HtmlFiller.cpp
    src/DesignatorParser.cpp
)

set(HEADERS
    include/MainWindow.h
    include/XlsxReader.h
    include/HtmlFiller.h
    include/DesignatorParser.h
)

add_executable(DocGenerator ${SOURCES} ${HEADERS})

target_link_libraries(DocGenerator PRIVATE
    Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Xml Qt5::PrintSupport
    QXlsx::QXlsx
)

target_include_directories(DocGenerator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/QXlsx/QXlsx/header
)

source_group("Source Files" FILES ${SOURCES})
source_group("Header Files" FILES ${HEADERS})

add_custom_command(TARGET DocGenerator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/templates $<TARGET_FILE_DIR:DocGenerator>/templates
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/scripts $<TARGET_FILE_DIR:DocGenerator>/scripts
)
