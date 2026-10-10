#include <sdk/hooks/hooks.hpp>
#include <sdk/hooks/detail/callbacks.hpp>
#include <sdk/dwm/dwm.hpp>
#include <backends/imgui_impl_dwm.hpp>
#include <core/input/input.hpp>
#include <features/menu/menu.hpp>
#include <d3d11sdklayers.h>
#include <wrl/client.h>
#include <imgui.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using Microsoft::WRL::ComPtr;
constexpr UINT width = 256, height = 192;
ComPtr<ID3D11Device> device;
ComPtr<ID3D11DeviceContext> context;
ComPtr<ID3D11InfoQueue> messages;
std::array<ComPtr<ID3D11Texture2D>, 3> buffers;
std::array<ComPtr<ID3D11RenderTargetView>, 3> views;
std::vector<unsigned> desktop( width* height ), scanout;
unsigned tick = 0, end_calls = 0, present_calls = 0, invalidations = 0;
bool dirty = false, reject = false, skip_submission = false, offscreen = false, fail_end = false;
UINT native_flags = 0, observed_flags = 0;
RECT panel { };

void require( bool value, const char* message ) {
    if( !value ) {
        std::fprintf( stderr, "FAIL: %s\n", message );
        std::exit( 1 );
    }
}

ID3D11Texture2D* back( ) {
    return buffers[ tick % 3 ].Get( );
}

std::vector<unsigned> read( ID3D11Texture2D* texture ) {
    D3D11_TEXTURE2D_DESC desc { };
    texture->GetDesc( &desc );
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = desc.MiscFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging;
    require( SUCCEEDED( device->CreateTexture2D( &desc, nullptr, &staging ) ), "create staging" );
    context->CopyResource( staging.Get( ), texture );
    D3D11_MAPPED_SUBRESOURCE mapped { };
    require( SUCCEEDED( context->Map( staging.Get( ), 0, D3D11_MAP_READ, 0, &mapped ) ), "read pixels" );
    std::vector<unsigned> result( width * height );
    for( UINT y = 0; y < height; ++y ) {
        std::memcpy( result.data( ) + y * width, static_cast<char*>( mapped.pData ) + y * mapped.RowPitch, width * 4 );
    }
    context->Unmap( staging.Get( ), 0 );
    return result;
}

namespace sdk {
    namespace {
        c_dwm dwm_instance;
        c_hooks hooks_instance;
    }

    c_dwm* const dwm = &dwm_instance;
    c_hooks* const hooks = &hooks_instance;

    void c_dwm::invalidate( void* ) const noexcept {
        dirty = true;
        ++invalidations;
    }

    HRESULT c_dwm::frame_texture( void*, ID3D11Texture2D** result ) const noexcept {
        *result = nullptr;
        if( offscreen ) {
            return S_FALSE;
        }
        *result = back( );
        ( *result )->AddRef( );
        return S_OK;
    }
}

namespace core {
    namespace {
        c_input instance;
    }

    c_input* const input = &instance;

    input_frame c_input::consume( ) {
        return { };
    }

    void c_input::set_panel( RECT bounds ) {
        ::panel = bounds;
    }
}

namespace features {
    namespace {
        c_menu instance;
    }

    c_menu* const menu = &instance;

    void c_menu::draw( ) {
        const float x = 20.0f + static_cast<float>( tick % 37 );
        ImGui::GetBackgroundDrawList( )->AddRectFilled( { x, 30 }, { x + 60, 90 }, IM_COL32( 240, 30, 60, 255 ) );
    }
}

HRESULT __fastcall native_end( void* ) {
    ++end_calls;
    if( fail_end ) {
        return E_FAIL;
    }
    return S_OK;
}

HRESULT __fastcall native_present(
    void*,
    void*,
    UINT flags,
    const sdk::hook_callbacks::dirty_rects&,
    UINT interval,
    bool* submitted,
    bool legacy
) {
    ++present_calls;
    observed_flags = flags;
    require( interval == 1 && legacy, "preserve native arguments" );
    *submitted = !reject && !skip_submission && !( flags & DXGI_PRESENT_TEST );
    if( *submitted ) {
        scanout = read( back( ) );
    }
    return reject ? E_FAIL : S_OK;
}

HRESULT __fastcall native_compose( void* ) {
    if( dirty ) {
        for( auto& pixel : desktop ) {
            pixel = 0xff102000 + tick;
        }
        context->UpdateSubresource( back( ), 0, nullptr, desktop.data( ), width * 4, 0 );
        dirty = false;
    }
    auto* view = views[ tick % 3 ].Get( );
    context->OMSetRenderTargets( 1, &view, nullptr );
    const auto result = sdk::hook_callbacks::end_frame( &tick );
    ComPtr<ID3D11RenderTargetView> restored;
    context->OMGetRenderTargets( 1, &restored, nullptr );
    require( restored.Get( ) == view, "restore DWM's existing render target and context state" );
    if( FAILED( result ) ) {
        return result;
    }
    const sdk::hook_callbacks::dirty_rects rectangles { };
    bool submitted = false;
    return sdk::hook_callbacks::present( &tick, &tick, native_flags, rectangles, 1, &submitted, true );
}

