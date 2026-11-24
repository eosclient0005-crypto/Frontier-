/*============================================================================================================================================
                                                       PARTICLEGPURENDERER.CPP
============================================================================================================================================*/
// 🧩 ParticleSpace GPU sprite/streak uniform builder, draw-batch planner & bit-accurate CPU mirror of ParticleShadingCommon + 18 cutouts
// Reference: Experimental/FluidEditor/Engine/ParticleShaders.js L753-L966, L1088-L1340 & ParticleSimulation.js L340-L655

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/ParticleSpace/ParticleGpuRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace Frontier::ParticleSpace
{

namespace
{

using Vec3f = std::array<float, 3>;

constexpr float kPi    = 3.14159265f;
constexpr float kTwoPi = 6.2831853f;

[[nodiscard]] float ClampF(const float x, const float lo, const float hi) noexcept { return std::clamp(x, lo, hi); }
[[nodiscard]] float FractF(const float x) noexcept { return x - std::floor(x); }
[[nodiscard]] float LerpF(const float a, const float b, const float t) noexcept { return a + (b - a) * t; }
[[nodiscard]] float StepF(const float edge, const float x) noexcept { return (x < edge) ? 0.0f : 1.0f; }
[[nodiscard]] float SmoothStepF(const float edge0, const float edge1, const float x) noexcept
{
    const float denom = edge1 - edge0;
    const float t = (std::fabs(denom) > 1.0e-9f) ? ClampF((x - edge0) / denom, 0.0f, 1.0f) : 0.0f;
    return t * t * (3.0f - 2.0f * t);
}

[[nodiscard]] Vec3f Add3(const Vec3f& a, const Vec3f& b) noexcept { return { a[0] + b[0], a[1] + b[1], a[2] + b[2] }; }
[[nodiscard]] Vec3f Mul3(const Vec3f& a, const float s) noexcept { return { a[0] * s, a[1] * s, a[2] * s }; }
[[nodiscard]] Vec3f Lerp3(const Vec3f& a, const Vec3f& b, const float t) noexcept
{
    return { LerpF(a[0], b[0], t), LerpF(a[1], b[1], t), LerpF(a[2], b[2], t) };
}
[[nodiscard]] float Dot3(const Vec3f& a, const Vec3f& b) noexcept { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
[[nodiscard]] Vec3f Cross3(const Vec3f& a, const Vec3f& b) noexcept
{
    return { a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0] };
}
[[nodiscard]] float Dist3(const Vec3f& a, const Vec3f& b) noexcept
{
    const float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

[[nodiscard]] std::array<float, 4> ParseHexSwatch(const std::string_view hex) noexcept
{
    std::string_view digits = hex;
    if (!digits.empty() && digits.front() == '#') digits.remove_prefix(1u);
    if (digits.size() != 6u) return { 1.0f, 1.0f, 1.0f, 0.0f };
    char buf[8] {};
    for (size_t i = 0u; i < 6u; ++i) buf[i] = digits[i];
    const unsigned long val = std::strtoul(buf, nullptr, 16);
    return {
        static_cast<float>((val >> 16u) & 255ul) / 255.0f,
        static_cast<float>((val >> 8u) & 255ul) / 255.0f,
        static_cast<float>(val & 255ul) / 255.0f,
        0.0f
    };
}

[[nodiscard]] Vec3f HueToRgb(const float h) noexcept
{
    auto ch = [h](const float offset) noexcept -> float {
        const float raw = offset + h * 6.0f;
        const float k = raw - 6.0f * std::floor(raw / 6.0f);
        return 1.0f - ClampF(std::min(k, 4.0f - k), 0.0f, 1.0f) * 0.85f;
    };
    return { ch(5.0f), ch(3.0f), ch(1.0f) };
}

[[nodiscard]] Vec3f SampleSwatchGradient(const ParticleShadeUniformBlock& shade, const float t) noexcept
{
    const int32_t count = std::max(1, static_cast<int32_t>(shade.ModeRampSwatch[3] + 0.5f));
    if (count == 1) return { shade.SwatchStops[0][0], shade.SwatchStops[0][1], shade.SwatchStops[0][2] };
    const float x = ClampF(t, 0.0f, 1.0f) * static_cast<float>(count - 1);
    const int32_t i = std::min(static_cast<int32_t>(std::floor(x)), count - 2);
    const Vec3f a { shade.SwatchStops[i][0], shade.SwatchStops[i][1], shade.SwatchStops[i][2] };
    const Vec3f b { shade.SwatchStops[i + 1][0], shade.SwatchStops[i + 1][1], shade.SwatchStops[i + 1][2] };
    return Lerp3(a, b, x - static_cast<float>(i));
}

[[nodiscard]] Vec3f SampleSwatchEntry(const ParticleShadeUniformBlock& shade, const float seed) noexcept
{
    const int32_t count = std::max(1, static_cast<int32_t>(shade.ModeRampSwatch[3] + 0.5f));
    const int32_t i = std::clamp(static_cast<int32_t>(std::floor(seed * static_cast<float>(count))), 0, count - 1);
    return { shade.SwatchStops[i][0], shade.SwatchStops[i][1], shade.SwatchStops[i][2] };
}

[[nodiscard]] Vec3f SampleColourGradient(const ParticleShadeUniformBlock& shade, const float t) noexcept
{
    const int32_t rampSource = static_cast<int32_t>(shade.ModeRampSwatch[2] + 0.5f);
    const uint32_t rampMode  = static_cast<uint32_t>(std::max(0, static_cast<int32_t>(shade.ModeRampSwatch[1] + 0.5f)));
    return (rampSource == 1) ? EvaluateAnalyticRampCpu(rampMode, t) : SampleSwatchGradient(shade, t);
}

[[nodiscard]] Vec3f HueRotate(const Vec3f& colour, const float angle) noexcept
{
    constexpr Vec3f k { 0.57735f, 0.57735f, 0.57735f };
    const float c = std::cos(angle), s = std::sin(angle);
    return Add3(Add3(Mul3(colour, c), Mul3(Cross3(k, colour), s)), Mul3(k, Dot3(k, colour) * (1.0f - c)));
}

[[nodiscard]] Vec3f FetchAnchorNormal(std::span<const float> anchorRgba, const int32_t anchorCount, const int32_t index) noexcept
{
    if (anchorCount < 1 || anchorRgba.size() < 8u) return { 0.0f, 1.0f, 0.0f };
    const int32_t slot = std::clamp(index, 0, anchorCount - 1);
    const size_t base = static_cast<size_t>(slot) * 8u;
    if (base + 7u >= anchorRgba.size()) return { 0.0f, 1.0f, 0.0f };
    return { anchorRgba[base + 4u], anchorRgba[base + 5u], anchorRgba[base + 6u] };
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                               RAMP & UNIFORM BUILDERS
//------------------------------------------------------------------------------------------------------------------------

uint32_t ResolveAnalyticRampOrdinal(const std::string_view rampName) noexcept
{
    if (rampName == "Engineering") return 1u;
    if (rampName == "Plasma")      return 2u;
    if (rampName == "Diverging")   return 3u;
    return 0u; // Turbo
}

ParticleShadeUniformBlock BuildParticleShadeUniforms(
    const ParticleCloudParameters& params,
    const float simulationTimeSeconds,
    const float regionHeightMeters) noexcept
{
    ParticleShadeUniformBlock block {};
    const size_t stopCount = params.ColourStops.empty() ? 1u : std::min<size_t>(params.ColourStops.size(), 8u);
    if (params.ColourStops.empty())
    {
        block.SwatchStops[0] = { 1.0f, 1.0f, 1.0f, 0.0f };
    }
    else
    {
        for (size_t i = 0u; i < stopCount; ++i)
        {
            block.SwatchStops[i] = ParseHexSwatch(params.ColourStops[i]);
        }
    }

    block.ModeRampSwatch = {
        static_cast<float>(params.ColourMode),
        static_cast<float>(ResolveAnalyticRampOrdinal(params.RampName)),
        static_cast<float>(params.RampSource),
        static_cast<float>(stopCount)
    };
    block.ColourJitterCycle = {
        params.HueJitter,
        params.BrightnessJitter,
        params.CycleSpeed,
        params.Brightness
    };
    block.LifeRangeFlicker = {
        std::max(0.05f, params.Lifetime),
        params.Linger,
        params.MaxSpeed,
        params.DisplacementRange
    };
    block.TimingRegionHeight = {
        params.Flicker,
        simulationTimeSeconds,
        std::max(1.0e-3f, regionHeightMeters),
        0.0f
    };
    return block;
}

ParticleSpriteUniformBlock BuildParticleSpriteUniforms(
    const ParticleCloudParameters& params,
    const uint32_t textureSide,
    const uint32_t anchorWidth,
    const uint32_t anchorCount,
    const ParticleCameraView& camera,
    const float simulationTimeSeconds,
    const float regionHeightMeters,
    const float haloScale,
    const float haloAlpha,
    const int32_t spriteOverride) noexcept
{
    ParticleSpriteUniformBlock block {};
    block.Shape = BuildParticleShapeUniforms(params, textureSide, anchorWidth, anchorCount);
    block.Shade = BuildParticleShadeUniforms(params, simulationTimeSeconds, regionHeightMeters);
    block.ViewProjection = camera.ViewProjection;

    const float viewW = std::max(1.0f, camera.ViewportWidthPx * camera.PixelRatio);
    const float viewH = std::max(1.0f, camera.ViewportHeightPx * camera.PixelRatio);
    const float focalPixels = viewH / (2.0f * std::tan(std::max(0.05f, camera.VerticalFovRad) * 0.5f));

    block.CameraPositionFocal    = { camera.CameraPosition[0], camera.CameraPosition[1], camera.CameraPosition[2], focalPixels };
    block.CameraForwardViewportX = { camera.CameraForward[0], camera.CameraForward[1], camera.CameraForward[2], viewW };
    block.ViewportYSizeParams    = { viewH, params.SizeStart, params.SizeEnd, params.SizeRandom };
    block.PixelDofParams         = { camera.PixelRatio, static_cast<float>(params.SizeMode), params.FocusDistance, params.Aperture };

    const float spriteVal = (spriteOverride >= 0) ? static_cast<float>(spriteOverride) : static_cast<float>(params.Sprite);
    block.HaloSpriteStyleTrail   = { haloScale, haloAlpha, spriteVal, static_cast<float>(params.Style) };
    return block;
}

//------------------------------------------------------------------------------------------------------------------------
//                                              ANALYTIC COLOUR RAMPS (CPU)
//------------------------------------------------------------------------------------------------------------------------

std::array<float, 3> EvaluateAnalyticRampCpu(const uint32_t rampMode, float t) noexcept
{
    t = ClampF(t, 0.0f, 1.0f);
    if (rampMode == 1u)
    {
        static constexpr std::array<Vec3f, 7u> kStops {{
            { 0.043f, 0.122f, 0.227f }, { 0.071f, 0.318f, 0.541f }, { 0.102f, 0.627f, 0.784f },
            { 0.373f, 0.816f, 0.627f }, { 0.898f, 0.890f, 0.416f }, { 0.949f, 0.545f, 0.176f },
            { 0.851f, 0.157f, 0.110f }
        }};
        const float u = t * 6.0f;
        const int32_t i = std::min(static_cast<int32_t>(std::floor(u)), 5);
        return Lerp3(kStops[i], kStops[i + 1], FractF(u));
    }
    if (rampMode == 2u)
    {
        const Vec3f c0 { 0.0509f, 0.0298f, 0.5279f };
        const Vec3f c1 { 2.1763f, 0.2383f, 0.7539f };
        const Vec3f c2 { -2.6895f, -7.4559f, 3.1107f };
        const Vec3f c3 { 6.1303f, 42.3461f, -28.5188f };
        const Vec3f c4 { -11.1074f, -82.6663f, 60.1398f };
        const Vec3f c5 { 10.0231f, 71.4136f, -54.0721f };
        const Vec3f c6 { -3.6587f, -22.9315f, 18.1919f };
        Vec3f acc = c6;
        acc = Add3(c5, Mul3(acc, t));
        acc = Add3(c4, Mul3(acc, t));
        acc = Add3(c3, Mul3(acc, t));
        acc = Add3(c2, Mul3(acc, t));
        acc = Add3(c1, Mul3(acc, t));
        acc = Add3(c0, Mul3(acc, t));
        return acc;
    }
    if (rampMode == 3u)
    {
        static constexpr std::array<Vec3f, 7u> kStops {{
            { 0.129f, 0.400f, 0.675f }, { 0.404f, 0.663f, 0.812f }, { 0.820f, 0.898f, 0.941f },
            { 0.969f, 0.969f, 0.969f }, { 0.992f, 0.859f, 0.780f }, { 0.937f, 0.541f, 0.384f },
            { 0.698f, 0.094f, 0.169f }
        }};
        const float u = t * 6.0f;
        const int32_t i = std::min(static_cast<int32_t>(std::floor(u)), 5);
        return Lerp3(kStops[i], kStops[i + 1], FractF(u));
    }

    // Turbo (mode 0)
    const float t2 = t * t, t3 = t2 * t, t4 = t2 * t2, t5 = t3 * t2;
    const float r = 0.13572138f + 4.61539260f * t - 42.66032258f * t2 + 132.13108234f * t3 - 152.94239396f * t4 + 59.28637943f * t5;
    const float g = 0.09140261f + 2.19418839f * t +  4.84296658f * t2 -  14.18503333f * t3 +   4.27729857f * t4 +  2.82956604f * t5;
    const float b = 0.10667330f + 12.64194608f * t - 60.58204836f * t2 + 110.36276771f * t3 -  89.90310912f * t4 + 27.34824973f * t5;
    return { r, g, b };
}

//------------------------------------------------------------------------------------------------------------------------
//                                              PER-PARTICLE SHADING (CPU)
//------------------------------------------------------------------------------------------------------------------------

ParticleShadeCpuResult EvaluateParticleShadeCpu(
    const ParticleShapeUniformBlock& shape,
    const ParticleShadeUniformBlock& shade,
    std::span<const float> anchorTexelRgba,
    const int32_t particleIndex,
    const std::array<float, 4>& currentPositionAge,
    const std::array<float, 4>& velocitySeed) noexcept
{
    ParticleShadeCpuResult res {};
    const float simTime = shade.TimingRegionHeight[1];
    const int32_t mode = static_cast<int32_t>(shape.ShapeCounts[3] + 0.5f);
    const int32_t count = std::max(1, static_cast<int32_t>(shape.ShapeCounts[1] + 0.5f));
    const int32_t shapeCode = static_cast<int32_t>(shape.ShapeCounts[0] + 0.5f);
    const int32_t anchorCount = static_cast<int32_t>(shape.AnchorLayout[1] + 0.5f);

    const Vec3f restHome = EvaluateShapePointAtTime(shape, anchorTexelRgba, particleIndex, { 0.0f, 0.0f, 0.0f }, true, simTime);
    res.EffectorWeight = EvaluateEffectorWeight(shape, restHome, simTime);
    const DerezEvaluationReading derez = EvaluateDerezFront(shape, restHome, simTime);
    res.DerezReleased = derez.Released ? 1.0f : 0.0f;

    const float lifetime = shade.LifeRangeFlicker[0];
    const float linger   = shade.LifeRangeFlicker[1];
    const float alive = ((mode == 1 || currentPositionAge[3] < lifetime) && particleIndex < count) ? 1.0f : 0.0f;
    res.Life = (mode == 1) ? 0.5f : ClampF(currentPositionAge[3] / std::max(lifetime, 1.0e-3f), 0.0f, 1.0f);

    const int32_t colourMode = static_cast<int32_t>(shade.ModeRampSwatch[0] + 0.5f);
    const float speed = std::sqrt(
        velocitySeed[0] * velocitySeed[0] + velocitySeed[1] * velocitySeed[1] + velocitySeed[2] * velocitySeed[2]);
    const Vec3f currPos { currentPositionAge[0], currentPositionAge[1], currentPositionAge[2] };
    Vec3f colour {};

    if (colourMode == 1)
    {
        colour = SampleColourGradient(shade, ClampF(speed / std::max(shade.LifeRangeFlicker[2], 1.0e-3f), 0.0f, 1.0f));
    }
    else if (colourMode == 2)
    {
        colour = SampleColourGradient(shade, ClampF(Dist3(currPos, restHome) / std::max(shade.LifeRangeFlicker[3], 1.0e-3f), 0.0f, 1.0f));
    }
    else if (colourMode == 3)
    {
        colour = SampleColourGradient(shade, ClampF(currPos[1] / std::max(shade.TimingRegionHeight[2], 1.0e-3f) + 0.5f, 0.0f, 1.0f));
    }
    else if (colourMode == 4)
    {
        colour = HueToRgb(velocitySeed[3]);
    }
    else if (colourMode == 5)
    {
        colour = SampleSwatchEntry(shade, FractF(velocitySeed[3] * 7.31f + 0.137f));
    }
    else if (colourMode == 6)
    {
        const float t = FractF(simTime * shade.ColourJitterCycle[2] + velocitySeed[3] * 0.35f + static_cast<float>(particleIndex) / static_cast<float>(count) * 0.5f);
        colour = SampleSwatchGradient(shade, std::fabs(t * 2.0f - 1.0f));
    }
    else
    {
        colour = SampleSwatchGradient(shade, res.Life);
    }

    if (shapeCode == 15 && anchorCount > 0)
    {
        const Vec3f fwTint = FetchAnchorNormal(anchorTexelRgba, anchorCount, particleIndex);
        colour = Lerp3(colour, Mul3(fwTint, 1.15f - 0.35f * res.Life), 0.78f);
    }

    const float effColourShift = shape.EffectorMotion[3];
    if (effColourShift > 0.001f && res.EffectorWeight > 0.001f)
    {
        const float shifted = ClampF(res.EffectorWeight * effColourShift, 0.0f, 1.0f);
        colour = Lerp3(colour, SampleSwatchGradient(shade, shifted), shifted);
    }

    const float hueJitter = shade.ColourJitterCycle[0];
    const float brightJitter = shade.ColourJitterCycle[1];
    if (hueJitter > 0.0f)
    {
        colour = HueRotate(colour, (FractF(velocitySeed[3] * 13.7f) - 0.5f) * hueJitter * kTwoPi);
    }
    if (brightJitter > 0.0f)
    {
        colour = Mul3(colour, 1.0f + (FractF(velocitySeed[3] * 29.3f) - 0.5f) * 2.0f * brightJitter);
    }
    colour = {
        std::max(colour[0], 0.0f) * shade.ColourJitterCycle[3],
        std::max(colour[1], 0.0f) * shade.ColourJitterCycle[3],
        std::max(colour[2], 0.0f) * shade.ColourJitterCycle[3]
    };

    const int32_t derezMode = static_cast<int32_t>(shape.EffectorAxisDerez[1] + 0.5f);
    const float derezMix = shape.EffectorAxisDerez[2];
    float derezFade = 1.0f;
    if (mode == 1 && derezMode > 0 && derezMix > 0.001f)
    {
        const float edgeFactor = derez.EdgeBand * shape.DerezFrontParams[2] * derezMix;
        colour[0] += shape.DerezEdgeColourWing[0] * edgeFactor;
        colour[1] += shape.DerezEdgeColourWing[1] * edgeFactor;
        colour[2] += shape.DerezEdgeColourWing[2] * edgeFactor;
        if (derez.Released && derezMode != 2)
        {
            const float releaseLife = ClampF(currentPositionAge[3] / std::max(lifetime, 0.25f), 0.0f, 1.0f);
            if (derezMode == 3) derezFade = 1.0f - SmoothStepF(0.25f, 0.85f, releaseLife);
            else derezFade = 1.0f - SmoothStepF(0.55f, 1.0f, releaseLife);
        }
        else if (derez.Released && derezMode == 2)
        {
            derezFade = 0.55f + 0.45f * derez.EdgeBand;
        }
    }

    const float flickerAmt = shade.TimingRegionHeight[0];
    res.Flicker = (flickerAmt > 0.0f)
        ? LerpF(1.0f, 0.35f + 0.65f * (0.5f + 0.5f * std::sin(simTime * (3.0f + 5.0f * velocitySeed[3]) + velocitySeed[3] * 50.0f)), flickerAmt)
        : 1.0f;

    const float glitchAmt = shape.GlitchParams[0];
    const float glitchRate = shape.GlitchParams[1];
    const float glitchScan = shape.GlitchParams[3];
    if (glitchAmt > 0.001f && glitchScan > 0.001f)
    {
        const float gTick = std::floor(simTime * std::max(glitchRate, 1.0f));
        const float d = (static_cast<float>(particleIndex) * 0.17f) * 12.9898f + gTick * 78.233f;
        const float gHash = FractF(static_cast<float>(std::sin(static_cast<double>(d))) * 43758.5453f);
        res.Flicker *= LerpF(1.0f, StepF(0.18f, gHash) * (0.65f + 0.7f * gHash), ClampF(glitchAmt * glitchScan, 0.0f, 1.0f));
    }

    const float fade = (mode == 1) ? derezFade : (SmoothStepF(0.0f, 0.05f, res.Life) * (1.0f - SmoothStepF(0.75f, 1.0f, res.Life)));
    res.Alpha = alive * fade * res.Flicker;
    res.Linger = (mode == 1)
        ? derezFade
        : ((particleIndex < count) ? ClampF(1.0f - (currentPositionAge[3] - lifetime) / std::max(linger, 1.0e-3f), 0.0f, 1.0f) : 0.0f);

    if (colourMode != 4) colour = Mul3(colour, 0.85f + 0.3f * res.Flicker);
    res.Colour = colour;
    return res;
}

//------------------------------------------------------------------------------------------------------------------------
//                                       18 ANALYTICAL SPRITE CUTOUTS (CPU)
//------------------------------------------------------------------------------------------------------------------------

SpriteCutoutCpuResult EvaluateSpriteCutoutCpu(
    const uint32_t spriteOrdinal,
    const uint32_t styleOrdinal,
    const std::array<float, 4>& inputColour,
    const float soft,
    const float across,
    const std::array<float, 4>& extra,
    const std::array<float, 2>& pointCoord) noexcept
{
    SpriteCutoutCpuResult out {};
    if (inputColour[3] <= 0.0f) { out.Discarded = true; return out; }
    Vec3f rgb { inputColour[0], inputColour[1], inputColour[2] };
    float alpha = inputColour[3];
    const bool butterflyMorph = (static_cast<int32_t>(std::floor(extra[0] + 0.5f)) == 6) && (spriteOrdinal == 5u);

    if (soft > 0.5f)
    {
        const float qx = pointCoord[0] - 0.5f;
        const float qy = pointCoord[1] - 0.5f;
        const float d = std::sqrt(qx * qx + qy * qy) * 2.0f;

        if (spriteOrdinal == 0u)
        {
            if (d > 1.0f) { out.Discarded = true; return out; }
            alpha *= std::exp(-d * d * 4.5f) * (1.0f - SmoothStepF(0.8f, 1.0f, d));
        }
        else if (spriteOrdinal == 1u)
        {
            if (d > 1.0f) { out.Discarded = true; return out; }
            alpha *= 1.0f - SmoothStepF(0.6f, 1.0f, d);
        }
        else if (spriteOrdinal == 2u)
        {
            if (d > 1.0f) { out.Discarded = true; return out; }
            alpha *= 0.22f + 0.78f * SmoothStepF(0.5f, 0.78f, d) * (1.0f - SmoothStepF(0.86f, 1.0f, d));
        }
        else if (spriteOrdinal == 3u)
        {
            const float arms = std::max(std::exp(-std::fabs(qx) * 14.0f), std::exp(-std::fabs(qy) * 14.0f));
            alpha *= std::max(arms * (1.0f - d * 0.9f), std::exp(-d * d * 10.0f));
            if (alpha < 0.01f) { out.Discarded = true; return out; }
        }
        else if (spriteOrdinal == 6u || butterflyMorph)
        {
            const float uvx = qx * 2.0f, uvy = -qy * 2.0f;
            const float wingW = std::max(extra[1], 0.16f);
            const float x = std::fabs(uvx);
            const float xs = x / wingW;
            const float dF = std::hypot((xs - 0.50f) / 0.50f, (uvy - 0.28f) / 0.36f);
            const float dH = std::hypot((xs - 0.40f) / 0.36f, (uvy + 0.28f) / 0.26f);
            const float dMin = std::min(dF, dH);
            const float wing = 1.0f - SmoothStepF(0.84f, 1.0f, dMin);
            const float body = (1.0f - SmoothStepF(0.035f, 0.065f, x)) * StepF(std::fabs(uvy), 0.62f);
            if (wing < 0.01f && body < 0.01f) { out.Discarded = true; return out; }
            const float edge = 0.76f + 0.48f * (1.0f - std::min(dMin, 1.0f));
            rgb = Lerp3(Mul3(rgb, edge), { 0.09f, 0.09f, 0.09f }, body * 0.85f);
            alpha *= std::max(wing, body);
        }
        else if (spriteOrdinal == 5u)
        {
            const float px = qx * 2.15f, py = -qy * 2.15f;
            const float hex = std::max(std::fabs(px) * 0.8660254f + std::fabs(py) * 0.5f, std::fabs(py));
            if (hex > 0.96f) { out.Discarded = true; return out; }
            const float mask = 1.0f - SmoothStepF(0.88f, 0.96f, hex);
            const bool topFace = py > std::fabs(px) * 0.57735f;
            const float shade = topFace ? 1.14f : (px < 0.0f ? 0.80f : 0.54f);
            const float seam = std::min(std::fabs(py - std::fabs(px) * 0.57735f), py < std::fabs(px) * 0.57735f ? std::fabs(px) : 1.0f);
            const float rim = SmoothStepF(0.80f, 0.90f, hex) * 0.28f + (1.0f - SmoothStepF(0.0f, 0.06f, seam)) * 0.22f;
            rgb = Mul3(rgb, shade + rim);
            alpha *= mask;
        }
        else if (spriteOrdinal == 7u)
        {
            const float px = std::fabs(qx * 2.1f), py = std::fabs(qy * 2.1f);
            const float hex = std::max(px * 0.8660254f + py * 0.5f, py);
            if (hex > 0.98f) { out.Discarded = true; return out; }
            const float rim = SmoothStepF(0.70f, 0.88f, hex) * (1.0f - SmoothStepF(0.90f, 0.98f, hex));
            const float core = (1.0f - SmoothStepF(0.0f, 0.88f, hex)) * 0.34f;
            rgb = Mul3(rgb, 0.65f + 1.15f * rim);
            alpha *= ClampF(core + rim * 1.15f, 0.0f, 1.0f);
        }
        else if (spriteOrdinal == 8u)
        {
            const float uvx = qx * 2.0f, uvy = qy * 2.0f;
            if (extra[3] > 0.5f)
            {
                if (std::fabs(uvy) > 0.24f || std::fabs(uvx) > 0.96f) { out.Discarded = true; return out; }
                const float scan = 0.7f + 0.3f * std::sin(uvx * 18.0f + extra[2] * 40.0f);
                alpha *= (1.0f - SmoothStepF(0.14f, 0.24f, std::fabs(uvy))) * scan;
            }
            else
            {
                const float gx = std::floor((qx + 0.5f) * 5.0f);
                const float gy = std::floor((qy + 0.5f) * 5.0f);
                const float symX = std::min(gx, 4.0f - gx);
                const float seedOff = std::floor(extra[2] * 31.0f);
                const float dv = (symX + seedOff) * 12.9898f + (gy + seedOff) * 78.233f;
                const float cellHash = FractF(static_cast<float>(std::sin(static_cast<double>(dv))) * 43758.5453f);
                const float lx = FractF((qx + 0.5f) * 5.0f) - 0.5f;
                const float ly = FractF((qy + 0.5f) * 5.0f) - 0.5f;
                const float box = StepF(std::max(std::fabs(lx), std::fabs(ly)), 0.38f);
                const float on = StepF(0.36f, cellHash) * box;
                if (on < 0.01f) { out.Discarded = true; return out; }
                alpha *= on * (0.82f + 0.18f * std::sin(qy * 36.0f));
            }
        }
        else if (spriteOrdinal == 9u)
        {
            const float uvx = qx * 2.0f, uvy = qy * 2.0f;
            const float r = std::hypot(uvx, uvy);
            if (r > 0.98f) { out.Discarded = true; return out; }
            const float a = std::atan2(uvy, uvx) + kPi;
            const float arm = std::pow(std::fabs(std::cos(a * 3.0f)), r > 0.16f ? 22.0f : 2.0f);
            const float branch = std::pow(std::fabs(std::cos(a * 3.0f + 1.05f)), 26.0f) * SmoothStepF(0.26f, 0.52f, r) * (1.0f - SmoothStepF(0.62f, 0.94f, r));
            const float hub = 1.0f - SmoothStepF(0.06f, 0.20f, r);
            const float arms = std::max(arm * (1.0f - SmoothStepF(0.72f, 0.98f, r)), branch * 0.82f);
            const float mask = std::max(std::max(arms, hub), 0.0f);
            if (mask < 0.012f) { out.Discarded = true; return out; }
            rgb = Lerp3(rgb, Lerp3(rgb, { 1.0f, 1.0f, 1.0f }, 0.35f), hub);
            alpha *= ClampF(mask, 0.0f, 1.0f) * (0.72f + 0.55f * mask);
        }
        else if (spriteOrdinal == 10u)
        {
            const float uvx = qx * 2.0f, uvy = -qy * 2.0f;
            const float t = ClampF((uvy + 0.85f) / 1.75f, 0.0f, 1.0f);
            const float halfW = 1.43f * std::sqrt(t) * std::pow(std::max(0.0f, 1.0f - t), 1.35f);
            const float acrossVal = std::fabs(uvx) - halfW;
            const float inY = SmoothStepF(-0.90f, -0.82f, uvy) * (1.0f - SmoothStepF(0.86f, 0.94f, uvy));
            const float mask = (1.0f - SmoothStepF(-0.04f, 0.04f, acrossVal)) * inY;
            if (mask < 0.01f || halfW < 0.004f) { out.Discarded = true; return out; }
            const float rim = SmoothStepF(0.45f, 0.96f, std::fabs(uvx) / std::max(halfW, 0.01f));
            const float bead = 1.0f - SmoothStepF(0.0f, 0.28f, std::hypot(uvx + 0.14f, uvy + 0.32f));
            const float coreShade = 0.76f + 0.26f * (1.0f - t) + 0.28f * rim;
            rgb = Lerp3(Mul3(rgb, coreShade), { 1.0f, 1.0f, 1.0f }, bead * 0.65f);
            alpha *= mask;
        }
        else if (spriteOrdinal == 11u)
        {
            const float uvx = qx * 2.0f, uvy = -qy * 2.0f;
            const float width = (1.0f - uvy * uvy) * 0.62f;
            const float serration = 0.055f * std::sin(uvy * 26.0f);
            const float acrossVal = std::fabs(uvx) - width - serration * StepF(0.0f, width);
            const float mask = (1.0f - SmoothStepF(-0.03f, 0.05f, acrossVal)) * StepF(std::fabs(uvy), 1.0f);
            if (mask < 0.01f) { out.Discarded = true; return out; }
            const float rib = 1.0f - SmoothStepF(0.0f, 0.05f + std::fabs(uvy) * 0.03f, std::fabs(uvx));
            const Vec3f bladeCol = Mul3(rgb, 0.72f + 0.42f * (1.0f - std::fabs(uvx) / std::max(width, 1.0e-3f)));
            const Vec3f ribCol   = { rgb[0] * 1.5f + 0.10f, rgb[1] * 1.5f + 0.10f, rgb[2] * 1.5f + 0.10f };
            rgb = Lerp3(bladeCol, ribCol, rib * 0.7f);
            alpha *= mask;
        }
        else if (spriteOrdinal == 12u)
        {
            const float uvx = qx * 2.0f, uvy = -qy * 2.0f;
            const float lobe = std::hypot(uvx / 0.66f, (uvy - 0.02f) / 0.90f);
            const float notch = std::hypot(uvx / 0.16f, (uvy - 0.96f) / 0.22f);
            const float mask = (1.0f - SmoothStepF(0.86f, 1.0f, lobe)) * SmoothStepF(0.30f, 0.62f, notch + 0.34f);
            if (mask < 0.01f) { out.Discarded = true; return out; }
            const float vein = 1.0f - SmoothStepF(0.0f, 0.09f, std::fabs(uvx + std::sin(uvy * 3.0f) * 0.05f));
            const Vec3f veinCol { rgb[0] * 1.35f + 0.06f, rgb[1] * 1.35f + 0.06f, rgb[2] * 1.35f + 0.06f };
            rgb = Lerp3(Mul3(rgb, 0.90f), veinCol, vein * 0.6f);
            alpha *= mask;
        }
        else if (spriteOrdinal == 13u)
        {
            const float px = qx * 2.0f, py = -qy * 2.0f;
            const float hex = std::max(std::fabs(px) * 0.8660254f + std::fabs(py) * 0.5f, std::fabs(py));
            const float taper = 1.0f - SmoothStepF(0.55f, 1.0f, std::fabs(py)) * 0.22f;
            if (hex > 0.96f * taper) { out.Discarded = true; return out; }
            const float mask = 1.0f - SmoothStepF(0.88f, 0.96f * taper, hex);
            const float faceA = SmoothStepF(0.0f, 0.55f, px + 0.30f);
            const float faceB = SmoothStepF(0.0f, 0.55f, -px + 0.30f);
            const float faceC = SmoothStepF(0.10f, 0.35f, py);
            const float shade = 0.62f + 0.62f * faceA + 0.28f * faceB + 0.34f * faceC;
            const float edge = SmoothStepF(0.78f, 0.92f, hex) * 0.55f;
            rgb = Mul3(rgb, shade + edge);
            alpha *= mask;
        }
        else if (spriteOrdinal == 14u)
        {
            const float uvx = qx * 2.0f, uvy = qy * 2.0f;
            const float r = std::hypot(uvx, uvy);
            if (r > 1.0f) { out.Discarded = true; return out; }
            const float tR = (r - 0.78f) / 0.10f;
            const float tI = (r - 0.46f) / 0.20f;
            const float ring = std::exp(-(tR * tR));
            const float inner = std::exp(-(tI * tI)) * 0.28f;
            const float mask = ring + inner;
            if (mask < 0.012f) { out.Discarded = true; return out; }
            rgb = Lerp3(Mul3(rgb, 0.65f), { 1.0f, 1.0f, 1.0f }, ring * 0.55f);
            alpha *= ClampF(mask, 0.0f, 1.0f);
        }
        else if (spriteOrdinal == 15u)
        {
            const float uvx = qx * 2.0f, uvy = qy * 2.0f;
            const float r = std::hypot(uvx, uvy);
            if (r > 1.0f) { out.Discarded = true; return out; }
            const float a = std::atan2(uvy, uvx);
            if (styleOrdinal == 1u)
            {
                const float pulse = FractF(r * 3.0f - extra[2] * 2.4f);
                const float tw = (pulse - 0.14f) / 0.10f;
                const float wave = std::exp(-(tw * tw));
                const float crossLines = (1.0f - SmoothStepF(0.015f, 0.04f, std::fabs(uvx))) * 0.25f
                                       + (1.0f - SmoothStepF(0.015f, 0.04f, std::fabs(uvy))) * 0.25f;
                const float rimNotch = StepF(0.84f, r) * StepF(0.86f, std::fabs(std::cos(a * 12.0f))) * 0.55f;
                const float mask = std::max(wave * (1.0f - r * 0.35f), std::max(crossLines * StepF(0.12f, r), rimNotch));
                if (mask < 0.012f) { out.Discarded = true; return out; }
                rgb = Lerp3(rgb, { 0.45f, 1.0f, 0.90f }, wave * 0.65f);
                alpha *= ClampF(mask, 0.0f, 1.0f) * (1.0f - SmoothStepF(0.92f, 1.0f, r));
            }
            else
            {
                const float sweep = extra[2] * kTwoPi;
                const float rawWrap = a - sweep + 12.5663706f;
                const float wrapped = rawWrap - kTwoPi * std::floor(rawWrap / kTwoPi);
                const float wake = SmoothStepF(0.0f, 1.9f, wrapped) * StepF(wrapped, 2.6f);
                const float tl = wrapped / 0.14f;
                const float leading = std::exp(-(tl * tl));
                const float rings = 0.22f * SmoothStepF(0.38f, 0.5f, std::fabs(FractF(r * 3.0f) - 0.5f));
                const float dAx = uvx - 0.42f, dAy = uvy - 0.36f;
                const float dBx = uvx + 0.52f, dBy = uvy + 0.22f;
                const float blipA = std::exp(-(dAx * dAx + dAy * dAy) * 180.0f);
                const float blipB = std::exp(-(dBx * dBx + dBy * dBy) * 180.0f);
                const float blips = (blipA + blipB) * (0.45f + 0.55f * wake);
                const float mask = std::max(std::max(leading, wake * 0.42f), blips) + rings * StepF(0.18f, r);
                if (mask < 0.01f) { out.Discarded = true; return out; }
                rgb = Lerp3(rgb, { 0.65f, 1.0f, 0.85f }, std::max(leading * 0.55f, blips * 0.85f));
                alpha *= ClampF(mask, 0.0f, 1.0f) * (1.0f - SmoothStepF(0.92f, 1.0f, r));
            }
        }
        else if (spriteOrdinal == 16u)
        {
            const float uvx = qx * 2.0f, uvy = -qy * 2.0f;
            const float ribbonA = std::sin(uvx * 5.0f + extra[2] * 4.0f) * 0.30f;
            const float ribbonB = std::sin(uvx * 9.0f - extra[2] * 6.5f) * 0.16f;
            const float band = 1.0f - SmoothStepF(0.10f, 0.62f, std::fabs(uvy - ribbonA - ribbonB));
            const float verticalFade = SmoothStepF(-1.0f, 0.55f, uvy) * (1.0f - SmoothStepF(0.72f, 1.0f, uvy));
            const float mask = band * verticalFade;
            if (mask < 0.012f) { out.Discarded = true; return out; }
            const Vec3f aurora = Lerp3({ 0.22f, 0.95f, 0.72f }, { 0.55f, 0.42f, 1.0f }, SmoothStepF(-0.4f, 0.8f, uvy));
            rgb = Lerp3(rgb, aurora, 0.45f);
            alpha *= mask * 0.92f;
        }
        else if (spriteOrdinal == 17u)
        {
            const float uvx = qx * 2.0f, uvy = qy * 2.0f;
            const float r = std::hypot(uvx, uvy);
            if (r > 1.0f) { out.Discarded = true; return out; }
            const float rim = SmoothStepF(0.72f, 0.82f, r) * (1.0f - SmoothStepF(0.88f, 0.96f, r));
            const float milled = 0.5f + 0.5f * std::sin(std::atan2(uvy, uvx) * 44.0f);
            const float boss = 1.0f - SmoothStepF(0.16f, 0.34f, r);
            const float face = 1.0f - SmoothStepF(0.94f, 1.0f, r);
            const float relief = 0.86f + 0.24f * SmoothStepF(0.18f, 0.66f, r) + 0.26f * boss;
            const float hi = rim * milled * 0.42f + boss * 0.22f;
            rgb = { rgb[0] * relief + hi, rgb[1] * relief + hi, rgb[2] * relief + hi };
            alpha *= face;
        }
    }
    else if (soft < -0.5f)
    {
        alpha *= 1.0f - across * across;
    }

    out.Rgb = rgb;
    out.Alpha = alpha;
    out.Discarded = false;
    return out;
}

void ComputeSpriteDofMetricsCpu(
    const ParticleSpriteUniformBlock& uniforms,
    const float baseSize,
    const float depthMeters,
    float& outPixelDiameter,
    float& outAlphaMultiplier) noexcept
{
    const int32_t sizeMode = static_cast<int32_t>(uniforms.PixelDofParams[1] + 0.5f);
    float pixels = (sizeMode == 0)
        ? (baseSize * uniforms.CameraPositionFocal[3] / std::max(depthMeters, 0.05f))
        : (baseSize * uniforms.PixelDofParams[0]);

    const float focusDist = uniforms.PixelDofParams[2];
    const float aperture  = uniforms.PixelDofParams[3];
    const float coc = (focusDist > 0.0f)
        ? ClampF(std::fabs(depthMeters - focusDist) / std::max(focusDist, 0.1f) * aperture * 3.0f, 0.0f, 1.0f)
        : 0.0f;
    const float blurGrow = 1.0f + coc * 6.0f;
    const float haloScale = uniforms.HaloSpriteStyleTrail[0];
    const float haloAlpha = uniforms.HaloSpriteStyleTrail[1];
    pixels *= blurGrow * haloScale;
    outPixelDiameter = ClampF(pixels, 1.0f, 256.0f);
    outAlphaMultiplier = (depthMeters <= 0.05f) ? 0.0f : (haloAlpha / (blurGrow * blurGrow * 0.6f + 0.4f));
}

//------------------------------------------------------------------------------------------------------------------------
//                                             SPRITE DRAW-BATCH PLANNER
//------------------------------------------------------------------------------------------------------------------------

std::vector<ParticleSpriteDrawCall> ParticleGpuRenderer::BuildSpriteRenderCalls(
    std::span<const ParticleCloudNode> clouds,
    const ParticleGpuSimulation& simulation,
    const ParticleCameraView& camera,
    const float simulationTimeSeconds,
    const float regionHeightMeters) const
{
    // Reference: ParticleSimulation.js L557-L643
    std::vector<ParticleSpriteDrawCall> calls;
    for (const ParticleCloudNode& node : clouds)
    {
        if (!node.Visible || !node.Enabled || node.Halted) continue;
        const ParticleCloudStateRecord* rec = simulation.FindCloudRecord(node.NodeId);
        if (!rec || rec->ActiveCount == 0u) continue;

        const ParticleCloudParameters& p = node.Evaluated;
        const uint32_t strandMode = static_cast<uint32_t>(p.StrandMode);
        if (strandMode >= 1u && strandMode <= 8u && p.CarrierMode != ParticleCarrierOrdinal::Object3D) continue;
        if (p.CarrierMode == ParticleCarrierOrdinal::Object3D) continue;
        if (p.Trail == ParticleTrailModeOrdinal::Only) continue;

        const uint32_t count = std::min(p.Count, rec->TextureSide * rec->TextureSide);
        const ParticleSpriteUniformBlock mainBlock = BuildParticleSpriteUniforms(
            p, rec->TextureSide, rec->AnchorTexels.Width, rec->AnchorTexels.Count,
            camera, simulationTimeSeconds, regionHeightMeters, 1.0f, 1.0f, -1);

        if (p.Style == ParticleRenderStyleOrdinal::Streak)
        {
            calls.push_back(ParticleSpriteDrawCall {
                node.NodeId,
                ParticleDrawTopologyOrdinal::StreakLines,
                p.Blend,
                count * 2u,
                false,
                mainBlock
            });
        }

        calls.push_back(ParticleSpriteDrawCall {
            node.NodeId,
            ParticleDrawTopologyOrdinal::SpriteQuads,
            p.Blend,
            count * 6u,
            false,
            mainBlock
        });

        if (p.Glow > 0.0f)
        {
            const ParticleSpriteUniformBlock haloBlock = BuildParticleSpriteUniforms(
                p, rec->TextureSide, rec->AnchorTexels.Width, rec->AnchorTexels.Count,
                camera, simulationTimeSeconds, regionHeightMeters, 3.2f, 0.16f * p.Glow, 0);
            calls.push_back(ParticleSpriteDrawCall {
                node.NodeId,
                ParticleDrawTopologyOrdinal::SpriteQuads,
                ParticleBlendOrdinal::Additive,
                count * 6u,
                true,
                haloBlock
            });
        }
    }
    return calls;
}

} // namespace Frontier::ParticleSpace
