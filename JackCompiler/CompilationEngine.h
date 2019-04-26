#pragma once
#include "Tokenizer.h"
#include "SymbolTable.h"
#include <istream>

namespace JackCompiler {
    class CompilationEngine;
}

class JackCompiler::CompilationEngine {
public:
    CompilationEngine(std::istream& inputStream, std::ostream& outputStream) 
        : tokenizer_(inputStream), outputStream_(outputStream) {}

    void compileClass();

private:
    SymbolTable symbolTable_;
    Tokenizer tokenizer_;
    std::ostream& outputStream_;
    size_t currentIndentionLevel_ = 0;

    void openNonTerminalTag(const std::string& name);
    void closeNonTerminalTag(const std::string& name);

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
    void compileExpressionList();

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
    void parseIdentifier();
    void parseIdentifierAsVariableDefinition(SymbolTable::SymbolKind kind, const std::string& type);
    void parseIdentifierAsSubroutineDefinition();
    void parseIdentifierAsClassName();
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
};