void frame( ) {
    sdk::hook_callbacks::compose( &tick );
    ++tick;
}

void check_menu( ) {
    bool found = false;
    for( UINT y = 0; y < height; ++y ) {
        for( UINT x = 0; x < width; ++x ) {
            const POINT point { static_cast<LONG>( x ), static_cast<LONG>( y ) };
            const auto pixel = scanout[ y * width + x ];
            if( !PtInRect( &panel, point ) ) {
                if( pixel != desktop[ y * width + x ] ) {
                    std::fprintf(
                        stderr, "tick=%u pixel(%u,%u)=%08x expected=%08x panel=%ld,%ld,%ld,%ld\n", tick, x, y, pixel,
                        desktop[ y * width + x ], panel.left, panel.top, panel.right, panel.bottom
                    );
                }
                require( pixel == desktop[ y * width + x ], "native redraw removes old menu pixels" );
            } else if( pixel != desktop[ y * width + x ] ) {
                found = true;
            }
        }
    }
    require( found, "ImGui pixels reach scanout" );
}

int main( ) {
    require(
        SUCCEEDED( D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION, &device,
            nullptr, &context
        ) ),
        "create WARP debug device"
    );
    require( SUCCEEDED( device.As( &messages ) ), "D3D debug queue" );
    D3D11_TEXTURE2D_DESC desc { };
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    for( unsigned i = 0; i < 3; ++i ) {
        require(
            SUCCEEDED( device->CreateTexture2D( &desc, nullptr, &buffers[ i ] ) ) &&
                SUCCEEDED( device->CreateRenderTargetView( buffers[ i ].Get( ), nullptr, &views[ i ] ) ),
            "native buffers"
        );
    }
    sdk::hook_callbacks::compose_original = native_compose;
    sdk::hook_callbacks::end_frame_original = native_end;
    sdk::hook_callbacks::present_original = native_present;
    backends::dwm->initialize( []( ) {
        features::menu->draw( );
    } );
    sdk::hooks->enabled = true;
    for( unsigned i = 0; i < 96; ++i ) {
        frame( );
        check_menu( );
    }
    require( end_calls == 96 && present_calls == 96 && invalidations == 96, "one native pass per frame" );
    native_flags = DXGI_PRESENT_DO_NOT_SEQUENCE;
    frame( );
    require( observed_flags == native_flags, "do not modify native presentation flags" );
    native_flags = 0;
    features::menu->opened = false;
    reject = true;
    frame( );
    require( backends::dwm->needs_redraw( false ), "failed submission retains cleanup" );
    reject = false;
    skip_submission = true;
    frame( );
    require( backends::dwm->needs_redraw( false ), "successful call without a submission retains cleanup" );
    skip_submission = false;
    native_flags = DXGI_PRESENT_TEST;
    frame( );
    require( backends::dwm->needs_redraw( false ), "test presentation does not commit cleanup" );
    native_flags = 0;
    frame( );
    require( scanout == desktop && !backends::dwm->needs_redraw( false ), "hide restores native desktop" );
    offscreen = true;
    features::menu->opened = true;
    frame( );
    require( scanout == desktop, "offscreen frame does not receive ImGui" );
    offscreen = false;
    fail_end = true;
    const auto previous_presents = present_calls;
    frame( );
    require( present_calls == previous_presents, "failed native EndFrame does not present or draw" );
    fail_end = false;
    frame( );
    check_menu( );
    sdk::hooks->enabled = false;
    backends::dwm->shutdown( );
    frame( );
    require( scanout == desktop && !backends::dwm->needs_redraw( false ), "shutdown cleans the displayed frame" );
    for( UINT64 i = 0; i < messages->GetNumStoredMessages( ); ++i ) {
        SIZE_T size = 0;
        messages->GetMessage( i, nullptr, &size );
        std::vector<unsigned char> storage( size );
        auto* message = reinterpret_cast<D3D11_MESSAGE*>( storage.data( ) );
        messages->GetMessage( i, message, &size );
        require( message->Severity > D3D11_MESSAGE_SEVERITY_ERROR, message->pDescription );
    }
    std::puts(
        "PASS: native full redraw, 96 moving frames, borrowed views, context restoration, hide/stop and failed/test "
        "submissions"
    );
}
