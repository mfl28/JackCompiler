#pragma once
#include "Tokenizer.h"
#include "SymbolTable.h"
#include "VMWriter.h"
#include <istream>

namespace JackCompiler {
    class CompilationEngine;
}

class JackCompiler::CompilationEngine {
public:
    /**
     * \brief Creates a new compilation engine that provides the functionality
     * to compile Jack code in a provided input-stream to Hack virtual-machine language
     * and write the result to a provided output-stream
     * \param inputStream 
     * \param outputStream 
     */
    CompilationEngine(std::istream& inputStream, std::ostream& outputStream) 
        : tokenizer_{inputStream}, vmWriter_{outputStream} {}

    /**
     * \brief Compiles a complete class.
     */
    void compileClass();

private:
    SymbolTable symbolTable_;
    Tokenizer tokenizer_;
    VMWriter vmWriter_;
    std::string className_;
    std::string currentSubroutineName_;
    Tokenizer::KeyWordType currentSubroutineType_{};
    size_t currentIfLabelIndex_{};
    size_t currentWhileLabelIndex_{};

    void compileClassVarDec();
    void compileSubroutineDec();
    void compileParameterList();
    void compileSubroutineBody();
    void compileVarDec();
    void compileStatements();
    void compileLet();
    void compileIf();
    void compileWhile();
    void compileDo();
    void compileReturn();
    void compileExpression();
    void compileTerm();
    int compileExpressionList();

    bool classVarDecEncountered() const;
    bool subroutineDecEncountered() const;
    bool typeEncountered() const;
    bool varDecEncountered() const;
    bool statementEncountered() const;
    bool termEncountered() const;

    void parseSymbol(char expectedSymbol) const;
    bool tryParseSymbol(char expectedSymbol) const;
    void parseKeyword(Tokenizer::KeyWordType expectedKeywordType) const;
    bool tryParseKeyword(Tokenizer::KeyWordType expectedKeywordType) const;
    void parseKeyword(std::initializer_list<Tokenizer::KeyWordType> validKeywordTypes) const;
    bool tryParseKeyword(std::initializer_list<Tokenizer::KeyWordType> validKeywordTypes) const;
    void parseIdentifier() const;
    bool tryParseIdentifier() const;
    void parseIdentifierAsVariableDefinition(SymbolTable::SymbolKind kind, const std::string& type);
    void parseIdentifierAsSubroutineDefinition();
    void parseIdentifierAsClassName();
    void parseIdentifierAsClassNameDefinition();
    bool tryParseIdentifierAsClassName();
    void parseIdentifierAsSubroutineName();
    void parseOpSymbol() const;
    bool tryParseOpSymbol() const;
    void parseUnaryOpSymbol() const;
    bool tryParseUnaryOpSymbol() const;
    bool tryParseIntConst() const;
    bool tryParseStringConst() const;
    void parseSubroutineReturnType();
    std::string parseVariableType();
    void processSubroutineCall();
    bool tryProcessAssignmentArrayElementAccess(const std::string& arrayVarName);
    void processExpressionArrayElementAccess(const std::string& arrayVarName);
    void processForeignMethodCall(const std::string& prefixName);
    void processFunctionCall(const std::string& prefixName);
    void processOwnMethodCall(const std::string& functionName);
};
