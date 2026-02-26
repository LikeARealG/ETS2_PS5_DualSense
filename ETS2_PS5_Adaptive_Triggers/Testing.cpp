#include <iostream>
#include <Windows.h>
#include <chrono>
#include <thread>
#include <vector>
#include <iomanip>

#define ETS2_SHARED_MEMORY_NAME L"Local\\SimTelemetryETS2"
#define SHARED_MEMORY_SIZE 1024
#define ENGINE_RPM_OFFSET 80
#define ENGINE_STATUS_OFFSET 598

int main() {
    // Open the shared memory
    HANDLE hMapFile = OpenFileMapping(FILE_MAP_READ, FALSE, ETS2_SHARED_MEMORY_NAME);
    if (hMapFile == NULL) {
        std::cerr << "Error: Could not open shared memory." << std::endl;
        return 1;
    }

    // Map the view of the file
    unsigned char* raw_data = (unsigned char*)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, SHARED_MEMORY_SIZE);
    if (raw_data == NULL) {
        std::cerr << "Error: Could not map view of file." << std::endl;
        CloseHandle(hMapFile);
        return 1;
    }

    // Start timing
    auto start_time = std::chrono::steady_clock::now();
    std::vector<std::tuple<std::chrono::milliseconds, bool, float, std::vector<float>>> data;

    // Collect data for 10 seconds
    while (true) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
        if (elapsed.count() >= 10000) break;

        // Read engine status and current RPM
        bool engine_on = (*(bool*)(raw_data + ENGINE_STATUS_OFFSET)) == 0;
        float current_rpm = *(float*)(raw_data + ENGINE_RPM_OFFSET);

        // Read additional floats from offsets 84 to 120
        std::vector<float> additional_floats;
        for (int offset = 84; offset <= 120; offset += 4) {
            additional_floats.push_back(*(float*)(raw_data + offset));
        }

        // Store the data
        data.push_back(std::make_tuple(elapsed, engine_on, current_rpm, additional_floats));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Clean up
    UnmapViewOfFile(raw_data);
    CloseHandle(hMapFile);

    // Output header
    std::cout << "timestamp_ms,engine_on,current_rpm";
    for (int i = 0; i <= (120 - 84) / 4; ++i) {
        int offset = 84 + i * 4;
        std::cout << ",float" << offset;
    }
    std::cout << std::endl;

    // Output data
    for (const auto& entry : data) {
        std::cout << std::get<0>(entry).count() << "," << std::get<1>(entry) << ","
            << std::fixed << std::setprecision(2) << std::get<2>(entry);
        for (float val : std::get<3>(entry)) {
            std::cout << "," << std::fixed << std::setprecision(2) << val;
        }
        std::cout << std::endl;
    }

    return 0;
}