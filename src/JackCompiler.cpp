#include "JackCompiler.h"
#include "CompilationEngine.h"
#include <filesystem>
#include <iostream>
#include <fstream>

using std::string;
using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::runtime_error;

namespace fs = std::filesystem;

namespace JackCompiler {
    int compile(const string& inputPathName) {
        const fs::path inputPath{inputPathName};

        if(!fs::is_directory(inputPath) && inputPath.extension() != ".jack") {
            cout << "Invalid argument: Must be either a path to a *.jack file "
                    "or a path to a directory (containing *.jack files)." << endl;
            return -1;
        }

        if(fs::is_directory(inputPath)) {
            size_t validFiles{};

            for(const auto& item : fs::directory_iterator(inputPath)) {
                if(item.path().extension() == ".jack") {
                    if(ifstream inputFile(item.path()); inputFile) {
                        fs::path outputPath{item.path()};
                        outputPath.replace_extension(".vm");

                        if(ofstream outputFile{outputPath}) {
                            CompilationEngine engine{inputFile, outputFile};

                            try {
                                engine.compileClass();
                            }
                            catch(const runtime_error& e) {
                                cout << "Compilation error in file " << item.path().filename() 
                                     << ": " << e.what() << endl;
                                return -1;
                            }
                        }
                        else {
                            cout << "Could not create output file " << outputPath << "." << endl;
                            return -1;
                        }
                    }
                    else {
                        cout << "Could not open file " << item.path().filename() << "." << endl;
                        return -1;
                    }

                    ++validFiles;
                }
            }

            if(validFiles == 0) {
                cout << "The directory " << inputPath << " does not contain any *.jack files." << endl;
                return -1;
            }
        }
        else if(ifstream inputFile{inputPath}) {
            fs::path outputPath{inputPath};
            outputPath.replace_extension(".vm");

            if(ofstream outputFile{outputPath}) {
                CompilationEngine engine{inputFile, outputFile};

                try {
                    engine.compileClass();
                }
                catch(const runtime_error& e) {
                    cout << "Compilation error: " << e.what() << endl;
                    return -1;
                }
            }
            else {
                cout << "Could not create output file " << outputPath << '.' << endl;
                return -1;
            }
        }
        else {
            cout << "Could not open file " << inputPath.filename() << '.' << endl;
            return -1;
        }

        return 0;
    }

}
