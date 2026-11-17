#pragma once

#include <algorithm>
#include <cmath>

class RimSteering {
public:
    // Match ETS2's 900-degree lock-to-lock animation in absolute steering mode.
    static constexpr float defaultLockDegrees = 450.0f;
    static constexpr float defaultMaxDegreeDeviation = 5.0f;
    static constexpr float defaultMaxDeviationSpeedKmph = 90.0f;

    explicit RimSteering(double lockDegrees = defaultLockDegrees,
                         double maxDegreeDeviation = defaultMaxDegreeDeviation,
                         double maxDeviationSpeedKmph = defaultMaxDeviationSpeedKmph)
        : steeringLock_(lockDegrees * oneTurn / 360.0),
          maxDegreeDeviation_(maxDegreeDeviation),
          maxDeviationSpeedKmph_(maxDeviationSpeedKmph) {}

    // DS5W reports positive X to the right and positive Y upward.
    double update(int x, int y, double speedKmph = 0.0) {
        constexpr double stickRange = 127.0;
        constexpr double gripRadius = 0.78 * stickRange;
        constexpr double releaseRadius = 0.68 * stickRange;
        const double radiusSquared = x * x + y * y;
        const double threshold = gripped_ ? releaseRadius : gripRadius;
        if (radiusSquared < threshold * threshold) {
            gripped_ = false;
            return wheelAngle_ / steeringLock_;
        }

        const double angle = std::atan2(static_cast<double>(x), static_cast<double>(y));
        if (gripped_) {
            const double delta = std::remainder(angle - previousAngle_, oneTurn);
            const double speed = std::isfinite(speedKmph) ? std::abs(speedKmph) : 0.0;
            const double speedFraction = std::min(1.0, speed / maxDeviationSpeedKmph_);
            const double degreeDeviation = 1.0 + speedFraction * (maxDegreeDeviation_ - 1.0);
            wheelAngle_ = std::max(-steeringLock_, std::min(steeringLock_, wheelAngle_ + delta / degreeDeviation));
        }
        gripped_ = true;
        previousAngle_ = angle;
        return wheelAngle_ / steeringLock_;
    }

private:
    static constexpr double oneTurn = 6.28318530717958647692;
    const double steeringLock_;
    const double maxDegreeDeviation_;
    const double maxDeviationSpeedKmph_;
    bool gripped_ = false;
    double previousAngle_ = 0.0;
    double wheelAngle_ = 0.0;
};
