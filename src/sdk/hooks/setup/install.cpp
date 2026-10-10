#include <sdk/hooks/hooks.hpp>
#include <sdk/hooks/methods/methods.hpp>
#include <sdk/dwm/dwm.hpp>
#include <MinHook.h>

namespace sdk {
    namespace {
        template <class F>
        bool create_hook( const void* address, F method, F& original ) {
            return MH_CreateHook(
                const_cast<void*>( address ), reinterpret_cast<void*>( method ), reinterpret_cast<void**>( &original )
            ) == MH_OK;
        }
    }

    bool c_hooks::install( ) {
        using namespace hook_methods;

        if( installed ) {
            return true;
        }

        if( !dwm->initialize( ) ) {
            return false;
        }

        if( !GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                reinterpret_cast<LPCWSTR>( &present ), &module
            ) || MH_Initialize( ) != MH_OK ) {
            return false;
        }

        const auto& native = dwm->addresses;

        installed = create_hook( native.end_frame, &end_frame, end_frame_original ) &&
                    create_hook( native.legacy_render_and_present, &legacy, legacy_original ) &&
                    create_hook( native.display_render_and_present, &display, display_original ) &&
                    create_hook( native.overlay_present, &present, present_original ) &&
                    MH_EnableHook( MH_ALL_HOOKS ) == MH_OK;

        if( !installed ) {
            MH_Uninitialize( );
        }

        return installed;
    }
}
