#include "CompilationEngine.h"
#include <algorithm>
#include <array>
#include <sstream>

using std::runtime_error;
using std::string;
using std::to_string;
using std::initializer_list;
using std::array;
using std::unordered_map;
using std::stringstream;
using std::find;

namespace JackCompiler {
    namespace {
        const array<char, 9> OPS{'+', '-', '*', '/', '&', '|', '<', '>', '='};
        const array<char, 2> UNARY_OPS{'-', '~'};

        const array<Tokenizer::KeyWordType, 4> KEYWORD_CONSTANTS{
            Tokenizer::KeyWordType::TRUE,
            Tokenizer::KeyWordType::FALSE,
            Tokenizer::KeyWordType::NULL_,
            Tokenizer::KeyWordType::THIS
        };

        const array<Tokenizer::KeyWordType, 5> STATEMENT_KEYWORD_TYPES{
            Tokenizer::KeyWordType::LET,
            Tokenizer::KeyWordType::IF,
            Tokenizer::KeyWordType::WHILE,
            Tokenizer::KeyWordType::DO,
            Tokenizer::KeyWordType::RETURN
        };

        const unordered_map<Tokenizer::KeyWordType, string> KEYWORD_TYPE_TO_STRING{
            { Tokenizer::KeyWordType::CLASS,       "class" },
            { Tokenizer::KeyWordType::CONSTRUCTOR, "constructor" },
            { Tokenizer::KeyWordType::FUNCTION,    "function" },
            { Tokenizer::KeyWordType::METHOD,      "method" },
            { Tokenizer::KeyWordType::FIELD,       "field" },
            { Tokenizer::KeyWordType::STATIC,      "static" },
            { Tokenizer::KeyWordType::VAR,         "var" },
            { Tokenizer::KeyWordType::INT,         "int" },
            { Tokenizer::KeyWordType::CHAR,        "char" },
            { Tokenizer::KeyWordType::BOOLEAN,     "boolean" },
            { Tokenizer::KeyWordType::VOID,        "void" },
            { Tokenizer::KeyWordType::TRUE,        "true" },
            { Tokenizer::KeyWordType::FALSE,       "false" },
            { Tokenizer::KeyWordType::NULL_,       "null" },
            { Tokenizer::KeyWordType::THIS,        "this" },
            { Tokenizer::KeyWordType::LET,         "let" },
            { Tokenizer::KeyWordType::DO,          "do" },
            { Tokenizer::KeyWordType::IF,          "if" },
            { Tokenizer::KeyWordType::ELSE,        "else" },
            { Tokenizer::KeyWordType::WHILE,       "while" },
            { Tokenizer::KeyWordType::RETURN,      "return" }
        };

        const unordered_map <SymbolTable::SymbolKind, VMWriter::Segment> SYMBOL_KIND_TO_SEGMENT{
            { SymbolTable::SymbolKind::ARG,    VMWriter::Segment::ARG },
            { SymbolTable::SymbolKind::VAR,    VMWriter::Segment::LOCAL },
            { SymbolTable::SymbolKind::FIELD,  VMWriter::Segment::THIS },
            { SymbolTable::SymbolKind::STATIC, VMWriter::Segment::STATIC }
        };

        const unordered_map <char, VMWriter::Command> OP_SYMBOL_TO_COMMAND{
            { '+', VMWriter::Command::ADD },
            { '-', VMWriter::Command::SUB },
            { '&', VMWriter::Command::AND },
            { '|', VMWriter::Command::OR },
            { '<', VMWriter::Command::LT },
            { '>', VMWriter::Command::GT },
            { '=', VMWriter::Command::EQ },
        };

        const unordered_map<char, VMWriter::Command> UNARY_OP_SYMBOL_TO_COMMAND{
            { '-', VMWriter::Command::NEG },
            { '~', VMWriter::Command::NOT }
        };
    }

