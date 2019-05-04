#include "JackCompiler.h"
#include "CompilationEngine.h"
#include <filesystem>
#include <iostream>
#include <fstream>

using std::string;
using std::cout;
using std::ifstream;
using std::ofstream;
using std::runtime_error;

namespace fs = std::filesystem;

namespace JackCompiler {
    namespace {
        bool isValidPath(const fs::path& path) {
            bool returnValue = true;

            if(!fs::exists(path)) {
                cout << path << " does not exist.";
                returnValue = false;
            }

            if(!fs::is_directory(path) && path.extension() != ".jack") {
                cout << "Invalid argument: Must be either a path to a *.jack file or a path to a directory (containing *.jack files).";
                returnValue = false;
            }

            return returnValue;
        }
    }

    int compile(const string& inputPathName) {
        const fs::path inputPath(inputPathName);

        if(!isValidPath(inputPath)) {
            return -1;
        }

        if(fs::is_directory(inputPath)) {
            size_t validFiles = 0;

            for(const auto& item : fs::directory_iterator(inputPath)) {
                if(item.path().extension() == ".jack") {
                    if(ifstream inputFile(item.path()); inputFile) {
                        fs::path outputPath = item.path();
                        outputPath.replace_extension(".vm");

                        if(ofstream outputFile(outputPath); outputFile) {
                            CompilationEngine engine(inputFile, outputFile);
                            try {
                                engine.compileClass();
                            }
                            catch(const runtime_error& e) {
                                cout << "Compilation error in file " << item.path().filename() << ":\n\t" << e.what();
                                return -1;
                            }
                        }
                        else {
                            cout << "Could not create output file " << outputPath << ".";
                            return -1;
                        }
                    }
                    else {
                        cout << "Could not open file " << item.path().filename() << ".";
                        return -1;
                    }

                    ++validFiles;
                }
            }

            if(validFiles == 0) {
                cout << "The directory " << inputPath << " does not contain any *.jack files.";
                return -1;
            }
        }
        else if(ifstream inputFile(inputPath); inputFile) {
            fs::path outputPath = inputPath;
            outputPath.replace_extension(".vm");

            if(ofstream outputFile(outputPath); outputFile) {
                CompilationEngine engine(inputFile, outputFile);
                try {
                    engine.compileClass();
                }
                catch(const runtime_error& e) {
                    cout << "Compilation error:\n\t" << e.what();
                    return -1;
                }
            }
            else {
                cout << "Could not create output file " << outputPath << ".";
                return -1;
            }
        }
        else {
            cout << "Could not open file " << inputPath.filename() << ".";
            return -1;
        }

        return 0;
    }

}
