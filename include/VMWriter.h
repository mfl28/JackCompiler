#pragma once
#include <ostream>

namespace JackCompiler {
    class VMWriter;
}

class JackCompiler::VMWriter {
public:
    enum class Segment {CONST, ARG, LOCAL, STATIC, THIS, THAT, POINTER, TEMP};
    enum class Command {ADD, SUB, NEG, EQ, GT, LT, AND, OR, NOT};

    explicit VMWriter(std::ostream& outputStream) : outputStream_(outputStream) {}

    void writePush(Segment segment, int index) const;
    void writePop(Segment segment, int index) const;
    void writeArithmetic(Command command) const;
    void writeLabel(const std::string& label) const;
    void writeGoto(const std::string& label) const;
    void writeIf(const std::string& label) const;
    void writeCall(const std::string& name, int nArgs) const;
    void writeFunction(const std::string& name, int nLocals) const;
    void writeReturn() const;
    
private:
    std::ostream& outputStream_;
};