    void CompilationEngine::compileClass() {
        tokenizer_.advance();

        parseKeyword(Tokenizer::KeyWordType::CLASS);
        tokenizer_.advance();
        parseIdentifierAsClassNameDefinition();
        tokenizer_.advance();
        parseSymbol('{');
        tokenizer_.advance();

        while(classVarDecEncountered()) {
            compileClassVarDec();
        }

        while(subroutineDecEncountered()) {
            compileSubroutineDec();
        }

        parseSymbol('}');

        if(tokenizer_.hasMoreTokens()) {
            throw runtime_error("Illegal occurence of tokens after the end of a class definition.");
        }
    }

    void CompilationEngine::compileClassVarDec() {
        parseKeyword({Tokenizer::KeyWordType::STATIC, Tokenizer::KeyWordType::FIELD});

        const auto symbolKind = tokenizer_.keyWord() == Tokenizer::KeyWordType::STATIC ? SymbolTable::SymbolKind::STATIC : SymbolTable::SymbolKind::FIELD;
        tokenizer_.advance();

        const auto type = parseVariableType();
        tokenizer_.advance();
        parseIdentifierAsVariableDefinition(symbolKind, type);
        tokenizer_.advance();

        while(tryParseSymbol(',')) {
            tokenizer_.advance();
            parseIdentifierAsVariableDefinition(symbolKind, type);
            tokenizer_.advance();
        }

        parseSymbol(';');
        tokenizer_.advance();
    }

    void CompilationEngine::compileSubroutineDec() {
        symbolTable_.startSubroutine();
        currentIfLabelIndex_ = 0;
        currentWhileLabelIndex_ = 0;

        parseKeyword({Tokenizer::KeyWordType::CONSTRUCTOR, Tokenizer::KeyWordType::FUNCTION, Tokenizer::KeyWordType::METHOD});
        currentSubroutineType_ = tokenizer_.keyWord();
        tokenizer_.advance();
        parseSubroutineReturnType();

        tokenizer_.advance();
        parseIdentifierAsSubroutineDefinition();
        currentSubroutineName_ = tokenizer_.identifier();
        tokenizer_.advance();
        parseSymbol('(');
        tokenizer_.advance();

        compileParameterList();

        parseSymbol(')');

        tokenizer_.advance();

        compileSubroutineBody();
    }

    void CompilationEngine::compileParameterList() {
        if(currentSubroutineType_ == Tokenizer::KeyWordType::METHOD) {
            symbolTable_.define("this", className_, SymbolTable::SymbolKind::ARG);
        }

        if(typeEncountered()) {
            auto type = parseVariableType();
            tokenizer_.advance();
            parseIdentifierAsVariableDefinition(SymbolTable::SymbolKind::ARG, type);
            tokenizer_.advance();

            while(tryParseSymbol(',')) {
                tokenizer_.advance();
                type = parseVariableType();
                tokenizer_.advance();
                parseIdentifierAsVariableDefinition(SymbolTable::SymbolKind::ARG, type);
                tokenizer_.advance();
            }
        }
    }

    void CompilationEngine::compileSubroutineBody() {
        parseSymbol('{');
        tokenizer_.advance();

        while(varDecEncountered()) {
            compileVarDec();
        }

        // function init
        vmWriter_.writeFunction(className_ + "." + currentSubroutineName_, symbolTable_.varCount(SymbolTable::SymbolKind::VAR));

        if(currentSubroutineType_ == Tokenizer::KeyWordType::METHOD) {
            vmWriter_.writePush(VMWriter::Segment::ARG, 0);
            vmWriter_.writePop(VMWriter::Segment::POINTER, 0);

        }
        else if(currentSubroutineType_ == Tokenizer::KeyWordType::CONSTRUCTOR) {
            vmWriter_.writePush(VMWriter::Segment::CONST, symbolTable_.varCount(SymbolTable::SymbolKind::FIELD));
            vmWriter_.writeCall("Memory.alloc", 1);
            vmWriter_.writePop(VMWriter::Segment::POINTER, 0);
        }
        // end function init

        compileStatements();

        parseSymbol('}');
        tokenizer_.advance();
    }

