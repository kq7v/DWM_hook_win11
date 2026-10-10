#include <sdk/hooks/hooks.hpp>
#include <common/diagnostics/log.hpp>

namespace {
    DWORD WINAPI start( void* ) {
        diagnostics::log( "DLL worker started" );
        const auto result = sdk::hooks->initialize( );
        diagnostics::log( "hooks initialize = 0x%08lX", static_cast<unsigned long>( result ) );

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
