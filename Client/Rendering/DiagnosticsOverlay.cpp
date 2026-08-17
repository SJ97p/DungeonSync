#include "DiagnosticsOverlay.h"

#include <algorithm>

using Microsoft::WRL::ComPtr;

namespace DungeonSync::Rendering
{
    bool DiagnosticsOverlay::Initialize(
        IDXGISwapChain& swapChain,
        std::uint32_t width,
        std::uint32_t height)
    {
        width_ = static_cast<float>(width);
        height_ = static_cast<float>(height);

        HRESULT result = D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED,
            factory_.ReleaseAndGetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        result = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(
                writeFactory_.ReleaseAndGetAddressOf()));

        if (FAILED(result))
        {
            return false;
        }

        ComPtr<IDXGISurface> backBufferSurface;

        result = swapChain.GetBuffer(
            0,
            IID_PPV_ARGS(
                backBufferSurface.ReleaseAndGetAddressOf()));

        if (FAILED(result))
        {
            return false;
        }

        const D2D1_RENDER_TARGET_PROPERTIES properties =
            D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(
                    DXGI_FORMAT_UNKNOWN,
                    D2D1_ALPHA_MODE_PREMULTIPLIED));

        result = factory_->CreateDxgiSurfaceRenderTarget(
            backBufferSurface.Get(),
            &properties,
            renderTarget_.ReleaseAndGetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        result = writeFactory_->CreateTextFormat(
            L"Consolas",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            16.0F,
            L"en-us",
            textFormat_.ReleaseAndGetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        textFormat_->SetWordWrapping(
            DWRITE_WORD_WRAPPING_NO_WRAP);

        result = renderTarget_->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::White),
            textBrush_.ReleaseAndGetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        result = renderTarget_->CreateSolidColorBrush(
            D2D1::ColorF(0.02F, 0.03F, 0.05F, 0.82F),
            backgroundBrush_.ReleaseAndGetAddressOf());

        return SUCCEEDED(result);
    }

    void DiagnosticsOverlay::Draw(
        std::wstring_view text) noexcept
    {
        if (text.empty() || renderTarget_ == nullptr)
        {
            return;
        }

        constexpr float Padding = 14.0F;
        constexpr float PanelWidth = 520.0F;
        constexpr float PanelHeight = 255.0F;

        const D2D1_RECT_F panelRectangle =
            D2D1::RectF(
                Padding,
                Padding,
                (std::min)(width_ - Padding, PanelWidth),
                (std::min)(height_ - Padding, PanelHeight));

        const D2D1_RECT_F textRectangle =
            D2D1::RectF(
                panelRectangle.left + 12.0F,
                panelRectangle.top + 10.0F,
                panelRectangle.right - 12.0F,
                panelRectangle.bottom - 10.0F);

        renderTarget_->BeginDraw();
        renderTarget_->FillRectangle(
            panelRectangle,
            backgroundBrush_.Get());

        renderTarget_->DrawTextW(
            text.data(),
            static_cast<UINT32>(text.size()),
            textFormat_.Get(),
            textRectangle,
            textBrush_.Get());

        const HRESULT result = renderTarget_->EndDraw();

        if (result == D2DERR_RECREATE_TARGET)
        {
            OutputDebugStringA(
                "Diagnostics overlay render target was lost.\n");
        }
    }
}
