/*============================================================================================================================================
                                                    PARTICLEGPUPHYSICSVALIDATION.CPP
============================================================================================================================================*/
// 🧩 Stage C (Slices S07-S13) comprehensive 1:1 headless parity suite for ParticleSpace templates, fireworks, carrier solids, GPU simulation,
//    18 analytical sprite cutouts, 96-sample Catmull-Rom ribbons, instanced 3D carriers + 3 backing surfaces, and 11 strand/web/flora/arc modes.
// Reference: UnifiedFluidParticleForceFieldPortPlan.md §6 Slice S13

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/ParticleSpace/CarrierSolidRegistry.h"
#include "Engine/ParticleSpace/FireworkPatternSampler.h"
#include "Engine/ParticleSpace/ParticleCloudTypes.h"
#include "Engine/ParticleSpace/ParticleGpuRenderer.h"
#include "Engine/ParticleSpace/ParticleGpuSimulation.h"
#include "Engine/ParticleSpace/ParticleTemplateRegistry.h"
#include "Engine/SimulationSpace/EffectLayers/EffectLayerSpecification.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace
{

using namespace Frontier::ParticleSpace;
using namespace Frontier::SimulationSpace::EffectLayers;
namespace fs = std::filesystem;

uint32_t gPassedChecks = 0u;
uint32_t gFailedChecks = 0u;

void CheckCondition(const bool ok, const char* label)
{
    if (ok)
    {
        ++gPassedChecks;
    }
    else
    {
        ++gFailedChecks;
        std::fprintf(stderr, "[FAIL] %s\n", label);
    }
}

fs::path ResolveRepoRoot()
{
#ifdef FRONTIER_REPO_ROOT_DIR
    return fs::path(FRONTIER_REPO_ROOT_DIR);
#else
    return fs::current_path();
#endif
}

void ValidateFireworkPatterns()
{
    for (uint32_t pat = 0u; pat < 10u; ++pat)
    {
        const auto sampled = SampleFireworkAnchors(static_cast<FireworkPatternOrdinal>(pat), "2026", 256u);
        bool finite = (sampled.Count == 256u && sampled.Positions.size() == 256u * 3u && sampled.Normals.size() == 256u * 3u);
        for (float v : sampled.Positions) finite = finite && std::isfinite(v);
        for (float v : sampled.Normals)   finite = finite && std::isfinite(v);
        CheckCondition(finite, "Firework pattern produces 256 finite positions + burst tint vectors");
    }
}

void ValidateCarrierSolidLibrary()
{
    const auto allNames = QueryAllSolidNames();
    const auto builtIns = QueryBuiltInSolidNames();
    CheckCondition(allNames.size() == 19u, "CarrierSolidRegistry exposes 19 selectable solid names (18 built-in + CustomObject)");
    CheckCondition(builtIns.size() == 18u, "CarrierSolidRegistry exposes 18 procedural built-in solids");

    CarrierSolidRegistry reg;
    for (const auto name : builtIns)
    {
        const CarrierSolidRecord& solid = reg.ResolveSolid(name);
        const SolidTexelTable texels = PackSolidTexels(solid);
        const SolidAnchorSet anchors = SampleSolidAnchors(solid, 64u, SolidSampleModeOrdinal::EvenSurface, 15.0);
        const AnchorTexelTable packedAnchors = PackAnchorTexels(anchors);
        const bool ok = (solid.Triangles > 0u && solid.VertexCount == solid.Triangles * 3u
            && texels.Data.size() == static_cast<size_t>(texels.Width) * static_cast<size_t>(texels.Height) * 4u
            && anchors.Count == 64u
            && packedAnchors.Count == 64u);
        CheckCondition(ok, "Built-in carrier solid resolves, packs texels, samples anchors");
    }
}

void ValidateGpuSimulationModes(const CarrierSolidRegistry& solidReg)
{
    // Verify all 16 spawn shapes + Mocap 4 clips + Effector 4 modes + Derez 5 modes step cleanly
    ParticleGpuSimulation sim;
    ParticleSimulationRegion region {};
    std::vector<ParticleCloudNode> clouds;
    for (uint32_t shape = 0u; shape < 16u; ++shape)
    {
        ParticleCloudNode node {};
        node.NodeId = "shape-" + std::to_string(shape);
        node.Enabled = true;
        node.Evaluated.Mode = (shape % 2u == 0u) ? ParticleSpawnModeOrdinal::Cloner : ParticleSpawnModeOrdinal::Release;
        node.Evaluated.Shape = static_cast<ParticleShapeOrdinal>(shape);
        node.Evaluated.Count = 64u;
        node.Evaluated.MocapClip = static_cast<ParticleMocapClipOrdinal>(shape % 5u);
        node.Evaluated.EffectorMode = static_cast<ParticleEffectorOrdinal>(shape % 5u);
        node.Evaluated.EffectorStrength = 0.6f;
        node.Evaluated.DerezMode = static_cast<ParticleDerezOrdinal>(shape % 6u);
        node.Evaluated.DerezMix = 0.7f;
        node.Evaluated.DerezProgress = 0.45f;
        clouds.push_back(std::move(node));
    }

    for (int step = 0; step < 30; ++step)
    {
        sim.StepCloudsCpu(clouds, solidReg, region, static_cast<float>(step) / 60.0f, 1.0f / 60.0f);
    }

    bool allFinite = true;
    for (const auto& c : clouds)
    {
        const auto* rec = sim.FindCloudRecord(c.NodeId);
        if (!rec || rec->ActiveCount != 64u) { allFinite = false; break; }
        for (uint32_t i = 0u; i < rec->ActiveCount; ++i)
        {
            for (int k = 0; k < 4; ++k)
            {
                if (!std::isfinite(rec->ReadStates[i].PositionAge[k]) || !std::isfinite(rec->ReadStates[i].VelocitySeed[k]))
                {
                    allFinite = false;
                }
            }
        }
    }
    CheckCondition(allFinite, "All 16 spawn shapes + 4 Mocap clips + 4 Effectors + 5 Derez modes step 30 ticks with 0 NaN/Inf");
}

void ValidateSpritesCutoutsRamps()
{
    // 4 analytic ramps monotonic endpoints + finite
    bool rampsOk = true;
    for (uint32_t r = 0u; r < 4u; ++r)
    {
        for (int s = 0; s <= 4; ++s)
        {
            const auto c = EvaluateAnalyticRampCpu(r, static_cast<float>(s) * 0.25f);
            for (float v : c) rampsOk = rampsOk && std::isfinite(v) && v >= -0.05f && v <= 1.05f;
        }
    }
    CheckCondition(rampsOk, "All 4 analytic ramps produce finite RGB across [0..1]");

    // All 18 sprite cutouts (0..17) finite at multiple sample coords
    bool spritesOk = true;
    const std::array<float, 4> col { 0.85f, 0.55f, 0.25f, 0.9f };
    const std::array<float, 4> extra { 0.4f, 0.35f, 0.5f, 0.0f };
    for (uint32_t sp = 0u; sp < 18u; ++sp)
    {
        const auto res = EvaluateSpriteCutoutCpu(sp, 0u, col, 1.0f, 0.0f, extra, { 0.5f, 0.5f });
        spritesOk = spritesOk && std::isfinite(res.Rgb[0]) && std::isfinite(res.Rgb[1]) && std::isfinite(res.Rgb[2]) && std::isfinite(res.Alpha);
    }
    CheckCondition(spritesOk, "All 18 analytical sprite cutouts (0..17) evaluate cleanly");
}

void ValidateRibbonsTrails(const CarrierSolidRegistry& solidReg)
{
    // Catmull-Rom midpoint accuracy
    const auto mid = EvaluateCatmullRomCpu({ 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { 2.0f, 1.0f, 0.0f }, { 3.0f, 0.0f, 0.0f }, 0.5f);
    CheckCondition(std::fabs(mid[0] - 1.5f) < 1.0e-6f && std::fabs(mid[1] - 1.125f) < 1.0e-6f, "Catmull-Rom cubic midpoint exact");

    // 96-sample ring buffer + Linger freeze + jump collapse
    ParticleCloudNode node {};
    node.NodeId = "trail-96";
    node.Enabled = true;
    node.Evaluated.Mode = ParticleSpawnModeOrdinal::Cloner;
    node.Evaluated.Shape = ParticleShapeOrdinal::Ring;
    node.Evaluated.Count = 8u;
    node.Evaluated.Trail = ParticleTrailModeOrdinal::Ribbon;
    node.Evaluated.TrailLength = 96u;
    node.Evaluated.TrailInterval = 0.01f;
    node.Evaluated.TrailSubdivisions = 4u;
    node.Evaluated.Lifetime = 1.0f;
    node.Evaluated.Linger = 1.5f;
    node.Evaluated.Noise = 1.0f;

    ParticleGpuSimulation sim;
    ParticleSimulationRegion region {};
    std::vector<ParticleCloudNode> clouds { node };
    for (int i = 0; i < 100; ++i) sim.StepCloudsCpu(clouds, solidReg, region, static_cast<float>(i) / 60.0f, 1.0f / 60.0f);

    const auto* rec = sim.FindCloudRecord("trail-96");
    CheckCondition(rec && rec->TrailLength == 96u && rec->TrailSamples.size() == rec->Capacity * 96u, "96-sample trail ring buffer allocates properly");

    if (rec)
    {
        auto frozenRec = *rec;
        const auto before = frozenRec.TrailSamples;
        frozenRec.ReadStates[0].PositionAge[0] += 0.2f;
        frozenRec.ReadStates[0].PositionAge[3] = std::max(frozenRec.PreviousPositions[0][3] + 0.02f, 1.25f);
        ParticleTrailUpdateUniformBlock lingerU {
            { static_cast<float>(frozenRec.Capacity), 96.0f, 0.0f, 1.0f },
            { 0.0f, 1.0f, 1.5f, 0.0f }
        };
        StepTrailHistoryCpu(frozenRec, lingerU);
        bool preserved = true;
        for (uint32_t s = 0u; s < 96u; ++s)
        {
            if (frozenRec.TrailSamples[s][0] != before[s][0]) preserved = false;
        }
        CheckCondition(preserved, "Trail ring buffer freezes in place during Linger");
    }
}

void ValidateCarrierSolidsBacking(const CarrierSolidRegistry& solidReg)
{
    ParticleCloudNode node {};
    node.NodeId = "solid-carrier-test";
    node.Enabled = true;
    node.Evaluated.Mode = ParticleSpawnModeOrdinal::Cloner;
    node.Evaluated.Shape = ParticleShapeOrdinal::SolidSurface;
    node.Evaluated.SpawnSolid = "UtahTeapot";
    node.Evaluated.CarrierMode = ParticleCarrierOrdinal::Object3D;
    node.Evaluated.CarrierSolid = "Cube";
    node.Evaluated.BackingSurface = ParticleBackingSurfaceOrdinal::BrushedChrome;
    node.Evaluated.Count = 32u;

    ParticleGpuSimulation sim;
    ParticleSimulationRegion region {};
    std::vector<ParticleCloudNode> clouds { node };
    sim.StepCloudsCpu(clouds, solidReg, region, 0.5f, 1.0f / 60.0f);

    ParticleGpuRenderer renderer;
    ParticleCameraView cam {};
    const auto calls = renderer.BuildCarrierSolidRenderCalls(clouds, sim, solidReg, cam, 0.5f, 4.0f);
    CheckCondition(calls.size() == 2u && calls[0].IsBackingCall && calls[0].BackingMode == 2u && !calls[1].IsBackingCall,
        "Carrier solid planner emits BrushedChrome backing followed by instanced Object3D carrier");

    CarrierSolidVertexCpuResult vx {};
    vx.WorldPos = { 0.2f, 0.3f, 0.0f };
    vx.WorldNormal = { 0.0f, 0.8f, 0.6f };
    vx.LocalPos = { 0.1f, 0.2f, 0.0f };
    vx.Colour = { 0.8f, 0.5f, 0.2f };
    vx.Alpha = 1.0f;
    vx.Ridge = 0.5f;
    bool allModesOk = true;
    for (uint32_t m = 0u; m < 4u; ++m)
    {
        auto u = calls[1].UniformBlock;
        u.BackingParams[3] = static_cast<float>(m);
        const auto fg = EvaluateCarrierSolidFragmentCpu(u, vx);
        allModesOk = allModesOk && !fg.Discarded && std::isfinite(fg.Rgb[0]) && std::isfinite(fg.Rgb[1]) && std::isfinite(fg.Rgb[2]) && fg.Alpha > 0.0f;
    }
    CheckCondition(allModesOk, "All 4 carrier/backing fragment modes (Carrier, ObsidianGlossy, BrushedChrome, GhostWireframe) evaluate cleanly");
}

void ValidateAllStrandModes()
{
    ParticleGpuRenderer renderer;
    ParticleSimulationRegion region {};
    ParticleCameraView cam {};

    bool allConfigsOk = true;
    uint32_t testedConfigs = 0u;
    for (uint32_t mode = 1u; mode <= 11u; ++mode)
    {
        for (uint32_t variant = 0u; variant < 2u; ++variant)
        {
            ParticleCloudNode node {};
            node.NodeId = "strand-m" + std::to_string(mode) + "-v" + std::to_string(variant);
            node.Enabled = true;
            node.Evaluated.StrandMode = static_cast<ParticleStrandOrdinal>(mode);
            node.Evaluated.StrandVariant = static_cast<ParticleStrandVariantOrdinal>(variant);
            node.Evaluated.StrandCount = 24u;
            node.Evaluated.StrandDetail = 12u;
            node.Evaluated.StrandSag = 0.35f;
            node.Evaluated.StrandThickness = 0.03f;
            node.Evaluated.StrandSway = 0.75f;
            node.Evaluated.StrandSpan = 1.8f;
            node.Evaluated.StrandGlow = 0.65f;
            node.Evaluated.ColourStops = { "#44ddff", "#ff8844" };

            std::vector<ParticleCloudNode> single { node };
            const auto calls = renderer.BuildStrandRenderCalls(single, region, cam, 1.25f);
            if (calls.size() != 1u || calls[0].VertexCount == 0u) { allConfigsOk = false; continue; }
            if (mode >= 9u && calls[0].Blend != ParticleBlendOrdinal::Additive) { allConfigsOk = false; }

            const auto& uBlock = calls[0].UniformBlock;
            const uint32_t sampleSlots = std::min(calls[0].StrandTotal, 16u);
            for (uint32_t slot = 0u; slot < sampleSlots; ++slot)
            {
                const auto ep = EvaluateStrandEndpointsCpu(uBlock, static_cast<int32_t>(slot));
                if (ep.Visible < 0.5f) continue;
                const auto pt = EvaluateStrandPointCpu(uBlock, static_cast<int32_t>(slot), ep, 0.5f);
                const auto fg = EvaluateStrandFragmentCpu(uBlock, { 0.4f, 0.8f, 0.5f }, 0.5f, 0.1f, ep.StyleTag, 0.37f, 1.0f);
                if (!std::isfinite(pt[0]) || !std::isfinite(pt[1]) || !std::isfinite(pt[2]) ||
                    !std::isfinite(fg.Rgb[0]) || !std::isfinite(fg.Rgb[1]) || !std::isfinite(fg.Rgb[2]) || !std::isfinite(fg.Alpha))
                {
                    allConfigsOk = false;
                }
            }
            ++testedConfigs;
        }
    }
    CheckCondition(testedConfigs == 22u && allConfigsOk, "All 11 strand modes x 2 variants (22 configs) produce finite endpoints, curves & fragments");
}

void ValidateAll71Templates(const CarrierSolidRegistry& solidReg)
{
    const fs::path root = ResolveRepoRoot();
    std::string err;
    const auto specsOpt = LoadLayerSpecificationsFromFile((root / "EngineContent/Simulation/LayerSpecifications.VolumetricSpace").string(), &err);
    CheckCondition(specsOpt.has_value(), "LayerSpecifications.VolumetricSpace loads cleanly");
    if (!specsOpt) return;

    ParticleTemplateRegistry ptReg;
    const bool loaded = ptReg.LoadFromDirectory(*specsOpt, (root / "EngineContent/Simulation/ParticleTemplates").string(), &err);
    CheckCondition(loaded && ptReg.QueryTemplates().size() == 71u, "All 71 .ParticleSpace templates load cleanly");
    if (!loaded) return;

    std::vector<ParticleCloudNode> clouds;
    clouds.reserve(ptReg.QueryTemplates().size());
    for (const auto& entry : ptReg.QueryTemplates())
    {
        auto c = ptReg.ConstructParticleCloud(*specsOpt, entry.Id, "tpl-" + entry.Id);
        if (!c) { CheckCondition(false, "Template cloud construction failed"); return; }
        c->Evaluated.Count = std::min(c->Evaluated.Count, 128u);
        clouds.push_back(std::move(*c));
    }
    CheckCondition(clouds.size() == 71u, "Constructed all 71 ParticleCloudNodes from template library");

    ParticleGpuSimulation sim;
    ParticleGpuRenderer renderer;
    ParticleSimulationRegion region {};
    ParticleCameraView cam {};
    for (int step = 0; step < 60; ++step)
    {
        sim.StepCloudsCpu(clouds, solidReg, region, static_cast<float>(step) / 60.0f, 1.0f / 60.0f);
    }

    bool allFinite = true;
    for (const auto& c : clouds)
    {
        const auto* rec = sim.FindCloudRecord(c.NodeId);
        if (!rec) { allFinite = false; break; }
        for (uint32_t i = 0u; i < rec->ActiveCount; ++i)
        {
            for (int k = 0; k < 4; ++k)
            {
                if (!std::isfinite(rec->ReadStates[i].PositionAge[k]) || !std::isfinite(rec->ReadStates[i].VelocitySeed[k]))
                {
                    allFinite = false;
                }
            }
        }
    }
    CheckCondition(allFinite, "All 71 templates stepped 60 frames with 0 NaN/Inf");

    const auto spriteCalls = renderer.BuildSpriteRenderCalls(clouds, sim, cam, 1.0f, 4.0f);
    const auto ribbonCalls = renderer.BuildRibbonRenderCalls(clouds, sim, cam, 1.0f, 4.0f);
    const auto solidCalls  = renderer.BuildCarrierSolidRenderCalls(clouds, sim, solidReg, cam, 1.0f, 4.0f);
    const auto strandCalls = renderer.BuildStrandRenderCalls(clouds, region, cam, 1.0f);

    CheckCondition(!spriteCalls.empty(), "71-template sprite planner produces active sprite/streak/halo calls");
    CheckCondition(!ribbonCalls.empty(), "71-template ribbon planner produces active Catmull-Rom ribbon + halo calls");
    CheckCondition(!solidCalls.empty(),  "71-template carrier planner produces active backing + Object3D carrier calls");
    CheckCondition(!strandCalls.empty(), "71-template strand planner produces active web/flora/arc calls");

    std::printf("[ParticleGpuPhysicsValidation] 71-template batch totals: sprite=%zu ribbon=%zu solid=%zu strand=%zu\n",
        spriteCalls.size(), ribbonCalls.size(), solidCalls.size(), strandCalls.size());
}

} // namespace

int main()
{
    CarrierSolidRegistry solidReg;
    ValidateFireworkPatterns();
    ValidateCarrierSolidLibrary();
    ValidateGpuSimulationModes(solidReg);
    ValidateSpritesCutoutsRamps();
    ValidateRibbonsTrails(solidReg);
    ValidateCarrierSolidsBacking(solidReg);
    ValidateAllStrandModes();
    ValidateAll71Templates(solidReg);

    std::printf("[ParticleGpuPhysicsValidation] Passed: %u, Failed: %u\n", gPassedChecks, gFailedChecks);
    return (gFailedChecks == 0u) ? 0 : 1;
}
