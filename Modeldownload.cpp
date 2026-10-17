#include<iostream>
#include<cstdlib>
#include<string>
#include <cctype>
using namespace std;
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

int main(){

    cout << "Checking Ollama installation..." << endl;

    int check = system("where.exe ollama > NUL");

    if (check == 0) {
        cout << "Ollama is installed!" << endl;
    }
    else {
        cout << "Ollama is not installed!" << endl;
        string choice;
        cout << "Do you want to download the Ollama? (Y/N)" << endl;
        getline(cin , choice);
        if (choice != "Y" && choice != "y"){
            cout << "Installation cancelled." << endl;
            return 0;
        }
        else{
            cout << "Start install ollama..." << endl;
            string installCommand = "powershell -NoProfile -Command "  "\"irm https://ollama.com/install.ps1 | iex\"";
            if (!commandrunning(installCommand)){
                cerr << "Installation failed." << endl;
                return 1;
            }
            cout << "Installed successfully." << endl;      
        }

    }

    cout << " Checking Ollama service... " <<endl;
    if(!commandrunning("curl.exe -fsS --max-time 3 -o NUL "
        "http://127.0.0.1:11434/api/tags 2>nul")){
        cout << "Starting Ollama..." << endl;
        if (!commandrunning("start \"\" ollama serve")) {
            cerr << "Cannot start Ollama!" << endl;
            return 1;
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
            return 1;
        }
    }

    string modelName;
    getline(cin, modelName);
    if (modelName.empty()){
        cerr << "Model name cannot be empty, please enter again." << endl;
        return 1;
    } 

    //need to check the modelName spell, make sure it is right.

    string command = "ollama pull " + modelName;
    cout << "Correct modelname, start loading" << endl;
    if (commandrunning(command) ){
        cout << "Download correctly!" << endl;
    }

    else{
        cerr << "Failed." << endl; 
    }
    string runCommand = "ollama run " +modelName;
    cout << "Starting model now..." << endl;
    if(!commandrunning(runCommand)){
        cerr << "Failed to run the model." << endl;
        return 1;
    }
    return 0;
}