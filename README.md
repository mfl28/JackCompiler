# JackCompiler [![Build Status](https://travis-ci.org/mfl28/JackCompiler.svg?branch=master)](https://travis-ci.org/mfl28/JackCompiler) [![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/mfl28/JackCompiler.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/mfl28/JackCompiler/context:cpp)
This is a C++-implementation of a compiler for the Jack programming language defined in [project 9](https://www.nand2tetris.org/project9) of the nand2tetris-course.
The program takes as argument either a single `.jack`-file or a directory containing such files, compiles the file(s) into Hack virtual-machine language code and writes the result to one or more `.vm`-file(s).

## Building the program
_Note_: To build this program, a compiler that supports C++17 and `std::filesystem` is required (e.g. gcc version &geq; 8).

```bash
git clone https://github.com/mfl28/JackCompiler.git
cd JackCompiler
cmake -B build    # Use option "-D BUILD_TESTING=OFF" if you do not want to build the unit-tests.
cmake --build build   
```
## Running the program
After you built the program, do the following from within the `build`-directory:
#### Linux
```bash
./JackCompiler path/to/filename.jack    # Or "./JackCompiler path/to/directory"
```
#### Windows
```bash
cd Debug    # Or "cd Release" if you built using Release-configuration.
.\JackCompiler.exe path\to\filename.jack    # Or ".\JackCompiler path\to\directory"
```
## Running the tests
If you built the program including the unit-tests, then these can be run from within the `build`-directory by doing the following:
#### Linux
```bash
ctest -V
```
#### Windows
```bash
ctest -C Debug -V    # Or "ctest -C Release -V" if you built using Release-configuration.
```
## References
- [nand2tetris-course](https://www.nand2tetris.org)
- [Google Test](https://github.com/google/googletest)
