#ifndef GPU_MONITOR_H
#define GPU_MONITOR_H
#include <atomic>

struct GpuSample {
    double gpuUsage = -1.0;
    double vramUsedMB = -1.0;

    bool valid = false;
};

void monitorGpu(
    std::atomic<bool>& monitoring,
    double& peakVram,
    double& totalGpuUsage,
    int& sampleCount
);

GpuSample readGpuUsage();

#endif