cmake_minimum_required(VERSION 3.1)
project(KDEBASE_APPS)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
        "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options("$<$<CONFIG:DEBUG>:-Wall;-Wextra;-pedantic>")
endif()

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )

include(CTest)
include(CTestConfig.cmake)

#search packages used by KDE
find_package(KDE4 4.10.60 REQUIRED)
include (KDE4Defaults)

find_package(ZLIB)
set_package_properties(ZLIB PROPERTIES DESCRIPTION "Support for gzip compressed files and data streams"
                       URL "http://www.zlib.net"
                       TYPE REQUIRED
                       PURPOSE "Required for building Konqueror"
                      )

include(ConfigureChecks.cmake)
configure_file (config-apps.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-apps.h )
include_directories (${CMAKE_CURRENT_BINARY_DIR})

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions (-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/lib/konq ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})

add_subdirectory( lib )
add_subdirectory( dolphin )
add_subdirectory( kdialog )
add_subdirectory( keditbookmarks )
add_subdirectory( konqueror )
add_subdirectory( kfind )
macro_optional_add_subdirectory( plasma )
add_subdirectory( konq-plugins )
add_subdirectory( kcolorchooser )
add_subdirectory( kcharselect )

if(UNIX)
  add_subdirectory( kdf )
endif(UNIX)

if ( Q_WS_MAC )
  add_subdirectory( kdepasswd )
endif ( Q_WS_MAC )

if ( Q_WS_X11 )
  add_subdirectory( kdepasswd )
endif ( Q_WS_X11 )
macro_optional_add_subdirectory( doc )

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

