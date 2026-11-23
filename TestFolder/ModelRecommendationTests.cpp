#include "ModelRecommendation.h"

#include <algorithm>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

namespace {
int checks = 0;
int failures = 0;

// helper function to record a test check and print a message if it fails
void expect(bool passed, const char* description) {
    ++checks;
    if (!passed) {
        ++failures;
        std::cerr << "FAIL: " << description << '\n';
    }
}

// test functions for the ModelRecommendation module
void testCatalog() {
    const auto models = getModels();
    std::set<std::string> names;
    for (const auto& model : models) {
        expect(!model.name.empty(), "catalog entries have names");
        expect(names.insert(model.name).second, "catalog names are unique");
        expect(model.suggestedRAM > 0 && model.peakRAM > 0 &&
               model.peakVRAM > 0 && model.responseTime > 0,
               "catalog measurements are positive");
    }
    const auto entry = std::find_if(models.begin(), models.end(), [](const ModelInfo& model) {
        return model.name == "qwen3:0.6b";
    });
    expect(entry != models.end(), "catalog includes qwen3:0.6b");
    if (entry != models.end()) {
        expect(entry->suggestedRAM == 8, "qwen3:0.6b suggests 8 GiB");
    }
}

void testLookup() {
    const std::vector<ModelInfo> models = {
        {"first:1b", 2, 1, 1, 8},
        {"second:3b", 3, 2, 2, 12},
    };
    expect(findModel(models, "second:3b") == &models[1], "lookup returns the matching entry");
    expect(findModel(models, "missing:1b") == nullptr, "unknown model returns nullptr");
    expect(findModel(models, "SECOND:3B") == nullptr, "lookup matches case exactly");
    expect(findModel({}, "first:1b") == nullptr, "empty catalog returns nullptr");
}

void testCompatibility() {
    using C = Compatibility;
    const ModelInfo model{"test:1b", 1, 2, 1, 8};
    const struct {
        double totalRAM;
        double availableRAM;
        C expected;
        const char* description;
    } cases[] = {
        {0, 0, C::NotRecommended, "undetected RAM"},
        {5.99, 3, C::NotRecommended, "below 75% of suggested RAM"},
        {6, 3, C::Limited, "exactly 75% of suggested RAM"},
        {7.99, 3, C::Limited, "just below suggested RAM"},
        {8, 1.99, C::Limited, "available RAM below peak requirement"},
        {8, 2, C::Compatible, "total and available RAM meet requirements exactly"},
        {9.99, 3, C::Compatible, "just below 125% of suggested RAM"},
        {10, 2, C::Recommended, "exactly 125% of suggested RAM"},
        {12, 1.99, C::Limited, "low available RAM takes priority over high total RAM"},
    };
    for (const auto& test : cases) {
        expect(checkCompatibility({test.totalRAM, test.availableRAM, 0}, model) == test.expected,
               test.description);
    }
}

void testLabels() {
    using C = Compatibility;
    expect(std::string(compatibilityLabel(C::Recommended)) == "Recommended", "Recommended label");
    expect(std::string(compatibilityLabel(C::Compatible)) == "Compatible", "Compatible label");
    expect(std::string(compatibilityLabel(C::Limited)) == "Limited", "Limited label");
    expect(std::string(compatibilityLabel(C::NotRecommended)) == "Not Recommended",
           "Not Recommended label");
}

void testHardwareDetection() {
    const auto hw = detectHardware();
    expect(hw.totalRAM > 0, "Windows reports positive total RAM");
    expect(hw.availableRAM >= 0 && hw.availableRAM <= hw.totalRAM,
           "available RAM is within the physical RAM range");
    expect(hw.availableVRAM == 0, "VRAM remains at its default value");
}

void testRecommendationOutput() {
    const ModelInfo light{"light:1b", 3, 2, 1, 8};
    const ModelInfo heavy{"heavy:8b", 8, 6, 4, 16};
    const HardwareInfo hw{10, 4, 0};
    std::ostringstream catalogOutput;
    std::ostringstream selectedOutput;
    const auto flags = std::cout.flags();
    const auto precision = std::cout.precision();
    auto* original = std::cout.rdbuf(catalogOutput.rdbuf());
    showRecommendations(hw, {light, heavy});
    std::cout.rdbuf(selectedOutput.rdbuf());
    showRecommendation(hw, heavy);
    std::cout.rdbuf(original);
    std::cout.flags(flags);
    std::cout.precision(precision);

    const auto catalog = catalogOutput.str();
    std::istringstream rows(catalog);
    std::string row;
    bool lightRecommended = false;
    bool heavyNotRecommended = false;
    while (std::getline(rows, row)) {
        if (row.find("light:1b") == 0) {
            lightRecommended = row.find("Recommended") != std::string::npos &&
                               row.find("Not Recommended") == std::string::npos;
        }
        if (row.find("heavy:8b") == 0) {
            heavyNotRecommended = row.find("Not Recommended") != std::string::npos;
        }
    }
    expect(catalog.find("Total RAM: 10.00 GiB") != std::string::npos,
           "catalog output includes detected total RAM");
    expect(lightRecommended, "catalog pairs the light model with Recommended");
    expect(heavyNotRecommended, "catalog pairs the heavy model with Not Recommended");

    const auto selected = selectedOutput.str();
    expect(selected.find("Name: heavy:8b") != std::string::npos, "selected output names the model");
    expect(selected.find("Reference peak RAM: 6.00 GiB") != std::string::npos,
           "selected output shows the model's peak RAM");
    expect(selected.find("RAM assessment: Not Recommended") != std::string::npos,
           "selected output shows the model's assessment");
}
}

int main() {
    testCatalog();
    testLookup();
    testCompatibility();
    testLabels();
    testHardwareDetection();
    testRecommendationOutput();
    std::cout << checks - failures << '/' << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
