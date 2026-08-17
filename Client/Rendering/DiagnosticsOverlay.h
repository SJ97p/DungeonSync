#pragma once

#include <d2d1.h>
#include <dwrite.h>
#include <dxgi.h>
#include <wrl/client.h>

#include <cstdint>
#include <string_view>

namespace DungeonSync::Rendering
{
    class DiagnosticsOverlay final
    {
    public:
        DiagnosticsOverlay() = default;
        ~DiagnosticsOverlay() = default;

        DiagnosticsOverlay(const DiagnosticsOverlay&) = delete;
        DiagnosticsOverlay& operator=(const DiagnosticsOverlay&) = delete;
        DiagnosticsOverlay(DiagnosticsOverlay&&) = delete;
        DiagnosticsOverlay& operator=(DiagnosticsOverlay&&) = delete;

        [[nodiscard]] bool Initialize(
            IDXGISwapChain& swapChain,
            std::uint32_t width,
            std::uint32_t height);

        void Draw(std::wstring_view text) noexcept;

    private:
        Microsoft::WRL::ComPtr<ID2D1Factory> factory_;
        Microsoft::WRL::ComPtr<IDWriteFactory> writeFactory_;
        Microsoft::WRL::ComPtr<ID2D1RenderTarget> renderTarget_;
        Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> textBrush_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> backgroundBrush_;

        float width_{};
        float height_{};
    };
}
