#include "GPUMonitor.h"
#include <cstdio>
#include <sstream>
#include <string>
#include <thread>
#include <algorithm>
#include <chrono>

using namespace std;

GpuSample readGpuUsage() {
    GpuSample sample;   // declare the sample structure to hold GPU usage data

    FILE* pipe = _popen(
        "nvidia-smi --query-gpu=utilization.gpu,memory.used "
        "--format=csv,noheader,nounits -i 0 2>NUL",
        "r"
    );  // open a pipe to run the nvidia-smi command and read its output

    if (pipe == nullptr) {  // open check
        return sample;
    }

    char buffer[256];   // buffer to read the output of the nvidia-smi command

    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {   // read a line of output from the nvidia-smi command
        string line(buffer);
        stringstream ss(line);

        string gpuValue;
        string vramValue;
        // attempt to parse the GPU and VRAM usage values
        if (getline(ss, gpuValue, ',') and getline(ss, vramValue, ',')) {
            try {   
                sample.gpuUsage = stod(gpuValue);   // convert the GPU usage string to a double
                sample.vramUsedMB = stod(vramValue);   // convert the VRAM usage string to a double
                sample.valid = true;
            }
            catch (...) {
                // Leave measurement marked as unavailable
                // leave empty --< this is part of the test function. Return Error ? Blow everything
            }
        }
    }
    _pclose(pipe);   // close the pipe to the nvidia-smi command
    return sample;
}


void monitorGpu(
    std::atomic<bool>& monitoring,
    double& peakVram,
    double& totalGpuUsage,
    int& sampleCount
) {
    while (monitoring.load()) {
        GpuSample sample = readGpuUsage();

        if (sample.valid) {
            peakVram = std::max(peakVram, sample.vramUsedMB);
            totalGpuUsage += sample.gpuUsage;
            sampleCount++;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(500)
        );
    }
}