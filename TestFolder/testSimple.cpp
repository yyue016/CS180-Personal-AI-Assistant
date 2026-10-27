//#include "Modeldownload.cpp"
#include <string>
#include <vector>
#include <iostream>

#include <fstream>
#include <sstream>

#include <chrono>   // for measuring time intervals

#include <windows.h>    // used for measuring the RAM CPU GPU VRAM
#include <tlhelp32.h>
#include <psapi.h>
#include <algorithm>
#include <cwctype>

#include <thread>   // used for measure the peak RAM usage
#include <atomic>
#include <chrono>

#include "CPUMonitor.h" // used for monitoring CPU usage
#include "GPUMonitor.h" // used for monitoring GPU usage

using namespace std;

// function to check if a command is running successfully
bool commandrunning(const string&cmd,bool showError = true){
    int ret = system(cmd.c_str());
    if(ret != 0){
        if (showError) {
            cerr << "Command failed, return " << ret << endl;
        }
        return false;
    }
    else{
        return true;
    }
}

// Structure to hold the performance metrics of a prompt
struct PromptPerformance{
    double responseTime = 0.0;      // time taken to generate the response
    double tokensPerSecond = 0.0;   //generated tokens per second
    int outputTokens = 0;           //response length

    double ramMB = 0.0;            // RAM usage in megabytes
    double peakRam = -1.0;          // peak RAM usage in megabytes
    double RamBefore = -1.0;          // RAM usage before the prompt in megabytes
    double RamAfter = -1.0;           // RAM usage after the prompt in megabytes
    double cpuUsage = 0.0;          // CPU usage in percentage
    double gpuUsage = 0.0;          // GPU usage in percentage
    double Vram = 0.0;           // VRAM usage in megabytes
    double peakVram = -1.0;          // peak VRAM usage in megabytes

    bool success = false;           // indicates if the prompt was successfully processed


    void OutputResult() const {     // Outputs the performance metrics to the console
        cout << "Response Time: " << responseTime << " s" << endl;
        cout << "Tokens Per Second: " << tokensPerSecond << endl;
        cout << "Output Tokens: " << outputTokens << endl;
        cout << "Peak RAM Usage: " << peakRam << " MB" << endl;
        cout << "RAM Usage Before: " << RamBefore << " MB" << endl;
        cout << "RAM Usage After: " << RamAfter << " MB" << endl;
        cout << "RAM Usage: " << ramMB << " MB" << endl;
        cout << "CPU Usage: " << cpuUsage << " %" << endl;
        cout << "GPU Usage: " << gpuUsage << " %" << endl;
        cout << "VRAM Usage: " << Vram << " MB" << endl;
        cout << "Peak VRAM Usage: " << peakVram << " MB" << endl;
        cout << "Success: " << (success ? "Yes" : "No") << endl;
    }
};

// function to remove ANSI escape codes from a string
string removeAnsi(const string& input) {
    string cleaned;

    for (size_t i = 0; i < input.size(); i++) { // iterate through each character in the input string

        if (input[i] == '\x1B' and i + 1 < input.size() and input[i + 1] == '[') {  // detect ANSI escape sequence
            i += 2;
            while (i < input.size() and !(input[i] >= '@' and input[i] <= '~')) {   // skip the rest of the ANSI escape sequence
                i++;
            }

            continue;
        }

        cleaned += input[i];
    }

    return cleaned;
}

// function to display the AI response from a file
void processOllamaOutput(PromptPerformance& performance) {

    ifstream outputFile("ollama_output.txt");   // open the file containing the AI response

    if (!outputFile.is_open()) {
        cerr << "Failed to open Ollama output file." << endl;
        return;
    }

    string line;
    bool statisticsStarted = false;

    cout << "\n========== AI RESPONSE ==========\n";

    while (getline(outputFile, line)) {

        line = removeAnsi(line);  // remove ANSI escape codes from the line

        if (line.find("total duration:") != string::npos) {
            statisticsStarted = true;
        }

        // Display the AI response
        if (!statisticsStarted) {
            cout << line << endl;
        }

        // Extract output tokens
        if (line.find("eval count:") != string::npos and line.find("prompt eval count:") == string::npos) {

            size_t pos = line.find(":");
            string value = line.substr(pos + 1);

            performance.outputTokens = stoi(value);
        }

        // Extract generation speed
        if (line.find("eval rate:") != string::npos and line.find("prompt eval rate:") == string::npos) {

            size_t pos = line.find(":");
            string value = line.substr(pos + 1);

            performance.tokensPerSecond = stod(value);
        }
    }
    outputFile.close();
    cout << "\n========== END OF RESPONSE ==========\n";

}

