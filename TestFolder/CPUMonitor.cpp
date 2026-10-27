#include "CpuMonitor.h"
#include <windows.h>

// Implementation of CPU monitoring functions defined in CpuMonitor.h
CpuTimes readCpuTimes() {
    CpuTimes result;    // this is the structure that will hold the CPU times read from the system

    FILETIME idleTime;  // declare the FILETIME structure for idle time
    FILETIME kernelTime;  // declare the FILETIME structure for kernel time
    FILETIME userTime;  // declare the FILETIME structure for user time

    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {   // if the function fails, return the result as is
        return result;
    }

    auto convert = [](const FILETIME& time) {   // lambda function to convert FILETIME to unsigned long long
        ULARGE_INTEGER value;

        value.LowPart = time.dwLowDateTime; // assign the low part of the FILETIME to the ULARGE_INTEGER
        value.HighPart = time.dwHighDateTime; // assign the high part of the FILETIME to the ULARGE_INTEGER

        return value.QuadPart;  // return the 64-bit integer representation of the FILETIME
    };

    result.idle = convert(idleTime);
    result.kernel = convert(kernelTime);
    result.user = convert(userTime);
    result.valid = true;

    return result;
}
// double function to calculate CPU usage between two CpuTimes structures
double calculateCpuUsage(
    const CpuTimes& before,
    const CpuTimes& after
) {
    if (!before.valid or !after.valid) {
        return -1.0;
    }

    if (after.idle < before.idle or after.kernel < before.kernel or after.user < before.user) {
        return -1.0;    // return error if any of the recorded after times are less than the before times
    }
    // calculate the differences between the before and after CPU times
    unsigned long long idleDiff = after.idle - before.idle;

    unsigned long long kernelDiff = after.kernel - before.kernel;

    unsigned long long userDiff = after.user - before.user;

    unsigned long long totalDiff = kernelDiff + userDiff;

    // check for division by zero
    if (totalDiff == 0) {
        return -1.0;
    }
    // calculate the CPU usage as a percentage of total time spent not idle
    double usage = (1.0 - static_cast<double>(idleDiff) / totalDiff) * 100.0;

    return usage;
}