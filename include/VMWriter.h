#pragma once
#include <ostream>

namespace JackCompiler {
    class VMWriter {
    public:
        /**
         * \brief The different RAM-segments (pseudo segment in case of CONST)
         */
        enum class Segment { CONST, ARG, LOCAL, STATIC, THIS, THAT, POINTER, TEMP };

        /**
         * \brief The different arithmetic commands
         */
        enum class Command { ADD, SUB, NEG, EQ, GT, LT, AND, OR, NOT };

        /**
         * \brief Creates a new VMWriter object that provides functionality to write 
         * Hack virtual-machine language constructs to a provided output-stream.
         * \param outputStream 
         */
        explicit VMWriter(std::ostream& outputStream) : outputStream_{outputStream} {}

        /**
         * \brief Writes a push command to the output-stream.
         * \param segment The source RAM-segment (or the pseudo-segment CONST)
         * \param index The index in the RAM-segment (or the value to be pushed if segment is CONST)
         */
        void writePush(Segment segment, int index) const;

        /**
         * \brief Writes a pop command to the output-stream.
         * \param segment The target RAM-segment (must not be CONST)
         * \param index The index in the RAM-segment
         */
        void writePop(Segment segment, int index) const;

        /**
         * \brief Writes an arithmetic command to the output-stream.
         * \param command The type of command
         */
        void writeArithmetic(Command command) const;

        /**
         * \brief Writes a label to the output-stream.
         * \param label The name of the label
         */
        void writeLabel(const std::string& label) const;

        /**
         * \brief Writes a goto-statement to the output-stream.
         * \param label The target-label of the goto
         */
        void writeGoto(const std::string& label) const;

        /**
         * \brief Writes a goto-if-statement to the output-stream.
         * \param label The target of the goto-if
         */
        void writeIf(const std::string& label) const;

        /**
         * \brief Write a function-call-statement to the output-stream. 
         * \param name The name of the function
         * \param nArgs The number of arguments of the function
         */
        void writeCall(const std::string& name, int nArgs) const;

        /**
         * \brief Write a function-declaration-statement to the output-stream.
         * \param name The name of the function
         * \param nLocals The number of local variables of the function
         */
        void writeFunction(const std::string& name, int nLocals) const;

        /**
         * \brief Write a return-statement to the output-stream.
         */
        void writeReturn() const;

    private:
        std::ostream& outputStream_;
    };
}
