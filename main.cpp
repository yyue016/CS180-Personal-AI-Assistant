#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ModelRecommendation.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;
static std::wstring quoteArgument(const std::wstring& value) {
    std::wstring result = L"\"";
    size_t slashes = 0;
    for (wchar_t ch : value) {
        if (ch == L'\\') {
            ++slashes;
        } 
        else if (ch == L'"') {
            result.append(slashes * 2 + 1, L'\\');
            result += ch;
            slashes = 0;
        } 
        else {
            result.append(slashes, L'\\');
            result += ch;
            slashes = 0;
        }
    }
    result.append(slashes * 2, L'\\');
    return result + L'"';
}

static std::wstring utf8ToWide(const std::string& input) {
    if (input.empty()) return L"";
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input.data(), static_cast<int>(input.size()), nullptr, 0);
    if (count == 0) return L"";
    std::wstring output(count, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input.data(), static_cast<int>(input.size()), output.data(), count);
    return output;
}

static bool runOllama( const std::vector<std::wstring>& args, double* elapsedSeconds = nullptr) {
    std::wstring command = L"ollama";

    for (const auto& arg : args) {
        command += L" " + quoteArgument(arg);
    }

    std::vector<wchar_t> mutableCommand(
        command.begin(), command.end()
    );

    mutableCommand.push_back(L'\0');
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    auto start = Clock::now();
    BOOL started = CreateProcessW(nullptr,mutableCommand.data(),nullptr,nullptr,FALSE,0,nullptr,nullptr,&startup,&process);

    if (!started) {
        std::cerr << "Failed to start Ollama. Error: " << GetLastError() << std::endl;
        return false;
    }
    WaitForSingleObject(process.hProcess, INFINITE);
    auto end = Clock::now();
    DWORD exitCode = 1;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    if (elapsedSeconds != nullptr) {
        *elapsedSeconds = std::chrono::duration<double>(end - start).count();
    }
    return exitCode == 0;
}

static bool askYesNo(const std::string& message) {
    std::string answer;
    std::cout << message << " [y/N]: ";
    if (!std::getline(std::cin, answer)) return false;
    return answer == "y" || answer == "Y" || answer == "yes" || answer == "YES";
}

int main() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    std::cout << "=== Ollama Model Manager ===\n";
    const HardwareInfo hardware = detectHardware();
    const std::vector<ModelInfo> models = getModels();
    showRecommendations(hardware, models);

    std::string modelName;
    const ModelInfo* selected = nullptr;
    while (!selected) {
        std::cout << "\nSelect an exact model name (or 'exit'): ";
        if (!std::getline(std::cin, modelName) || modelName == "exit") return 0;
        selected = findModel(models, modelName);
        if (!selected) std::cout << "Unknown model. Choose one from the list.\n";
    }
    showRecommendation(hardware, *selected);

    Compatibility assessment = checkCompatibility(hardware, *selected);
    if (assessment == Compatibility::Limited || assessment == Compatibility::NotRecommended) {
        std::cout << "WARNING: This model might run slowly or fail on this computer.\n";
        if (!askYesNo("Continue anyway?")) return 0;
    }

    std::cout << "\nChecking Ollama installation...\n";
    if (!runOllama({L"--version"})) return 1;
    std::cout << "\nDownloading model (already downloaded models are usually reused)...\n";
    if (!runOllama({L"pull", utf8ToWide(modelName)})) {
        std::cerr << "Model download failed.\n";
        return 1;
    }

    std::cout << "\nReady. Enter a question and press Enter. Type /exit to quit.\n";
    std::cout << "Each question is an independent Ollama request (no previous chat context).\n";
    while (true) {
        std::string prompt;
        std::cout << "\nYou> ";
        if (!std::getline(std::cin, prompt) || prompt == "/exit") break;
        if (prompt.empty()) continue;
        auto widePrompt = utf8ToWide(prompt);
        if (widePrompt.empty()) {
            std::cerr << "The question could not be converted from UTF-8.\n";
            continue;
        }
        std::cout << "Model> " << std::flush;
        double elapsed = 0.0;
        bool success = runOllama({L"run", utf8ToWide(modelName), widePrompt}, &elapsed);
        std::cout << "\n[Response elapsed time: " << std::fixed << std::setprecision(3)
                  << elapsed << " seconds]";
        if (!success) std::cout << " [Ollama request failed]";
        std::cout << '\n';
    }
    std::cout << "Goodbye!\n";
    return 0;
}
