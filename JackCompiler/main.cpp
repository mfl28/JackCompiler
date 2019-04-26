#include "CompilationEngine.h"
#include <iostream>
#include <fstream>

using std::cout;

int main(int argc, char** argv)
{
    std::ifstream stream(argv[1]);

    JackCompiler::CompilationEngine engine(stream, std::cout);

    try
    {
        engine.compileClass();
    }
    catch(const std::runtime_error&  e)
    {
        std::cout << e.what();
        return -1;
    }
}
