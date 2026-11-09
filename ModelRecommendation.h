#ifndef MODEL_RECOMMENDATION_H
#define MODEL_RECOMMENDATION_H

#include <string>
#include <vector>

struct ModelInfo {
    std::string name;
    double responseTime;
    double peakRAM;
    double peakVRAM;
    double suggestedRAM;
};

struct HardwareInfo {
    double totalRAM = 0;
    double availableRAM = 0;
    double availableVRAM = 0;
};

enum class Compatibility { Recommended, Compatible, Limited, NotRecommended };

std::vector<ModelInfo> getModels();
const ModelInfo* findModel(const std::vector<ModelInfo>& models, const std::string& name);
HardwareInfo detectHardware();
Compatibility checkCompatibility(const HardwareInfo& hardware, const ModelInfo& model);
const char* compatibilityLabel(Compatibility result);
void showRecommendations(const HardwareInfo& hardware, const std::vector<ModelInfo>& models);
void showRecommendation(const HardwareInfo& hardware, const ModelInfo& model);

#endif