    void CompilationEngine::compileVarDec() {
        parseKeyword(Tokenizer::KeyWordType::VAR);
        tokenizer_.advance();
        const auto type = parseVariableType();
        tokenizer_.advance();
        parseIdentifierAsVariableDefinition(SymbolTable::SymbolKind::VAR, type);
        tokenizer_.advance();

        while(tryParseSymbol(',')) {
            tokenizer_.advance();
            parseIdentifierAsVariableDefinition(SymbolTable::SymbolKind::VAR, type);
            tokenizer_.advance();
        }

        parseSymbol(';');
        tokenizer_.advance();
    }

    void CompilationEngine::compileStatements() {
        while(statementEncountered()) {
            switch(tokenizer_.keyWord()) {
                case Tokenizer::KeyWordType::LET:
                    compileLet();
                    break;
                case Tokenizer::KeyWordType::IF:
                    compileIf();
                    break;
                case Tokenizer::KeyWordType::WHILE:
                    compileWhile();
                    break;
                case Tokenizer::KeyWordType::DO:
                    compileDo();
                    break;
                case Tokenizer::KeyWordType::RETURN:
                    compileReturn();
                    break;
                default:
                    throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Invalid statement.");
            }
        }
    }

    void CompilationEngine::compileLet() {
        parseKeyword(Tokenizer::KeyWordType::LET);
        tokenizer_.advance();
        parseIdentifier();
        const auto identifier = tokenizer_.identifier();
        const auto segment = SYMBOL_KIND_TO_SEGMENT.at(symbolTable_.kindOf(identifier));
        const auto index = symbolTable_.indexOf(identifier);

        tokenizer_.advance();

        const auto assignmentToArrayElement = tryProcessAssignmentArrayElementAccess(identifier);

        parseSymbol('=');
        tokenizer_.advance();

        compileExpression();

        parseSymbol(';');

        if(assignmentToArrayElement) {
            vmWriter_.writePop(VMWriter::Segment::TEMP, 0);
            vmWriter_.writePop(VMWriter::Segment::POINTER, 1);
            vmWriter_.writePush(VMWriter::Segment::TEMP, 0);
            vmWriter_.writePop(VMWriter::Segment::THAT, 0);
        }
        else {
            vmWriter_.writePop(segment, index);
        }

        tokenizer_.advance();
    }

    void CompilationEngine::compileIf() {
        parseKeyword(Tokenizer::KeyWordType::IF);
        tokenizer_.advance();
        parseSymbol('(');
        tokenizer_.advance();

        compileExpression();

        parseSymbol(')');

        const auto ifLabelIndex = currentIfLabelIndex_++;
        const auto ifTrueLabel = "IF_TRUE" + to_string(ifLabelIndex);
        const auto ifFalseLabel = "IF_FALSE" + to_string(ifLabelIndex);

        vmWriter_.writeIf(ifTrueLabel);
        vmWriter_.writeGoto(ifFalseLabel);
        vmWriter_.writeLabel(ifTrueLabel);

        tokenizer_.advance();
        parseSymbol('{');
        tokenizer_.advance();

        compileStatements();

        parseSymbol('}');
        tokenizer_.advance();

        if(tryParseKeyword(Tokenizer::KeyWordType::ELSE)) {
            const auto ifEndLabel = "IF_END" + to_string(ifLabelIndex);
            vmWriter_.writeGoto(ifEndLabel);
            vmWriter_.writeLabel(ifFalseLabel);

            tokenizer_.advance();
            parseSymbol('{');
            tokenizer_.advance();

            compileStatements();

            parseSymbol('}');
            tokenizer_.advance();
            vmWriter_.writeLabel(ifEndLabel);
        }
        else {
            vmWriter_.writeLabel(ifFalseLabel);
        }
    }

