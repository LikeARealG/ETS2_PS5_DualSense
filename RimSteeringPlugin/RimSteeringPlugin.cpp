#include "RimSteeringChannel.h"
#include "scssdk_input.h"

namespace {
HANDLE mapping = nullptr;
LONG64* sample = nullptr;
ULONGLONG nextConnectionAttempt = 0;

float readSteering() {
    const ULONGLONG now = GetTickCount64();
    if (!sample && now >= nextConnectionAttempt) {
        nextConnectionAttempt = now + 500;
        mapping = OpenFileMappingW(FILE_MAP_WRITE, FALSE, RimSteeringChannel::mappingName);
        if (mapping) {
            sample = static_cast<LONG64*>(MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, sizeof(LONG64)));
            if (!sample) {
                CloseHandle(mapping);
                mapping = nullptr;
            }
        }
    }
    if (!sample) return 0.0f;
    const LONG64 reading = InterlockedCompareExchange64(sample, 0, 0);
    return RimSteeringChannel::decode(reading, GetTickCount64());
}

SCSAPI_RESULT pollSteering(scs_input_event_t* event, const scs_u32_t flags, scs_context_t) {
    if (!(flags & SCS_INPUT_EVENT_CALLBACK_FLAG_first_in_frame)) return SCS_RESULT_not_found;
    event->input_index = 0;
    event->value_float.value = readSteering();
    return SCS_RESULT_ok;
}
}

SCSAPI_RESULT scs_input_init(const scs_u32_t version, const scs_input_init_params_t* params) {
    if (version != SCS_INPUT_VERSION_1_00) return SCS_RESULT_unsupported;
    const auto* inputParams = static_cast<const scs_input_init_params_v100_t*>(params);

    static const scs_input_device_input_t inputs[] = {
        { "steering", "Rim Steering Axis", SCS_VALUE_TYPE_float, 0 }
    };
    const scs_input_device_t device = {
        "dualsense_rim", "DualSense Rim Steering", SCS_INPUT_DEVICE_TYPE_generic,
        1, inputs, nullptr, nullptr, pollSteering
    };
    return inputParams->register_device(&device);
}

SCSAPI_VOID scs_input_shutdown() {
    if (sample) UnmapViewOfFile(sample);
    if (mapping) CloseHandle(mapping);
    sample = nullptr;
    mapping = nullptr;
    nextConnectionAttempt = 0;
}
