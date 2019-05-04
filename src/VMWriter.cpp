#include "VMWriter.h"
#include <unordered_map>
#include <string>

using std::unordered_map;
using std::string;

namespace JackCompiler {
    namespace {
        const unordered_map<VMWriter::Segment, string> SEGMENT_TO_NAME{
            { VMWriter::Segment::ARG,     "argument" },
            { VMWriter::Segment::CONST,   "constant" },
            { VMWriter::Segment::LOCAL,   "local" },
            { VMWriter::Segment::POINTER, "pointer" },
            { VMWriter::Segment::STATIC,  "static" },
            { VMWriter::Segment::TEMP,    "temp" },
            { VMWriter::Segment::THAT,    "that" },
            { VMWriter::Segment::THIS,    "this" }
        };

        const unordered_map<VMWriter::Command, string> COMMAND_TO_NAME{
            { VMWriter::Command::ADD, "add" },
            { VMWriter::Command::AND, "and" },
            { VMWriter::Command::EQ,  "eq" },
            { VMWriter::Command::GT,  "gt" },
            { VMWriter::Command::LT,  "lt" },
            { VMWriter::Command::NEG, "neg" },
            { VMWriter::Command::NOT, "not" },
            { VMWriter::Command::OR,  "or" },
            { VMWriter::Command::SUB, "sub" },
        };

    }

    void VMWriter::writePush(Segment segment, int index) const {
        outputStream_ << "push " << SEGMENT_TO_NAME.at(segment) << ' ' << index << '\n';
    }

    void VMWriter::writePop(Segment segment, int index) const {
        outputStream_ << "pop " << SEGMENT_TO_NAME.at(segment) << ' ' << index << '\n';
    }

    void VMWriter::writeArithmetic(Command command) const {
        outputStream_ << COMMAND_TO_NAME.at(command) << '\n';
    }

    void VMWriter::writeLabel(const string& label) const {
        outputStream_ << "label " << label << '\n';
    }

    void VMWriter::writeGoto(const string& label) const {
        outputStream_ << "goto " << label << '\n';
    }

    void VMWriter::writeIf(const string& label) const {
        outputStream_ << "if-goto " << label << '\n';
    }

    void VMWriter::writeCall(const string& name, int nArgs) const {
        outputStream_ << "call " << name << ' ' << nArgs <<  '\n';
    }

    void VMWriter::writeFunction(const string& name, int nLocals) const {
        outputStream_ << "function " << name << ' ' << nLocals << '\n';
    }

    void VMWriter::writeReturn() const {
        outputStream_ << "return\n";
    }
}