    void CompilationEngine::compileWhile() {
        parseKeyword(Tokenizer::KeyWordType::WHILE);

        const auto whileLabelIndex = currentWhileLabelIndex_++;
        const auto whileConditionLabel = "WHILE_EXP" + to_string(whileLabelIndex);
        vmWriter_.writeLabel(whileConditionLabel);

        tokenizer_.advance();
        parseSymbol('(');
        tokenizer_.advance();

        compileExpression();

        parseSymbol(')');

        vmWriter_.writeArithmetic(VMWriter::Command::NOT);

        const auto whileEndLabel = "WHILE_END" + to_string(whileLabelIndex);
        vmWriter_.writeIf(whileEndLabel);

        tokenizer_.advance();
        parseSymbol('{');
        tokenizer_.advance();

        compileStatements();

        vmWriter_.writeGoto(whileConditionLabel);

        parseSymbol('}');

        vmWriter_.writeLabel(whileEndLabel);

        tokenizer_.advance();
    }

    void CompilationEngine::compileDo() {
        parseKeyword(Tokenizer::KeyWordType::DO);
        tokenizer_.advance();
        // Subroutine call

        processSubroutineCall();
        // End subroutine call
        parseSymbol(';');
        // only functions/methods with void return type can be called in a do statement,
        // but every functions has to return a value, so to ignore the dummy value we
        // pop it from the stack
        vmWriter_.writePop(VMWriter::Segment::TEMP, 0);

        tokenizer_.advance();
    }

    void CompilationEngine::compileReturn() {
        parseKeyword(Tokenizer::KeyWordType::RETURN);
        tokenizer_.advance();

        if(!tryParseSymbol(';')) {
            compileExpression();
            parseSymbol(';');
        }
        else {
            vmWriter_.writePush(VMWriter::Segment::CONST, 0);
        }

        vmWriter_.writeReturn();

        tokenizer_.advance();
    }

    void CompilationEngine::compileExpression() {
        compileTerm();

        while(tryParseOpSymbol()) {
            const auto opSymbol = tokenizer_.symbol();
            tokenizer_.advance();

            compileTerm();

            switch(opSymbol) {
                case '*':
                    vmWriter_.writeCall("Math.multiply", 2);
                    break;
                case '/':
                    vmWriter_.writeCall("Math.divide", 2);
                    break;
                default:
                    vmWriter_.writeArithmetic(OP_SYMBOL_TO_COMMAND.at(opSymbol));
            }
        }
    }

