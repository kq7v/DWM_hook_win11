#include <core/input/input.hpp>

namespace core {
    namespace {
        c_input instance;
    }

    c_input* const input = &instance;

    HRESULT c_input::initialize( HMODULE module ) {
        if( thread ) {
            return S_FALSE;
        }

        {
            std::lock_guard lock( mutex );
            pending = { };
            panel = { };
            owned.fill( false );
            insert_down = false;
        }

        stop_event = CreateEventW( nullptr, TRUE, FALSE, nullptr );
        if( !stop_event ) {
            return HRESULT_FROM_WIN32( GetLastError( ) );
        }

        ready_event = CreateEventW( nullptr, TRUE, FALSE, nullptr );
        thread = ready_event ? CreateThread( nullptr, 0, run, module, 0, nullptr ) : nullptr;

        HRESULT hr = thread ? S_OK : HRESULT_FROM_WIN32( GetLastError( ) );

        if( SUCCEEDED( hr ) ) {
            const auto wait = WaitForSingleObject( ready_event, 3000 );

            hr = wait == WAIT_OBJECT_0 ? result.load( )
                                       : HRESULT_FROM_WIN32( wait == WAIT_TIMEOUT ? ERROR_TIMEOUT : GetLastError( ) );
        }

        if( FAILED( hr ) ) {
            shutdown( );
        }

        return hr;
    }

    void c_input::shutdown( ) {
        if( stop_event ) {
            SetEvent( stop_event );
        }

        if( thread ) {
            WaitForSingleObject( thread, INFINITE );
        }

        for( auto handle : { thread, stop_event, ready_event } ) {
            if( handle ) {
                CloseHandle( handle );
            }
        }

        thread = stop_event = ready_event = nullptr;
        release( );
    }
}
