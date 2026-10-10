#include <core/input/input.hpp>
#include <sdk/dwm/dwm.hpp>
#include <backends/dwm/dwm.hpp>
#include <shellscalingapi.h>

namespace core {
    void c_input::update_display( ) {
        const auto monitor = MonitorFromPoint( { 0, 0 }, MONITOR_DEFAULTTOPRIMARY );
        DEVICE_SCALE_FACTOR scale = SCALE_100_PERCENT;
        GetScaleFactorForMonitor( monitor, &scale );

        std::lock_guard lock( mutex );

        pending.scale = static_cast<float>( scale ) / 100.0f;
    }

    void c_input::redraw( bool force ) {
        if( force || backends::dwm->needs_redraw( ) ) {
            sdk::dwm->request_frame( );
        }
    }

    DWORD c_input::poll( HMODULE module ) {
        const auto previous_dpi = SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 );
        update_display( );

        POINT position { -1000, -1000 };
        GetCursorPos( &position );

        {
            std::lock_guard lock( mutex );
            pending.position = position;
        }

        MSG message { };
        PeekMessageW( &message, nullptr, 0, 0, PM_NOREMOVE );
        const HHOOK hook = SetWindowsHookExW( WH_MOUSE_LL, mouse_hook, module, 0 );
        const HHOOK keys = hook ? SetWindowsHookExW( WH_KEYBOARD_LL, keyboard_hook, module, 0 ) : nullptr;

        result = hook && keys ? S_OK : HRESULT_FROM_WIN32( GetLastError( ) );
        SetEvent( ready_event );

        ULONGLONG last_redraw = 0, last_display = GetTickCount64( );

        while( hook && keys ) {
            const auto wait = MsgWaitForMultipleObjectsEx( 1, &stop_event, 16, QS_ALLINPUT, MWMO_INPUTAVAILABLE );

            if( wait == WAIT_OBJECT_0 || wait == WAIT_FAILED ) {
                break;
            }

            while( PeekMessageW( &message, nullptr, 0, 0, PM_REMOVE ) ) {
                TranslateMessage( &message );
                DispatchMessageW( &message );
            }

            const auto now = GetTickCount64( );

            if( now - last_display >= 1000 ) {
                update_display( );
                last_display = now;
            }

            if( now - last_redraw >= 16 ) {
                redraw( );
                last_redraw = now;
            }
        }

        if( keys ) {
            UnhookWindowsHookEx( keys );
        }

        if( hook ) {
            UnhookWindowsHookEx( hook );
        }

        redraw( true );

        if( previous_dpi ) {
            SetThreadDpiAwarenessContext( previous_dpi );
        }

        return 0;
    }

    DWORD WINAPI c_input::run( void* module ) {
        return input->poll( static_cast<HMODULE>( module ) );
    }
}
