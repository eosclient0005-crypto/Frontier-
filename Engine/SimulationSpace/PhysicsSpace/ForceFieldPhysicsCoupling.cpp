/*============================================================================================================================================
                                                      FORCEFIELDPHYSICSCOUPLING.CPP
============================================================================================================================================*/
// 🧩 Wait-free ForceFieldRegister -> JoltPhysics rigid body & vehicle aerodynamic coupling + Stage F GPU impulse readback contract
// Reference: UnifiedFluidParticleForceFieldPortPlan.md §1.4 & §2.2A (Slice S06)

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/SimulationSpace/PhysicsSpace/ForceFieldPhysicsCoupling.h"
#include "Engine/SimulationSpace/PhysicsSpace/PhysicsExtension.h"

#include <algorithm>
#include <cmath>

namespace Frontier::SimulationSpace::PhysicsSpace
{

namespace
{

[[nodiscard]] Mathematics::Axis3D ClampForceMagnitude(const Mathematics::Axis3D& f, const float maxNewtons) noexcept
{
    if (!std::isfinite(f.X) || !std::isfinite(f.Y) || !std::isfinite(f.Z))
    {
        return { 0.0f, 0.0f, 0.0f };
    }
    const float magSq = f.X * f.X + f.Y * f.Y + f.Z * f.Z;
    if (magSq <= maxNewtons * maxNewtons || maxNewtons <= 0.0f)
    {
        return f;
    }
    const float scale = maxNewtons / std::sqrt(magSq);
    return { f.X * scale, f.Y * scale, f.Z * scale };
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                           PURE FORCE COUPLING COMPUTATION
//------------------------------------------------------------------------------------------------------------------------

Mathematics::Axis3D ComputeCoupledRigidForceNewtons(
    const CoupledRigidBodyEntry& entry,
    const ForceFields::PublishedForceFieldReading& reading,
    const RigidBodyReading& bodyReading,
    const double simulationSeconds) noexcept
{
    if (!bodyReading.Resident || bodyReading.MassKg <= 0.0f || reading.ActiveCount == 0u)
    {
        return { 0.0f, 0.0f, 0.0f };
    }

    const std::array<float, 3> worldPos { bodyReading.PositionMeters.X, bodyReading.PositionMeters.Y, bodyReading.PositionMeters.Z };
    const std::array<float, 3> worldVel { bodyReading.LinearVelocity.X, bodyReading.LinearVelocity.Y, bodyReading.LinearVelocity.Z };
    const std::array<float, 3> fieldVec = ForceFields::EvaluateTotalForceField(
        reading.ActiveSpan(),
        worldPos,
        worldVel,
        static_cast<float>(simulationSeconds),
        entry.ChannelMaskBits);

    Mathematics::Axis3D forceN { 0.0f, 0.0f, 0.0f };
    if (entry.CouplingMode == ForceCouplingMode::MassProportionalAcceleration)
    {
        const float factor = bodyReading.MassKg * entry.CouplingScale;
        forceN = { fieldVec[0] * factor, fieldVec[1] * factor, fieldVec[2] * factor };
    }
    else
    {
        // Aerodynamic quadratic drag: F = 0.5 * rho * Cd * A * |v_air - v_body| * (v_air - v_body)
        const float relX = fieldVec[0] - worldVel[0];
        const float relY = fieldVec[1] - worldVel[1];
        const float relZ = fieldVec[2] - worldVel[2];
        const float relSpeed = std::sqrt(relX * relX + relY * relY + relZ * relZ);
        const float qFactor = 0.5f * entry.AirDensityKgPerM3 * entry.DragCoefficient * entry.DragAreaM2 * relSpeed * entry.CouplingScale;
        forceN = { relX * qFactor, relY * qFactor, relZ * qFactor };
    }

    return ClampForceMagnitude(forceN, entry.MaximumForceNewtons);
}

Mathematics::Axis3D ComputeVehicleWindForceNewtons(
    const ForceFields::PublishedForceFieldReading& reading,
    const RigidBodyReading& chassisReading,
    const float frontalAreaM2,
    const float sideAreaM2,
    const float dragCoefficient,
    const float airDensityKgPerM3,
    const double simulationSeconds) noexcept
{
    if (!chassisReading.Resident || chassisReading.MassKg <= 0.0f || reading.ActiveCount == 0u)
    {
        return { 0.0f, 0.0f, 0.0f };
    }

    const std::array<float, 3> worldPos { chassisReading.PositionMeters.X, chassisReading.PositionMeters.Y, chassisReading.PositionMeters.Z };
    const std::array<float, 3> zeroVel  { 0.0f, 0.0f, 0.0f };
    const std::array<float, 3> windAir = ForceFields::EvaluateTotalForceField(
        reading.ActiveSpan(),
        worldPos,
        zeroVel,
        static_cast<float>(simulationSeconds),
        static_cast<uint32_t>(ForceFields::ForceTargetChannelBits::Rigid));

    // Relative wind vector in world space (RHR +Z Up)
    const Mathematics::Axis3D relWorld {
        windAir[0] - chassisReading.LinearVelocity.X,
        windAir[1] - chassisReading.LinearVelocity.Y,
        windAir[2] - chassisReading.LinearVelocity.Z
    };

    // Transform into chassis-local basis (X = forward, Y = left/right lateral, Z = up)
    const Mathematics::Axis3D relLocal = RigidBodyDynamics::RotateByConjugateQuaternion(chassisReading.Orientation, relWorld);
    const float qHalf = 0.5f * airDensityKgPerM3 * dragCoefficient;

    const Mathematics::Axis3D localForce {
        qHalf * frontalAreaM2 * std::fabs(relLocal.X) * relLocal.X,
        qHalf * sideAreaM2    * std::fabs(relLocal.Y) * relLocal.Y,
        qHalf * frontalAreaM2 * 0.5f * std::fabs(relLocal.Z) * relLocal.Z
    };

    return ClampForceMagnitude(RigidBodyDynamics::RotateByQuaternion(chassisReading.Orientation, localForce), 250000.0f);
}

//------------------------------------------------------------------------------------------------------------------------
//                                               FORCEFIELDPHYSICSCOUPLING
//------------------------------------------------------------------------------------------------------------------------

void ForceFieldPhysicsCoupling::RegisterRigidBody(const CoupledRigidBodyEntry& entry)
{
    for (CoupledRigidBodyEntry& existing : CoupledBodies)
    {
        if (existing.BodyToken == entry.BodyToken)
        {
            existing = entry;
            return;
        }
    }
    CoupledBodies.push_back(entry);
}

bool ForceFieldPhysicsCoupling::UnregisterRigidBody(const RigidBodyToken bodyToken) noexcept
{
    for (auto it = CoupledBodies.begin(); it != CoupledBodies.end(); ++it)
    {
        if (it->BodyToken == bodyToken)
        {
            CoupledBodies.erase(it);
            return true;
        }
    }
    return false;
}

ForceCouplingStepSummary ForceFieldPhysicsCoupling::ApplyPublishedForceFields(
    PhysicsExtension& physics,
    const ForceFields::ForceFieldRegister& forceRegister,
    const double simulationSeconds) const noexcept
{
    ForceCouplingStepSummary summary {};
    if (CoupledBodies.empty()) return summary;

    auto lease = forceRegister.AcquirePublishedReading();
    if (!lease.Valid()) return summary;
    const auto& reading = lease.InspectPayload();
    if (reading.ActiveCount == 0u) return summary;

    for (const CoupledRigidBodyEntry& entry : CoupledBodies)
    {
        if (!entry.BodyToken.Valid()) continue;
        const RigidBodyReading bodyReading = physics.InspectBodyReading(entry.BodyToken);
        if (!bodyReading.Resident || bodyReading.MassKg <= 0.0f) continue;

        const Mathematics::Axis3D forceN = ComputeCoupledRigidForceNewtons(entry, reading, bodyReading, simulationSeconds);
        const float mag = std::sqrt(forceN.X * forceN.X + forceN.Y * forceN.Y + forceN.Z * forceN.Z);
        if (mag > 1.0e-6f)
        {
            physics.ApplyForce(entry.BodyToken, forceN);
            ++summary.CoupledBodyCount;
            summary.PeakForceNewtons = std::max(summary.PeakForceNewtons, mag);
            summary.TotalForceNewtons += mag;
        }
    }
    return summary;
}

Mathematics::Axis3D ForceFieldPhysicsCoupling::ApplyPublishedWindToVehicleChassis(
    PhysicsExtension& physics,
    const RigidBodyToken chassisToken,
    const ForceFields::ForceFieldRegister& forceRegister,
    const float frontalAreaM2,
    const float sideAreaM2,
    const float dragCoefficient,
    const double simulationSeconds,
    const float airDensityKgPerM3) const noexcept
{
    if (!chassisToken.Valid()) return { 0.0f, 0.0f, 0.0f };
    auto lease = forceRegister.AcquirePublishedReading();
    if (!lease.Valid()) return { 0.0f, 0.0f, 0.0f };

    const RigidBodyReading chassisReading = physics.InspectBodyReading(chassisToken);
    const Mathematics::Axis3D forceN = ComputeVehicleWindForceNewtons(
        lease.InspectPayload(),
        chassisReading,
        frontalAreaM2,
        sideAreaM2,
        dragCoefficient,
        airDensityKgPerM3,
        simulationSeconds);

    const float magSq = forceN.X * forceN.X + forceN.Y * forceN.Y + forceN.Z * forceN.Z;
    if (magSq > 1.0e-12f)
    {
        physics.ApplyForce(chassisToken, forceN);
    }
    return forceN;
}

uint32_t ForceFieldPhysicsCoupling::ApplyGpuImpulseReadback(
    PhysicsExtension& physics,
    const std::span<const RigidImpulseAccumulatorEntry> readbackEntries) const noexcept
{
    uint32_t appliedCount = 0u;
    for (const RigidImpulseAccumulatorEntry& entry : readbackEntries)
    {
        if (!entry.BodyToken.Valid() || entry.ContactCount == 0u) continue;
        const Mathematics::Axis3D impulse { entry.LinearImpulse[0], entry.LinearImpulse[1], entry.LinearImpulse[2] };
        const Mathematics::Axis3D contact { entry.ContactPoint[0], entry.ContactPoint[1], entry.ContactPoint[2] };
        const float magSq = impulse.X * impulse.X + impulse.Y * impulse.Y + impulse.Z * impulse.Z;
        if (magSq > 1.0e-12f)
        {
            physics.ApplyImpulseAtPoint(entry.BodyToken, impulse, contact);
            ++appliedCount;
        }
    }
    return appliedCount;
}

} // namespace Frontier::SimulationSpace::PhysicsSpace
