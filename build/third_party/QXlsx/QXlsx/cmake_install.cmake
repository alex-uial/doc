# Install script for directory: /workspace/third_party/QXlsx/QXlsx

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/workspace/build/third_party/QXlsx/QXlsx/libQXlsxQt5.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/QXlsx" TYPE FILE FILES
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxabstractooxmlfile.h"
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxabstractsheet.h"
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxabstractsheet_p.h"
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxcellformula.h"
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxcell.h"
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxcelllocation.h"
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxcellrange.h"
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxcellreference.h"
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxchart.h"
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxchartsheet.h"
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxconditionalformatting.h"
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxdatavalidation.h"
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxdatetype.h"
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxdocument.h"
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxformat.h"
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxglobal.h"
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxrichstring.h"
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxworkbook.h"
    "/workspace/third_party/QXlsx/QXlsx/header/xlsxworksheet.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "devel" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/QXlsxQt5/QXlsxQt5Targets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/QXlsxQt5/QXlsxQt5Targets.cmake"
         "/workspace/build/third_party/QXlsx/QXlsx/CMakeFiles/Export/9160ef171b5927dbe66bf41de9e1c9c5/QXlsxQt5Targets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/QXlsxQt5/QXlsxQt5Targets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/QXlsxQt5/QXlsxQt5Targets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/QXlsxQt5" TYPE FILE FILES "/workspace/build/third_party/QXlsx/QXlsx/CMakeFiles/Export/9160ef171b5927dbe66bf41de9e1c9c5/QXlsxQt5Targets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^()$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/QXlsxQt5" TYPE FILE FILES "/workspace/build/third_party/QXlsx/QXlsx/CMakeFiles/Export/9160ef171b5927dbe66bf41de9e1c9c5/QXlsxQt5Targets-noconfig.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/QXlsxQt5" TYPE FILE FILES
    "/workspace/build/third_party/QXlsx/QXlsx/QXlsxQt5Config.cmake"
    "/workspace/build/third_party/QXlsx/QXlsx/QXlsxQt5ConfigVersion.cmake"
    )
endif()

