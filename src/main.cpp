#include "JackCompiler.h"
#include <iostream>

using std::cout;
using std::endl;

int main(int argc, char** argv) {
    if(argc == 2) {
        return JackCompiler::compile(argv[1]);
    }

    cout << "Wrong number of arguments: Usage: JackCompiler <<filename>.jack OR <directoryName>>" << endl;
    return -1;
}