    void CompilationEngine::compileTerm() {
        if(tryParseSymbol('(')) {
            // (expression)
            tokenizer_.advance();

            compileExpression();

            parseSymbol(')');
            tokenizer_.advance();
        }
        else if(tryParseUnaryOpSymbol()) {
            // unaryOp term
            const auto symbol = tokenizer_.symbol();
            tokenizer_.advance();

            compileTerm();

            vmWriter_.writeArithmetic(UNARY_OP_SYMBOL_TO_COMMAND.at(symbol));
        }
        else if(tryParseIntConst()) {
            vmWriter_.writePush(VMWriter::Segment::CONST, tokenizer_.intVal());

            tokenizer_.advance();
        }
        else if(tryParseStringConst()) {
            const auto stringValue = tokenizer_.stringVal();

            vmWriter_.writePush(VMWriter::Segment::CONST, stringValue.size());
            vmWriter_.writeCall("String.new", 1);

            for(auto c : stringValue) {
                vmWriter_.writePush(VMWriter::Segment::CONST, c);
                vmWriter_.writeCall("String.appendChar", 2);
            }

            tokenizer_.advance();
        }
        else if(tryParseKeyword({Tokenizer::KeyWordType::TRUE, Tokenizer::KeyWordType::FALSE, Tokenizer::KeyWordType::NULL_, Tokenizer::KeyWordType::THIS})) {
            // keyWord OR integerConstant OR stringConstant
            const auto keyword = tokenizer_.keyWord();

            if(keyword == Tokenizer::KeyWordType::TRUE) {
                vmWriter_.writePush(VMWriter::Segment::CONST, 0);
                vmWriter_.writeArithmetic(VMWriter::Command::NOT);
            }
            else if(keyword == Tokenizer::KeyWordType::FALSE || keyword == Tokenizer::KeyWordType::NULL_) {
                vmWriter_.writePush(VMWriter::Segment::CONST, 0);
            }
            else {
                vmWriter_.writePush(VMWriter::Segment::POINTER, 0);
            }

            tokenizer_.advance();
        }
        else if(tryParseIdentifier()) {
            const auto identifier = tokenizer_.identifier();
            tokenizer_.advance();

            if(const auto kind = symbolTable_.kindOf(identifier); kind != SymbolTable::SymbolKind::NONE) {
                // varName OR varName[expression] OR varName.methodName(expressionList)
                if(tryParseSymbol('[')) {
                    // [expression]
                    tokenizer_.advance();
                    processExpressionArrayElementAccess(identifier);
                }
                else if(tryParseSymbol('.')) {
                    // .methodName(expressionList)
                    tokenizer_.advance();
                    processForeignMethodCall(identifier);
                }
                else {
                    // >empty<
                    vmWriter_.writePush(SYMBOL_KIND_TO_SEGMENT.at(kind), symbolTable_.indexOf(identifier));
                }
            }
            else {
                // methodName(expressionList) OR className.functionName(expressionList)
                if(tryParseSymbol('.')) {
                    // .functionName(expressionList)
                    tokenizer_.advance();
                    processFunctionCall(identifier);
                }
                else {
                    // methodName(expressionList)
                    processOwnMethodCall(identifier);
                }
            }
        }
        else {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Invalid term-construct.");
        }
    }

    int CompilationEngine::compileExpressionList() {
        auto nrExpressions = 0;

        if(termEncountered()) {
            compileExpression();
            ++nrExpressions;

            while(tryParseSymbol(',')) {
                tokenizer_.advance();
                compileExpression();
                ++nrExpressions;
            }
        }

        return nrExpressions;
    }

