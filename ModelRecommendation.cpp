#include "ModelRecommendation.h"
#include <windows.h>
#include <algorithm>
#include <iomanip>
#include <iostream>

std::vector<ModelInfo> getModels() {
    // Measurements are reference values, not guaranteed timings on this PC.
    return {
        {"qwen3:0.6b", 1.99594, 1.04, 1.77, 8},
        {"gemma3:1b", 5.51727, 1.66, 2.26, 8},
        {"llama3.2:1b", 2.98075, 0.95, 1.90, 8},
        {"deepseek-r1:1.5b", 2.72241, 0.78, 1.57, 8},
        {"qwen3:1.7b", 4.80791, 1.46, 2.07, 8},
        {"llama3.2:3b", 3.68654, 3.19, 4.56, 12},
        {"qwen3:4b", 14.5312, 2.58, 3.94, 16},
        {"deepseek-r1:7b", 7.76875, 1.07, 4.91, 16},
        {"qwen3:8b", 19.7326, 2.10, 5.69, 16}
    };
}

const ModelInfo* findModel(const std::vector<ModelInfo>& models, const std::string& name) {
    for (const auto& model : models) {
        if (model.name == name) return &model;
    }
    return nullptr;
}

HardwareInfo detectHardware() {
    HardwareInfo hardware;
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    if (!GlobalMemoryStatusEx(&status)) {
        std::cerr << "Failed to detect RAM (Windows error " << GetLastError() << ").\n";
        return hardware;
    }
    constexpr double GiB = 1024.0 * 1024 * 1024;
    hardware.totalRAM = status.ullTotalPhys / GiB;
    hardware.availableRAM = status.ullAvailPhys / GiB;
    return hardware;
}

Compatibility checkCompatibility(const HardwareInfo& hw, const ModelInfo& model) {
    if (hw.totalRAM <= 0 || hw.totalRAM < model.suggestedRAM * 0.75)
        return Compatibility::NotRecommended;
    if (hw.totalRAM < model.suggestedRAM)
        return Compatibility::Limited;
    if (hw.availableRAM < model.peakRAM)
        return Compatibility::Limited;
    if (hw.totalRAM >= model.suggestedRAM * 1.25)
        return Compatibility::Recommended;
    return Compatibility::Compatible;
}

const char* compatibilityLabel(Compatibility c) {
    switch (c) {
        case Compatibility::Recommended: return "Recommended";
        case Compatibility::Compatible: return "Compatible";
        case Compatibility::Limited: return "Limited";
        default: return "Not Recommended";
    }
}

void showRecommendations(const HardwareInfo& hw, const std::vector<ModelInfo>& models) {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n===== Detected hardware =====\n"<< "Total RAM: " << hw.totalRAM << " GiB\n"  << "Available RAM: " << hw.availableRAM << " GiB\n"  << "VRAM: not detected (GPU suitability unknown)\n\n";
    std::cout << "===== Models (RAM-only preliminary guidance) =====\n";
    std::cout << std::left << std::setw(22) << "Model"  << std::setw(17) << "Suggested RAM"  << "Assessment\n";
    for (const auto& model : models) {
        std::cout << std::left << std::setw(22) << model.name  << std::setw(17) << (std::to_string(static_cast<int>(model.suggestedRAM)) + " GiB")  << compatibilityLabel(checkCompatibility(hw, model)) << '\n';
    }
    std::cout << "Note: these estimates do not guarantee GPU support or response speed.\n";
}

void showRecommendation(const HardwareInfo& hw, const ModelInfo& model) {
    std::cout << "\n===== Selected Model =====\n" << "Name: " << model.name << '\n' << "Suggested RAM: " << model.suggestedRAM << " GiB\n"<< "Reference peak RAM: " << model.peakRAM << " GiB\n" << "Reference peak VRAM: " << model.peakVRAM << " GiB\n" << "Reference response time: " << model.responseTime << " seconds\n"  << "RAM assessment: " << compatibilityLabel(checkCompatibility(hw, model)) << '\n';
}
