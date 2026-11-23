/*============================================================================================================================================
                                                       FORCEFIELDPHYSICSCOUPLING.H
============================================================================================================================================*/
// 🧩 Wait-free ForceFieldRegister -> JoltPhysics rigid body & vehicle aerodynamic coupling + Stage F GPU impulse readback contract
// Reference: UnifiedFluidParticleForceFieldPortPlan.md §1.4 & §2.2A (Slice S06)

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/Mathematics/AxisCoordinates.h"
#include "Engine/SimulationSpace/ForceFields/ForceFieldRegister.h"
#include "Engine/SimulationSpace/ForceFields/ForceFieldSampler.h"
#include "Engine/SimulationSpace/PhysicsSpace/RigidBodyDynamics.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace Frontier::SimulationSpace::PhysicsSpace
{

class PhysicsExtension;

//------------------------------------------------------------------------------------------------------------------------
//                                                     DATA STRUCTURES
//------------------------------------------------------------------------------------------------------------------------

enum class ForceCouplingMode : uint8_t
{
    MassProportionalAcceleration = 0u, // [N] - F = m * a_field * CouplingScale (gravity / wind / tractor beam parity)
    AerodynamicQuadraticDrag     = 1u, // [N] - F = 0.5 * rho * Cd * A * |v_rel| * v_rel using field vector as air velocity
};

struct CoupledRigidBodyEntry
{
    RigidBodyToken    BodyToken             {};                                               // [id]     - Live Jolt rigid body token
    ForceCouplingMode CouplingMode          = ForceCouplingMode::MassProportionalAcceleration; // [-]     - Force integration mode
    float             CouplingScale         = 1.0f;                                           // [-]      - Scalar strength multiplier
    float             DragAreaM2            = 1.0f;                                           // [m^2]    - Reference frontal/lateral area for Aero mode
    float             DragCoefficient       = 0.8f;                                           // [-]      - Dimensionless Cd for Aero mode
    float             AirDensityKgPerM3     = 1.225f;                                         // [kg/m^3] - Ambient fluid density (ISA sea level)
    float             MaximumForceNewtons   = 250000.0f;                                      // [N]      - Safety clamp against extreme singularities
    uint32_t          ChannelMaskBits       = static_cast<uint32_t>(ForceFields::ForceTargetChannelBits::Rigid);
};

struct ForceCouplingStepSummary
{
    uint32_t CoupledBodyCount    = 0u;   // [count] - Live dynamic bodies actuated during this tick
    float    PeakForceNewtons    = 0.0f; // [N]     - Largest force magnitude applied this tick
    float    TotalForceNewtons   = 0.0f; // [N]     - Sum of force magnitudes applied this tick
};

/// 64-byte GPU->CPU atomic impulse readback entry (shared contract for Stage F Slice S24 two-way particle-rigid coupling).
struct alignas(16) RigidImpulseAccumulatorEntry
{
    RigidBodyToken BodyToken       {};                   // [id]    - Target dynamic Jolt body
    uint32_t       ContactCount    = 0u;                 // [count] - Particle collisions accumulated this step
    uint32_t       ReservedPad     = 0u;                 // [-]     - 16-byte alignment padding
    float          LinearImpulse[4]= { 0.0f, 0.0f, 0.0f, 0.0f };  // [N*s]   - xyz = net linear impulse on rigid body
    float          AngularImpulse[4]= { 0.0f, 0.0f, 0.0f, 0.0f }; // [N*m*s] - xyz = net angular impulse about COM
    float          ContactPoint[4] = { 0.0f, 0.0f, 0.0f, 0.0f };  // [m]     - xyz = weighted average contact point
};

static_assert(sizeof(RigidImpulseAccumulatorEntry) == 64u, "RigidImpulseAccumulatorEntry must remain 64 bytes.");

//------------------------------------------------------------------------------------------------------------------------
//                                           PURE FORCE COUPLING COMPUTATION
//------------------------------------------------------------------------------------------------------------------------

/// Evaluates the net ForceField force vector [N] acting on a single RigidBodyReading from an immutable PublishedForceFieldReading.
[[nodiscard]] Mathematics::Axis3D ComputeCoupledRigidForceNewtons(
    const CoupledRigidBodyEntry& entry,
    const ForceFields::PublishedForceFieldReading& reading,
    const RigidBodyReading& bodyReading,
    double simulationSeconds) noexcept;

/// Evaluates the local aerodynamic crosswind force [N] on a vehicle chassis from ForceFields (using lateral/frontal drag areas).
[[nodiscard]] Mathematics::Axis3D ComputeVehicleWindForceNewtons(
    const ForceFields::PublishedForceFieldReading& reading,
    const RigidBodyReading& chassisReading,
    float frontalAreaM2,
    float sideAreaM2,
    float dragCoefficient,
    float airDensityKgPerM3,
    double simulationSeconds) noexcept;

//------------------------------------------------------------------------------------------------------------------------
//                                               FORCEFIELDPHYSICSCOUPLING
//------------------------------------------------------------------------------------------------------------------------

class ForceFieldPhysicsCoupling
{
public:
    ForceFieldPhysicsCoupling() noexcept = default;
    ~ForceFieldPhysicsCoupling() noexcept = default;

    void Clear() noexcept { CoupledBodies.clear(); }
    void RegisterRigidBody(const CoupledRigidBodyEntry& entry);
    bool UnregisterRigidBody(RigidBodyToken bodyToken) noexcept;
    [[nodiscard]] uint32_t QueryCoupledBodyCount() const noexcept { return static_cast<uint32_t>(CoupledBodies.size()); }

    /// Wait-free reads the published ForceFieldRegister and applies resulting forces to all registered dynamic bodies in PhysicsExtension.
    [[nodiscard]] ForceCouplingStepSummary ApplyPublishedForceFields(
        PhysicsExtension& physics,
        const ForceFields::ForceFieldRegister& forceRegister,
        double simulationSeconds) const noexcept;

    /// Wait-free reads the published ForceFieldRegister and applies aerodynamic wind/gust forces to a vehicle chassis body.
    [[nodiscard]] Mathematics::Axis3D ApplyPublishedWindToVehicleChassis(
        PhysicsExtension& physics,
        RigidBodyToken chassisToken,
        const ForceFields::ForceFieldRegister& forceRegister,
        float frontalAreaM2,
        float sideAreaM2,
        float dragCoefficient,
        double simulationSeconds,
        float airDensityKgPerM3 = 1.225f) const noexcept;

    /// Applies GPU-readback particle collision impulses (Stage F Slice S24) onto live dynamic Jolt rigid bodies.
    [[nodiscard]] uint32_t ApplyGpuImpulseReadback(
        PhysicsExtension& physics,
        std::span<const RigidImpulseAccumulatorEntry> readbackEntries) const noexcept;

private:
    std::vector<CoupledRigidBodyEntry> CoupledBodies {}; // [-] - Registered dynamic rigid bodies receiving force fields
};

} // namespace Frontier::SimulationSpace::PhysicsSpace