// function to get the RAM usage of Ollama processes in megabytes
double getOllamaRam() {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);  // take a snapshot of all processes in the system

    if (snapshot == INVALID_HANDLE_VALUE) {
        return -1.0;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    unsigned long long totalBytes = 0;

    if (Process32FirstW(snapshot, &entry)) {    // iterate through all processes in the snapshot
        do {
            wstring name = entry.szExeFile; // get the process name

            transform(name.begin(), name.end(), name.begin(), towlower);    // convert the process name to lowercase for case-insensitive comparison

            if (name.find(L"ollama") != wstring::npos or name.find(L"llama") != wstring::npos) {    // check if the process is an Ollama or Llama process

                HANDLE process = OpenProcess(
                    PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
                    FALSE,
                    entry.th32ProcessID
                );  // open the process with the necessary access rights

                if (process != nullptr) {
                    PROCESS_MEMORY_COUNTERS pmc{};

                    if (GetProcessMemoryInfo(
                        process, &pmc, sizeof(pmc))) {

                        totalBytes += pmc.WorkingSetSize;
                    }   // add the process's working set size to the total

                    CloseHandle(process);
                }
            }

        } while (Process32NextW(snapshot, &entry));  // continue iterating through the processes in the snapshot
    }

    CloseHandle(snapshot);

    return static_cast<double>(totalBytes) / (1024.0 * 1024.0);
}

// function to monitor the peak RAM usage of Ollama processes in a separate thread
void monitorPeakRam(atomic<bool>& monitoring, double& peakRam) {
    // continuously monitor the RAM usage while the monitoring flag is set
    while (monitoring.load()) {

        double currentRam = getOllamaRam(); // get the current RAM usage of Ollama processes
        if (currentRam >= 0 && currentRam > peakRam) {  // update the peak RAM usage if the current usage is higher
            peakRam = currentRam;
        }
        this_thread::sleep_for(chrono::milliseconds(50));   // sleep for 50 milliseconds before checking again
    }
}

