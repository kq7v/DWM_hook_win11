#include <sdk/hooks/hooks.hpp>

namespace {
    DWORD WINAPI start( void* ) {
        if( !sdk::hooks->initialize( ) ) {
            OutputDebugStringW( L"dwm_hook: initialization failed\n" );
        }

        return 0;
    }
}

BOOL WINAPI DllMain( HINSTANCE, DWORD reason, LPVOID ) {
    if( reason == DLL_PROCESS_ATTACH ) {
        if( const auto thread = CreateThread( nullptr, 0, start, nullptr, 0, nullptr ) ) {
            CloseHandle( thread );
        }
    }

    return TRUE;
}