    void CompilationEngine::processSubroutineCall() {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::IDENTIFIER) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Invalid subroutine-call.");
        }

        const auto identifier = tokenizer_.identifier();
        tokenizer_.advance();

        if(symbolTable_.kindOf(identifier) == SymbolTable::SymbolKind::NONE) {
            // The definition of the Jack-language implies that if in an error-free program, an identifier is not of type STATIC, FIELD, ARG or VAR
            // then it must be either a subroutine-name or a class-name
            // methodName(expressionList) OR className.functionName(expressionList)
            if(tryParseSymbol('.')) {
                // .functionName(expressionList)
                tokenizer_.advance();
                processFunctionCall(identifier);
            }
            else {
                // methodName(expressionList)
                processOwnMethodCall(identifier);
            }
        }
        else {
            // .methodName(expressionList)
            if(tryParseSymbol('.')) {
                
                tokenizer_.advance();
                processForeignMethodCall(identifier);
            }
            else {
                throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Invalid subroutine-call.");
            }
        }
    }

    bool CompilationEngine::tryProcessAssignmentArrayElementAccess(const std::string& arrayVarName) {
        if(tryParseSymbol('[')) {
            tokenizer_.advance();
            compileExpression();

            parseSymbol(']');

            vmWriter_.writePush(SYMBOL_KIND_TO_SEGMENT.at(symbolTable_.kindOf(arrayVarName)), symbolTable_.indexOf(arrayVarName));
            vmWriter_.writeArithmetic(VMWriter::Command::ADD);
            tokenizer_.advance();
            return true;
        }

        return false;
    }

    void CompilationEngine::processExpressionArrayElementAccess(const std::string& arrayVarName) {
        compileExpression();

        parseSymbol(']');

        vmWriter_.writePush(SYMBOL_KIND_TO_SEGMENT.at(symbolTable_.kindOf(arrayVarName)), symbolTable_.indexOf(arrayVarName));
        vmWriter_.writeArithmetic(VMWriter::Command::ADD);
        vmWriter_.writePop(VMWriter::Segment::POINTER, 1);
        vmWriter_.writePush(VMWriter::Segment::THAT, 0);

        tokenizer_.advance();
    }


    void CompilationEngine::processForeignMethodCall(const std::string& prefixName) {
        parseIdentifierAsSubroutineName();
        const auto calledSubroutineName = tokenizer_.identifier();
        tokenizer_.advance();

        vmWriter_.writePush(SYMBOL_KIND_TO_SEGMENT.at(symbolTable_.kindOf(prefixName)), symbolTable_.indexOf(prefixName));

        parseSymbol('(');
        tokenizer_.advance();

        const auto nrArgs = compileExpressionList();

        parseSymbol(')');

        vmWriter_.writeCall(symbolTable_.typeOf(prefixName) + "." + calledSubroutineName, nrArgs + 1);
 
        tokenizer_.advance();
    }

    void CompilationEngine::processFunctionCall(const std::string& prefixName) {
        parseIdentifierAsSubroutineName();
        const auto functionName = tokenizer_.identifier();
        tokenizer_.advance();
        parseSymbol('(');
        tokenizer_.advance();

        const auto nrArgs = compileExpressionList();

        parseSymbol(')');

        vmWriter_.writeCall(prefixName + "." + functionName, nrArgs);
        tokenizer_.advance();
    }


    void CompilationEngine::processOwnMethodCall(const string& functionName) {
        vmWriter_.writePush(VMWriter::Segment::POINTER, 0);

        parseSymbol('(');
        tokenizer_.advance();

        const auto nrArgs = compileExpressionList();

        parseSymbol(')');

        vmWriter_.writeCall(className_ + "." + functionName, nrArgs + 1);

        tokenizer_.advance();
    }




    bool CompilationEngine::classVarDecEncountered() const {
        return tokenizer_.tokenType() == Tokenizer::TokenType::KEYWORD &&
            (tokenizer_.keyWord() == Tokenizer::KeyWordType::STATIC ||
                tokenizer_.keyWord() == Tokenizer::KeyWordType::FIELD);
    }


    bool CompilationEngine::subroutineDecEncountered() const {
        return tokenizer_.tokenType() == Tokenizer::TokenType::KEYWORD &&
            (tokenizer_.keyWord() == Tokenizer::KeyWordType::CONSTRUCTOR ||
                tokenizer_.keyWord() == Tokenizer::KeyWordType::FUNCTION ||
                tokenizer_.keyWord() == Tokenizer::KeyWordType::METHOD);
    }

    bool CompilationEngine::typeEncountered() const {
        return tokenizer_.tokenType() == Tokenizer::TokenType::IDENTIFIER ||
            (tokenizer_.tokenType() == Tokenizer::TokenType::KEYWORD &&
            (tokenizer_.keyWord() == Tokenizer::KeyWordType::INT ||
                tokenizer_.keyWord() == Tokenizer::KeyWordType::CHAR ||
                tokenizer_.keyWord() == Tokenizer::KeyWordType::BOOLEAN));
    }

    bool CompilationEngine::varDecEncountered() const {
        return tokenizer_.tokenType() == Tokenizer::TokenType::KEYWORD &&
            tokenizer_.keyWord() == Tokenizer::KeyWordType::VAR;
    }

    bool CompilationEngine::statementEncountered() const {
        return tokenizer_.tokenType() == Tokenizer::TokenType::KEYWORD &&
            std::find(STATEMENT_KEYWORD_TYPES.cbegin(), STATEMENT_KEYWORD_TYPES.cend(),
                tokenizer_.keyWord()) != STATEMENT_KEYWORD_TYPES.cend();
    }

    bool CompilationEngine::termEncountered() const {
        switch(tokenizer_.tokenType()) {
            case Tokenizer::TokenType::INT_CONST:
                return true;
            case Tokenizer::TokenType::STRING_CONST:
                return true;
            case Tokenizer::TokenType::KEYWORD:
                return std::find(KEYWORD_CONSTANTS.cbegin(), KEYWORD_CONSTANTS.cend(),
                    tokenizer_.keyWord()) != KEYWORD_CONSTANTS.cend();
            case Tokenizer::TokenType::SYMBOL:
                return tokenizer_.symbol() == '(' || std::find(UNARY_OPS.cbegin(), UNARY_OPS.cend(),
                    tokenizer_.symbol()) != UNARY_OPS.cend();
            case Tokenizer::TokenType::IDENTIFIER:
                return true;
        }

        return false;
    }

    void CompilationEngine::parseSymbol(char expectedSymbol) const {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::SYMBOL) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected a symbol-token.");
        }

        if(const auto symbol = tokenizer_.symbol();  symbol != expectedSymbol) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected symbol \"" +
                to_string(expectedSymbol) + "\" but got \"" + to_string(symbol) + "\".");
        }
    }

    bool CompilationEngine::tryParseSymbol(char expectedSymbol) const {
        return tokenizer_.tokenType() == Tokenizer::TokenType::SYMBOL && tokenizer_.symbol() == expectedSymbol;
    }

    void CompilationEngine::parseOpSymbol() const {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::SYMBOL) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected a symbol-token.");
        }

        if(find(OPS.cbegin(), OPS.cend(), tokenizer_.symbol()) == OPS.cend()) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected operation symbol.");
        }
    }

    bool CompilationEngine::tryParseOpSymbol() const {
        return tokenizer_.tokenType() == Tokenizer::TokenType::SYMBOL && find(
            OPS.cbegin(), OPS.cend(), tokenizer_.symbol()) != OPS.cend();
    }

    void CompilationEngine::parseUnaryOpSymbol() const {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::SYMBOL) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected a symbol-token.");
        }

        if(find(UNARY_OPS.cbegin(), UNARY_OPS.cend(), tokenizer_.symbol()) == UNARY_OPS.cend()) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected unary-operation symbol.");
        }
    }

    bool CompilationEngine::tryParseUnaryOpSymbol() const {
        return tokenizer_.tokenType() == Tokenizer::TokenType::SYMBOL && find(
            UNARY_OPS.cbegin(), UNARY_OPS.cend(), tokenizer_.symbol()) != UNARY_OPS.cend();
    }

    void CompilationEngine::parseKeyword(Tokenizer::KeyWordType expectedKeywordType) const {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::KEYWORD) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected a keyword-token.");
        }

        if(const auto keyword = tokenizer_.keyWord(); keyword != expectedKeywordType) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected keyword \"" +
                KEYWORD_TYPE_TO_STRING.at(expectedKeywordType) + "\" but got \"" + KEYWORD_TYPE_TO_STRING.at(keyword) + "\".");
        }
    }

    bool CompilationEngine::tryParseKeyword(Tokenizer::KeyWordType expectedKeywordType) const {
        return tokenizer_.tokenType() == Tokenizer::TokenType::KEYWORD && tokenizer_.keyWord() == expectedKeywordType;
    }

    void CompilationEngine::parseKeyword(initializer_list<Tokenizer::KeyWordType> validKeywordTypes) const {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::KEYWORD) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected a keyword-token.");
        }

        if(const auto keyword = tokenizer_.keyWord(); find(validKeywordTypes.begin(), validKeywordTypes.end(), keyword) == validKeywordTypes.end()) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Invalid keyword \"" + KEYWORD_TYPE_TO_STRING.at(keyword) + "\".");
        }
    }

    bool CompilationEngine::tryParseIdentifier() const {
        return tokenizer_.tokenType() == Tokenizer::TokenType::IDENTIFIER;
    }


    bool CompilationEngine::tryParseKeyword(initializer_list<Tokenizer::KeyWordType> validKeywordTypes) const {
        return tokenizer_.tokenType() == Tokenizer::TokenType::KEYWORD && find(
            validKeywordTypes.begin(), validKeywordTypes.end(), tokenizer_.keyWord()) != validKeywordTypes.end();
    }

    void CompilationEngine::parseIdentifierAsVariableDefinition(SymbolTable::SymbolKind kind, const std::string& type) {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::IDENTIFIER) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected an identifier-token.");
        }

        const auto identifier = tokenizer_.identifier();

        if(const auto symbolKind = symbolTable_.kindOf(identifier); symbolKind != SymbolTable::SymbolKind::NONE && symbolKind == kind) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Redefinition of identifier in same scope.");
        }

        symbolTable_.define(identifier, type, kind);
    }

    void CompilationEngine::parseIdentifierAsSubroutineDefinition() {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::IDENTIFIER) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected an identifier-token.");
        }

        if(symbolTable_.kindOf(tokenizer_.identifier()) != SymbolTable::SymbolKind::NONE) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Invalid definition of a subroutine with the same name as a static/field variable.");
        }
    }

    void CompilationEngine::parseIdentifier() const {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::IDENTIFIER) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected an identifier-token.");
        }
    }

    void CompilationEngine::parseIdentifierAsClassName() {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::IDENTIFIER) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected an identifier-token.");
        }

        if(symbolTable_.kindOf(tokenizer_.identifier()) != SymbolTable::SymbolKind::NONE) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected a class-name.");
        }
    }

    bool CompilationEngine::tryParseIdentifierAsClassName() {
        return tokenizer_.tokenType() == Tokenizer::TokenType::IDENTIFIER &&
            symbolTable_.kindOf(tokenizer_.identifier()) == SymbolTable::SymbolKind::NONE;
    }

    void CompilationEngine::parseIdentifierAsClassNameDefinition() {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::IDENTIFIER ||
            symbolTable_.kindOf(tokenizer_.identifier()) != SymbolTable::SymbolKind::NONE) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Invalid class definition.");
        }

        className_ = tokenizer_.identifier();
    }

    void CompilationEngine::parseIdentifierAsSubroutineName() {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::IDENTIFIER) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected an identifier-token.");
        }

        if(symbolTable_.kindOf(tokenizer_.identifier()) != SymbolTable::SymbolKind::NONE) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected an subroutine-name.");
        }
    }

    bool CompilationEngine::tryParseIntConst() const {
        return tokenizer_.tokenType() == Tokenizer::TokenType::INT_CONST;
    }

    bool CompilationEngine::tryParseStringConst() const {
        return tokenizer_.tokenType() == Tokenizer::TokenType::STRING_CONST;
    }

    std::string CompilationEngine::parseVariableType() {
        if(tryParseKeyword({Tokenizer::KeyWordType::INT, Tokenizer::KeyWordType::CHAR, Tokenizer::KeyWordType::BOOLEAN})) {
            return KEYWORD_TYPE_TO_STRING.at(tokenizer_.keyWord());
        }

        if(tryParseIdentifierAsClassName()) {
            return tokenizer_.identifier();
        }

        throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Invalid type.");
    }

    void CompilationEngine::parseSubroutineReturnType() {
        if(!tryParseKeyword({Tokenizer::KeyWordType::VOID, Tokenizer::KeyWordType::INT, Tokenizer::KeyWordType::CHAR, Tokenizer::KeyWordType::BOOLEAN}) &&
            !tryParseIdentifierAsClassName()) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Invalid subroutine return-type.");
        }
    }
}