// function to run the model and measure its performance
PromptPerformance runPerformanceTest(
    const string& modelName,
    const string& prompt
){
    PromptPerformance performance;
    // Construct the command to run the model with the given prompt
    string command =
        "ollama run " + modelName +
        " --verbose \"" + prompt + "\""
        " > ollama_output.txt 2>&1";

    
    //system(command.c_str()); 
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Read everything before we run the model --> CPU GPU VRAM stuff
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // ram part
    performance.RamBefore = getOllamaRam();
    performance.peakRam = performance.RamBefore;

    atomic<bool> monitoring(true);
    thread ramMonitor(monitorPeakRam, ref(monitoring), ref(performance.peakRam));   // start the RAM monitoring thread

    GpuSample gpuBefore = readGpuUsage();
    double peakVram = gpuBefore.valid ? gpuBefore.vramUsedMB : -1.0;

    double totalGpuUsage = 0.0;
    int gpuSampleCount = 0;

    thread gpuMonitor(
        monitorGpu,
        ref(monitoring),
        ref(peakVram),
        ref(totalGpuUsage),
        ref(gpuSampleCount)
    );
    if (gpuBefore.valid) {
        performance.Vram = gpuBefore.vramUsedMB;
    }   // update the VRAM usage if the GPU usage data is valid
    
    // Start the timer, CPU monitor
    CpuTimes cpuBefore = readCpuTimes();
    auto start = chrono::steady_clock::now();
    // Run Ollama and wait for the response------------------------------------------------
    bool success = commandrunning(command);
    //-------------------------------------------------------------------------------------
    // Stop the timer, CPU monitor, RAM monitor after the AI model has finished running
    // ------------------------------------------------------------------------------------
    auto end = chrono::steady_clock::now();
    CpuTimes cpuAfter = readCpuTimes();

    // stop sampleing GPU usage, wait to ram and gpu thread complete
    monitoring.store(false);   // stop the RAM monitoring thread
    ramMonitor.join();   
    gpuMonitor.join();   

    // read info after run
    GpuSample gpuAfter = readGpuUsage();   // read the GPU usage after running the model
    performance.RamAfter = getOllamaRam();
    if (gpuAfter.valid) {
        cout << "GPU Usage After: " << gpuAfter.gpuUsage << " %" << endl;

        cout << "VRAM After: " << gpuAfter.vramUsedMB << " MiB" << endl;
    }
    
    //?????????????????????????????????????????????????????????????????????????????????? Calculation
    // calculate the RAM usage during the model run
    performance.ramMB = performance.RamAfter - performance.RamBefore;

    if (performance.RamAfter >= 0) {
        performance.peakRam = max(performance.peakRam, performance.RamAfter);
    }
    performance.cpuUsage = calculateCpuUsage(cpuBefore, cpuAfter);
    // Calculate average GPU usage
    if (gpuSampleCount > 0) {
        performance.gpuUsage = totalGpuUsage / gpuSampleCount;
    }else {
        performance.gpuUsage = -1.0;
    }

    performance.Vram = gpuBefore.valid ? gpuBefore.vramUsedMB : -1.0;   // get vram

    performance.peakVram = peakVram;            // get peak VRAM usage 

    if (gpuAfter.valid) {
        performance.peakVram = max(performance.peakVram, gpuAfter.vramUsedMB);
    }

    if (!success) {
        cerr << "Failed to run the model." << endl;
        return performance;
    }

    //calculate the response time
    performance.responseTime = chrono::duration<double>(end - start).count();
    //performance.success = true;

    // Open the output file to read the model's response
    processOllamaOutput(performance);

    if (performance.outputTokens > 0 and performance.tokensPerSecond > 0.0) {   // check if the performance metrics were successfully extracted

        performance.success = true;
    } else {
        cerr << "Failed to extract Ollama statistics." << endl;
    }
    return performance;
}


// check for Ollama Service
bool checkOllamaService() {

    cout << "Checking Ollama service..." << endl;

    int checkpath = system("Where.exe ollama");

    if (checkpath == 0) {
        cout << "Ollama found!" << endl;
    }
    else {
        cout << "Ollama installed, but not found in PATH." << endl;
        return false;
    }

    // Check if the Ollama service is running
    cout << "Checking if Ollama service is running..." << endl;

    if(!commandrunning("curl.exe -fsS --max-time 3 -o NUL " "http://127.0.0.1:11434/api/tags 2>nul")){

        cout << "Starting Ollama..." << endl;
        if (!commandrunning("start \"\" ollama serve")) {
            cerr << "Cannot start Ollama!" << endl;
            return false;
        }
        bool start = false;
        for (int i = 0; i < 30; i++){
            if(system("curl.exe -fsS --max-time 2 -o NUL "
                "http://127.0.0.1:11434/api/tags 2>nul") == 0){
            start = true;
            break;
            }
            system("timeout /t 1 /nobreak >nul 2>&1");
        }
        if(!start){
            cerr << " Ollama service does not started." << endl;
            return false;
        }
    }

    cout << "Ollama service is ready!" << endl;
    return true;
}

// load txt files for input prompts
vector<string> loadPrompts(const string& filename) {
    vector<string> prompts;
    ifstream file(filename);

    if (!file.is_open()) {  // validate the file was successfully opened
        cerr << "Failed to open: " << filename << endl;
        return prompts;
    }

    string line;

    while (getline(file, line)) {   // read each line from the file
        if (!line.empty() and line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            prompts.push_back(line);
        }
    }
    return prompts;
}

