#pragma once
#include <string>

namespace JackCompiler{
    /**
     * \brief Compiles .jack files containing Jack code into .vm files containing Hack virtual-machine
     * language code. If the input-path points to a single .jack files, then exactly one output .jack
     * file with the same name will be created in the input-file's directory. If the input-path
     * points to a directory this will be done for every .jack file contained in the directory.
     * \param inputPathName The path to a .jack file or the path to a directory containing .jack files
     * \return 0 if the compilation was successful, -1 otherwise 
     */
    int compile(const std::string& inputPathName);
}
