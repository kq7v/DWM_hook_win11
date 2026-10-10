cmake_minimum_required( VERSION 3.24 )
project( dwm_hook LANGUAGES C CXX )

if( NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8 )
    message( FATAL_ERROR "Use MSVC x64" )
endif( )

set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )
set( CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>" )

set_property( GLOBAL PROPERTY USE_FOLDERS ON )
option( BUILD_TESTING "Build DWM and graphics regression checks" OFF )
option( DWM_BUILD_TOOLS "Build the test injector" OFF )

include( cmake/dependencies/imgui.cmake )
include( cmake/dependencies/minhook.cmake )
include( cmake/dependencies/libhat.cmake )
include( cmake/targets/product.cmake )

if( DWM_BUILD_TOOLS )
    add_executable( injector tools/injector/main.cpp )
    target_compile_definitions( injector PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX )
    target_compile_options( injector PRIVATE /W4 /WX /permissive- /utf-8 )
    target_link_libraries( injector PRIVATE advapi32 )
endif( )

if( BUILD_TESTING )
    enable_testing( )
    include( cmake/targets/tests.cmake )
endif( )