// calculate average performance for each prompt
void printAverageResults( const vector<PromptPerformance>& results) {
    if (results.empty()) {  // validation for empty results
        cout << "No successful results available.\n";
        return;
    }

    double totalTime = 0.0;     // initialize everything that needs total
    double totalSpeed = 0.0;
    long long totalTokens = 0;

    double totalCpu = 0.0;
    double totalGpu = 0.0;

    double totalPeakRam = 0.0;
    double totalPeakVram = 0.0;

    int gpuCount = 0;
    int ramCount = 0;
    int vramCount = 0;
    int cpuCount = 0;

    for (const PromptPerformance& result : results) {    // iterate through each result to accumulate totals
        totalTime += result.responseTime;
        totalSpeed += result.tokensPerSecond;
        totalTokens += result.outputTokens;

        if (result.cpuUsage >= 0) {
            totalCpu += result.cpuUsage;
            cpuCount++;
        }

        if (result.gpuUsage >= 0) {
            totalGpu += result.gpuUsage;
            gpuCount++;
        }

        if (result.peakRam >= 0) {
            totalPeakRam += result.peakRam;
            ramCount++;
        }

        if (result.peakVram >= 0) {
            totalPeakVram += result.peakVram;
            vramCount++;
        }
    }

    double count = static_cast<double>(results.size()); // total number of results as a double for averaging calculations

    cout << "\n========== MODEL SUMMARY ==========\n";
    cout << "Successful running " << results.size()  << " tests : " << endl;
    cout << "Average response time : " << totalTime / count << " s\n";
    cout << "Average output tokens : " << totalTokens / count << endl;
    cout << "Total output tokens : " << totalTokens << endl;
    cout << "Average generation speed : " << totalSpeed / count << " tokens/s\n";
    cout << "Generation Speed in min : " << (totalSpeed / count) * 60.0 << " tokens/min\n";

    if (cpuCount > 0) cout << "Average CPU usage: " << totalCpu / cpuCount << " %\n";
    if (gpuCount > 0) cout << "Average GPU usage: " << totalGpu / gpuCount << " %\n";
    if (ramCount > 0) cout << "Average peak RAM: " << totalPeakRam / ramCount << " MiB\n";
    if (vramCount > 0) cout << "Average peak VRAM: " << totalPeakVram / vramCount << " MiB\n";
    cout << "===================================\n";
}


int main(){

    //Step 1 : check for service
    if (!checkOllamaService()) {
        cerr << "Ollama service is not running." << endl;
        return 1;
    }

    cout << "Ollama service is ready." << endl;

    //Step 2 : Ask the user for the model name
    
    string modelName;
    cout << "Enter the model name (EG: qwen3:0.6b): ";

    

    if (!getline(cin, modelName) or modelName.empty()) {
        cerr << "Failed to read model name." << endl;
        return 1;
    }

    cout << "enter 1 for file test, 2 for single prompt test: ";
    int testMode;
    cin >> testMode;
    cin.ignore();  // ignore the newline character left in the input buffer
    if (testMode != 1 && testMode != 2) {
        cerr << "Invalid test mode." << endl;
        return 1;
    }

    if (testMode == 2) {
        string prompt;
        cout << "Enter the prompt: ";
        if (!getline(cin, prompt)) {
            cerr << "Failed to read prompt." << endl;
            return 1;
        }
        PromptPerformance performance = runPerformanceTest(modelName, prompt);
        performance.OutputResult();
        return 0;  // exit after single prompt test
    }
    //Step 3 :  Ask the user for the prompt  run the performance test()
    vector<string> prompts = loadPrompts("testfile.txt");

    if (prompts.empty()) {
        cerr << "No prompts loaded." << endl;
        return 1;
    }
    cout << "Loaded " << prompts.size() << " prompts." << endl;

    // Run the performance test


    cout << "\nWarming up model...\n";
    PromptPerformance warmup = runPerformanceTest(modelName, prompts[0]);

    if (!warmup.success) {
        cerr << "Warm-up failed. Stopping benchmark." << endl;
        return 1;
    }

    cout << "Warm-up completed.\n";
    //PromptPerformance performance = runPerformanceTest(modelName, prompt);

    //Step 4 : Display the results
    //performance.OutputResult();

    const int repeats = 3;  // repeat each prompt this many times

    vector<PromptPerformance> results;

    for (size_t i = 0; i < prompts.size(); i++) {

        for (int rep = 1; rep <= repeats; rep++) {

            cout << "\nTesting prompt " << i + 1 << ", repetition " << rep << endl;

            PromptPerformance result = runPerformanceTest(modelName, prompts[i]);

            if (result.success) {
                results.push_back(result);
            }
            else {
                cerr << "Test failed, result not recorded.\n";
            }
        }
    }      

    //output
    printAverageResults(results);

    cout << "Successful tests: " << results.size() << " / " << prompts.size() * repeats << endl;
    return 0;

}