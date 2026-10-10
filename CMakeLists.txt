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

include( cmake/dependencies/imgui.cmake )
include( cmake/dependencies/minhook.cmake )
include( cmake/dependencies/libhat.cmake )

file( GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "src/*.cpp" )
add_library( dwm_hook SHARED ${PROJECT_SOURCES} )

target_include_directories( dwm_hook PRIVATE src )
target_compile_definitions( dwm_hook PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE _WIN32_WINNT=0x0A00 )
target_compile_options( dwm_hook PRIVATE /W4 /WX /permissive- /utf-8 )
target_link_libraries( dwm_hook PRIVATE imgui minhook libhat::libhat dxgi user32 shcore )
