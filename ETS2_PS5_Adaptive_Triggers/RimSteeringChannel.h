#pragma once

#include <Windows.h>
#include <cmath>

namespace RimSteeringChannel {
constexpr wchar_t mappingName[] = L"Local\\ETS2_PS5_RimSteering_v1";
constexpr wchar_t publisherName[] = L"Local\\ETS2_PS5_RimSteeringPublisher_v1";
constexpr unsigned positionBits = 21;
constexpr ULONGLONG positionMask = (1ULL << positionBits) - 1;
constexpr LONG positionScale = 1000000;
constexpr ULONGLONG sampleTimeoutMs = 1000;

// Store the timestamp and axis position in one atomic sample.
inline LONG64 encode(double position, ULONGLONG now) {
    const auto offset = std::lround(position * positionScale) + positionScale;
    return static_cast<LONG64>((now << positionBits) | offset);
}

inline float decode(LONG64 sample, ULONGLONG now) {
    const auto bits = static_cast<ULONGLONG>(sample);
    const auto timestamp = bits >> positionBits;
    const auto offset = bits & positionMask;
    if (sample == 0 || now - timestamp >= sampleTimeoutMs || offset > 2ULL * positionScale) {
        return 0.0f;
    }
    return static_cast<float>(static_cast<LONG>(offset) - positionScale) / positionScale;
}

class Publisher {
public:
    Publisher() = default;
    Publisher(const Publisher&) = delete;
    Publisher& operator=(const Publisher&) = delete;

    ~Publisher() {
        if (sample_) {
            InterlockedExchange64(sample_, 0);
            UnmapViewOfFile(sample_);
        }
        if (mapping_) CloseHandle(mapping_);
        if (publisher_) CloseHandle(publisher_);
    }

    const char* open() {
        // Only the app holds this handle, so Windows releases the claim on exit.
        publisher_ = CreateMutexW(nullptr, FALSE, publisherName);
        if (!publisher_) return "Could not create the steering publisher.";
        if (GetLastError() == ERROR_ALREADY_EXISTS) return "Rim steering is already running.";

        mapping_ = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
            0, sizeof(LONG64), mappingName);
        if (!mapping_) return "Could not create the steering channel.";
        sample_ = static_cast<LONG64*>(MapViewOfFile(mapping_, FILE_MAP_WRITE, 0, 0, sizeof(LONG64)));
        if (!sample_) return "Could not map the steering channel.";
        publish(0.0);
        return nullptr;
    }

    void publish(double position) const {
        InterlockedExchange64(sample_, encode(position, GetTickCount64()));
    }

private:
    HANDLE publisher_ = nullptr;
    HANDLE mapping_ = nullptr;
    LONG64* sample_ = nullptr;
};
}
