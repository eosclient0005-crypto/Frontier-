/*============================================================================================================================================
                                                  GEOMETRYINSPECTORTHEMEVALIDATION.CPP
============================================================================================================================================*/
// Validate geometry card themes and the shared inspector page strip.

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
        // The first vertices of the strip are the previous button's themed fill.
        const int StripVertex = DrawList->VtxBuffer.Size;
        PixelSpace StripSurface(DrawList, { 0.0f, 0.0f });
        const std::array<const char*, 2u> StripPages = {{ "Overview", "Geometry" }};
        uint32_t StripPage = 0u;
        bool StripSwitched = false;
        (void)InspectorPrimitives::ConstructPageStrip(StripSurface, 10.0f, 130.0f, 250.0f, StripPages, StripPage, {}, StripSwitched);
        const ColorRgba ExpectedStep = ControlAppearance::Colors().SectionSurface.ToRgba8();
        const ImU32 StepFill = IM_COL32(ExpectedStep.Red, ExpectedStep.Green, ExpectedStep.Blue, ExpectedStep.Alpha);
        const bool StripMatch = DrawList->VtxBuffer.Size > StripVertex && DrawList->VtxBuffer[StripVertex].col == StepFill;
        if (!Match || !Changed || !StripMatch) std::printf("FAIL: theme or strip color for preset %u\n", static_cast<unsigned>(Preset));
        Passed = Passed && Match && Changed && StripMatch;
        PriorFill = ExpectedFill;
        ImGui::End();
        ImGui::Render();
    }

    // Exercise real strip hit-testing, wrapping, disabled input and large page counts.
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(390.0f, 290.0f));
    ImGui::Begin("Page strip test", nullptr, ImGuiWindowFlags_NoDecoration);
    PixelSpace Surface(ImGui::GetWindowDrawList(), { 0.0f, 0.0f });
    const std::array<const char*, 3u> Pages = {{ "Overview", "Geometry", "LOD" }};
    PaneInputReading Input {};
    uint32_t Active = 0u;
    bool Switched = false;
    auto Run = [&](std::span<const char* const> Labels)
    {
        (void)InspectorPrimitives::ConstructPageStrip(Surface, 10.0f, 10.0f, 300.0f, Labels, Active, Input, Switched);
    };
    auto Check = [&](bool Condition, const char* Name)
    {
        if (!Condition) std::printf("FAIL: %s\n", Name);
        Passed = Passed && Condition;
    };
    Input.PrimaryPointer = { 20.0f, 25.0f, false, true, false, true };
    Run(Pages);
    Check(Active == 2u && Switched, "previous wraps from first to last");
    // Counter space is measured using the widest N / N label, as in the control.
    const float CountWidth = std::max(28.0f, WorkspaceAppearance::MeasureWeightedTextWidth(Surface, FontWeightClass::Regular, 10.5f, "3 / 3"));
    Input.PrimaryPointer.X = 310.0f - CountWidth - 6.0f - 14.0f;
    Run(Pages);
    Check(Active == 0u && Switched, "next wraps from last to first");
    const float FirstWidth = std::min(134.0f, 46.0f + WorkspaceAppearance::MeasureWeightedTextWidth(Surface, FontWeightClass::Regular, 12.0f, Pages[0]));
    Input.PrimaryPointer.X = 10.0f + 28.0f + 6.0f + 3.0f + FirstWidth + 2.0f + 10.0f;
    Run(Pages);
    Check(Active == 1u && Switched, "visible pill selects its page");
    Active = 0u;
    Input.PrimaryPointer.Enabled = false;
    Run(Pages);
    Check(Active == 0u && !Switched, "disabled pointer does not change page");
    Input.PrimaryPointer.Enabled = true;
    Active = 0u;
    Run(std::span<const char* const>(Pages.data(), 1u));
    Check(Active == 0u && !Switched, "single page does not switch");
    Run(std::span<const char* const>());
    Check(!Switched, "empty page list does not switch");
    std::array<const char*, 20u> ManyPages {};
    ManyPages.fill("Page");
    Active = 19u;
    Input.PrimaryPointer.Pressed = false;
    Run(ManyPages);
    Check(Active == 19u && !Switched, "more than sixteen pages remain addressable");
    ImGui::End();
    ImGui::Render();
    ImGui::DestroyContext(Session);
    std::puts(Passed ? "Geometry card theme and page strip validation passed" : "Geometry card theme or page strip validation failed");
    return Passed ? 0 : 1;
}
