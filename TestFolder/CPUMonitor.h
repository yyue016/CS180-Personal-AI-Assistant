#ifndef CPU_MONITOR_H
#define CPU_MONITOR_H

struct CpuTimes {
    unsigned long long idle = 0;
    unsigned long long kernel = 0;
    unsigned long long user = 0;

    bool valid = false;
};

CpuTimes readCpuTimes();

double calculateCpuUsage(
    const CpuTimes& before,
    const CpuTimes& after
);

#endif