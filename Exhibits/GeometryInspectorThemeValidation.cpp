/*============================================================================================================================================
                                                  GEOMETRYINSPECTORTHEMEVALIDATION.CPP
============================================================================================================================================*/
// Validate that geometry card surfaces use the currently selected UI theme.

#include "Engine/Editor/Inspectors/GeometryInspectorShared.h"

#include <imgui.h>

#include <array>
#include <cstdio>

int main()
{
    using namespace Frontier;
    ImGuiContext* Session = ImGui::CreateContext();
    ImGuiIO& Io = ImGui::GetIO();
    Io.IniFilename = nullptr;
    Io.DisplaySize = ImVec2(400.0f, 300.0f);
    Io.DeltaTime = 1.0f / 60.0f;
    unsigned char* FontPixels = nullptr;
    int AtlasWidth = 0;
    int AtlasHeight = 0;
    Io.Fonts->GetTexDataAsRGBA32(&FontPixels, &AtlasWidth, &AtlasHeight);

    ThemeRepository Theme {};
    const std::array<ThemePreset, 3u> Presets = {{ ThemePreset::Oled, ThemePreset::Dim, ThemePreset::Light }};
    bool Passed = true;
    ImU32 PriorFill = 0u;
    for (const ThemePreset Preset : Presets)
    {
        Theme.ConfigureTheme(Preset);
        ControlAppearance::ConfigureTheme(Theme);
        const ColorRgba Expected = Theme.QuerySurfaceSet().SectionBackground.ToRgba8();
        const ImU32 ExpectedFill = IM_COL32(Expected.Red, Expected.Green, Expected.Blue, Expected.Alpha);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(300.0f, 200.0f));
        ImGui::Begin("Theme test", nullptr, ImGuiWindowFlags_NoDecoration);
        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        const int FirstVertex = DrawList->VtxBuffer.Size;
        {
            PixelSpace Surface(DrawList, { 0.0f, 0.0f });
            GeometryUi::ConstructGeomCard(Surface, { 20.0f, 20.0f, 200.0f, 100.0f });
        }
        const bool Match = DrawList->VtxBuffer.Size > FirstVertex && DrawList->VtxBuffer[FirstVertex].col == ExpectedFill;
        const bool Changed = PriorFill == 0u || PriorFill != ExpectedFill;
        Passed = Passed && Match && Changed;
        PriorFill = ExpectedFill;
        ImGui::End();
        ImGui::Render();
    }
    ImGui::DestroyContext(Session);
    std::puts(Passed ? "Geometry card theme validation passed" : "Geometry card theme validation failed");
    return Passed ? 0 : 1;
}
