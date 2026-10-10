#include <backends/dx11/renderer.hpp>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <d3d11_1.h>
#include <wrl/client.h>
#include <algorithm>
#include <utility>

namespace backends::detail {
    namespace {
        c_renderer instance;
    }

    c_renderer* const renderer = &instance;

    struct c_renderer::device_state_scope {
        c_renderer& owner;
        ComPtr<ID3DDeviceContextState> previous;

        explicit device_state_scope( c_renderer& service ) : owner( service ) {
            owner.state.context1->SwapDeviceContextState( owner.state.context_state.Get( ), &previous );
        }

        ~device_state_scope( ) {
            owner.state.context->ClearState( );
            owner.state.context1->SwapDeviceContextState( previous.Get( ), nullptr );
            ImGui::GetPlatformIO( ).Renderer_RenderState = nullptr;
        }
    };

    bool c_renderer::initialize( ID3D11Device* device ) {
        shutdown( );

        ComPtr<ID3D11Device1> device1;
        renderer_state next;
        device->GetImmediateContext( &next.context );

        if( FAILED( device->QueryInterface( IID_PPV_ARGS( &device1 ) ) ) ||
            FAILED( next.context.As( &next.context1 ) ) ) {
            return false;
        }

        const auto level = device->GetFeatureLevel( );
        const UINT flags = ( device->GetCreationFlags( ) & D3D11_CREATE_DEVICE_SINGLETHREADED )
                               ? D3D11_1_CREATE_DEVICE_CONTEXT_STATE_SINGLETHREADED
                               : 0;

        if( FAILED( device1->CreateDeviceContextState(
                flags, &level, 1, D3D11_SDK_VERSION, __uuidof( ID3D11Device ), nullptr, &next.context_state
            ) ) ||
            !ImGui_ImplDX11_Init( device, next.context.Get( ) ) ) {
            return false;
        }

        LARGE_INTEGER frequency;
        QueryPerformanceFrequency( &frequency );
        next.device = device;
        next.frequency = frequency.QuadPart;
        state = std::move( next );

        return true;
    }

    void c_renderer::update_time( ) {
        LARGE_INTEGER now;
        QueryPerformanceCounter( &now );

        const double elapsed =
            state.time ? static_cast<double>( now.QuadPart - state.time ) / static_cast<double>( state.frequency )
                       : 1.0 / 60.0;
        ImGui::GetIO( ).DeltaTime = static_cast<float>( std::clamp( elapsed, 0.001, 0.1 ) );
        state.time = now.QuadPart;
    }

    bool c_renderer::new_frame( ID3D11Texture2D* texture ) {
        if( !texture ) {
            return false;
        }

        D3D11_TEXTURE2D_DESC desc { };
        texture->GetDesc( &desc );

        if( !desc.Width || !desc.Height || desc.SampleDesc.Count != 1 || desc.ArraySize != 1 || desc.MipLevels != 1 ||
            !( desc.BindFlags & D3D11_BIND_RENDER_TARGET ) ||
            ( desc.MiscFlags & ( D3D11_RESOURCE_MISC_HW_PROTECTED | D3D11_RESOURCE_MISC_RESTRICTED_CONTENT ) ) ) {
            return false;
        }

        ComPtr<ID3D11Device> device;
        texture->GetDevice( &device );

        if( state.device.Get( ) != device.Get( ) && !initialize( device.Get( ) ) ) {
            return false;
        }

        if( state.texture != texture ) {
            ComPtr<ID3D11RenderTargetView> target;
            state.context->OMGetRenderTargets( 1, &target, nullptr );
            if( target ) {
                ComPtr<ID3D11Resource> resource;
                ComPtr<ID3D11Texture2D> bound;
                target->GetResource( &resource );
                if( FAILED( resource.As( &bound ) ) || bound.Get( ) != texture ) {
                    target.Reset( );
                }
            }

            if( !target && FAILED( device->CreateRenderTargetView( texture, nullptr, &target ) ) ) {
                return false;
            }

            state.target = std::move( target );
            state.texture = texture;
        }

        device_state_scope scope( *this );

        if( !state.fonts ) {
            if( !ImGui_ImplDX11_CreateDeviceObjects( ) ) {
                return false;
            }

            state.fonts = true;
        }

        ImGui::GetIO( ).DisplaySize = { static_cast<float>( desc.Width ), static_cast<float>( desc.Height ) };
        update_time( );
        ImGui_ImplDX11_NewFrame( );

        return true;
    }

    void c_renderer::draw( ImDrawData* data ) {
        device_state_scope scope( *this );
        auto* target = state.target.Get( );

        state.context->OMSetRenderTargets( 1, &target, nullptr );
        ImGui_ImplDX11_RenderDrawData( data );
    }

    void c_renderer::invalidate( ) {
        if( ImGui::GetIO( ).BackendRendererUserData ) {
            ImGui_ImplDX11_InvalidateDeviceObjects( );
        }

        state.fonts = false;
    }

    void c_renderer::shutdown( ) {
        if( ImGui::GetIO( ).BackendRendererUserData ) {
            ImGui_ImplDX11_Shutdown( );
        }

        state = { };
    }
}
