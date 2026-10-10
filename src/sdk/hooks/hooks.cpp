#include <sdk/hooks/hooks.hpp>
#include <sdk/hooks/methods/methods.hpp>
#include <sdk/dwm/dwm.hpp>
#include <core/input/input.hpp>
#include <backends/dwm/dwm.hpp>
#include <features/menu/menu.hpp>
#include <MinHook.h>

namespace sdk {
    namespace {
        c_hooks instance;

        template <class F>
        bool create_hook( const void* address, F method, F& original, void*& created ) {
            if( !address ) {
                return false;
            }

            auto* target = const_cast<void*>( address );
            if( MH_CreateHook( target, reinterpret_cast<void*>( method ), reinterpret_cast<void**>( &original ) ) !=
                MH_OK ) {
                return false;
            }

            created = target;
            return true;
        }
    }

    c_hooks* const hooks = &instance;

    bool c_hooks::create( ) {
        using namespace hook_methods;

        if( !dwm->initialize( ) ) {
            return false;
        }

        const auto& native = dwm->addresses;
        return create_hook( native.end_frame, &end_frame, end_frame_original, addresses[ 0 ] ) &&
               create_hook( native.legacy_render_and_present, &legacy, legacy_original, addresses[ 1 ] ) &&
               create_hook( native.display_render_and_present, &display, display_original, addresses[ 2 ] ) &&
               create_hook( native.overlay_present, &present, present_original, addresses[ 3 ] );
    }

    void c_hooks::rollback( ) {
        for( auto*& address : addresses ) {
            if( address ) {
                MH_RemoveHook( address );
                address = nullptr;
            }
        }

        if( owns_minhook ) {
            MH_Uninitialize( );
            owns_minhook = false;
        }
    }

    HRESULT c_hooks::initialize_input( ) {
        HMODULE module = nullptr;
        if( !GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                reinterpret_cast<LPCWSTR>( &hook_methods::present ), &module
            ) ) {
            return HRESULT_FROM_WIN32( GetLastError( ) );
        }

        return core::input->initialize( module );
    }

    HRESULT c_hooks::initialize( ) {
        std::lock_guard lock( mutex );

        if( enabled ) {
            return S_FALSE;
        }

        if( GetSystemMetrics( SM_CMONITORS ) != 1 ) {
            return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
        }

        if( !installed ) {
            const auto status = MH_Initialize( );
            if( status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED ) {
                return E_FAIL;
            }
            owns_minhook = status == MH_OK;

            try {
                if( !create( ) ) {
                    rollback( );
                    return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
                }
            } catch( ... ) {
                rollback( );
                return E_OUTOFMEMORY;
            }
        }

        const auto result = initialize_input( );
        if( FAILED( result ) ) {
            if( !installed ) {
                rollback( );
            }
            return result;
        }

        backends::dwm->initialize( []( ) {
            features::menu->draw( );
        } );
        features::menu->opened = true;
        enabled = true;

        if( !installed ) {
            for( auto* address : addresses ) {
                // Limit changes to this DLL's hooks, even when MinHook was initialized elsewhere.
                if( MH_QueueEnableHook( address ) != MH_OK ) {
                    enabled = false;
                    core::input->shutdown( );
                    backends::dwm->shutdown( );
                    rollback( );
                    return E_FAIL;
                }
            }
            if( MH_ApplyQueued( ) != MH_OK ) {
                enabled = false;
                core::input->shutdown( );
                backends::dwm->shutdown( );
                rollback( );
                return E_FAIL;
            }
            installed = true;
        }

        return S_OK;
    }

    HRESULT c_hooks::shutdown( ) {
        std::lock_guard lock( mutex );
        enabled = false;
        core::input->shutdown( );
        backends::dwm->shutdown( );

        // Pinned callbacks complete a native redraw and release ImGui on the compositor thread.
        return S_OK;
    }
}
