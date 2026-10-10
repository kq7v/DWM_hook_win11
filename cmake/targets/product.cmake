file( GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp" )

add_library( dwm_hook SHARED ${PROJECT_SOURCES} )
target_include_directories( dwm_hook PRIVATE src )
target_compile_definitions( dwm_hook PRIVATE
    WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE _WIN32_WINNT=0x0A00
)
target_compile_options( dwm_hook PRIVATE /W4 /WX /permissive- /utf-8 )
target_link_libraries( dwm_hook PRIVATE imgui minhook libhat::libhat dxgi user32 shcore )
