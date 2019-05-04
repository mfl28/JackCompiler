#include "CompilationEngine.h"
#include <iostream>
#include "JackCompiler.h"

using std::cout;
using std::flush;

int main(int argc, char** argv) {
    if(argc == 2) {
        try {
            return JackCompiler::compile(argv[1]);
        }
        catch(const std::exception& e) {
            std::cout << e.what() << flush;
            return -1;
        }
        
    }

    cout << "Wrong number of arguments: Usage: JackCompiler <filename.jack OR directoryName>" << flush;

    return -1;
}
