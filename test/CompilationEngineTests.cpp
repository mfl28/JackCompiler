#include "CompilationEngine.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>

using std::vector;
using std::string;
using std::istream;
using std::ostream;
using std::ifstream;
using std::istreambuf_iterator;
using std::stringstream;
namespace fs = std::filesystem;

extern string testFilesPath;

namespace {
    const vector<string> TEST_FILE_NAMES{
        "AverageMain.jack",
        "ComplexArraysMain.jack",
        "ConvertToBinMain.jack",
        "PongBall.jack",
        "PongBat.jack",
        "PongGame.jack",
        "PongMain.jack",
        "SevenMain.jack",
        "Square.jack",
        "SquareGame.jack",
        "SquareMain.jack"
    };

    class CompilationEngineTest : public testing::TestWithParam<string> {};

    /**
     * \brief A parametrized test that gets an input-file <filename>.jack as a parameter, opens the file
     * using the testFilePath-string as stem-directory and compiles the file into
     * Hack VM-language code. The produced output is then compared with the code in the reference-file
     * <filename>_Ref.vm which is expected to exist in the directory denoted by testFilePath.
     * All reference files have been assembled using the provided JackCompiler.bat from nand2tetris.org.
     */
    TEST_P(CompilationEngineTest, CompilationOutputMatchesReference) {
        const fs::path inputPath{testFilesPath + GetParam()};

        ASSERT_TRUE(fs::exists(inputPath)) << "The test-file " << inputPath << " does not exist.";

        fs::path referenceOutputFilePath{inputPath};
        referenceOutputFilePath.replace_filename(inputPath.stem().string() + "_Ref.vm");

        ASSERT_TRUE(fs::exists(referenceOutputFilePath)) << "The required reference-file " << referenceOutputFilePath.filename() 
            << " does not exist in the test-file directory " << inputPath.parent_path();

        ifstream referenceOutputStream{referenceOutputFilePath};
        const string referenceOutput{istreambuf_iterator<char>{referenceOutputStream}, istreambuf_iterator<char>{}};

        stringstream outputStream;
        ifstream inputStream{inputPath};

        JackCompiler::CompilationEngine engine{inputStream, outputStream};
        engine.compileClass();

        ASSERT_EQ(referenceOutput, outputStream.str());
    }

    INSTANTIATE_TEST_CASE_P(CompilationEngineTestInstance, CompilationEngineTest, ::testing::ValuesIn(TEST_FILE_NAMES), 
        [] (const ::testing::TestParamInfo<string>& info) { return info.param.substr(0, info.param.find('.')); });
}