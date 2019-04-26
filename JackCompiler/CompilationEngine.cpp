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
using std::pair;
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

        const unordered_map<Tokenizer::TokenType, string> TOKEN_TYPE_TO_TAG_NAME{
            { Tokenizer::TokenType::IDENTIFIER,   "identifier" },
            { Tokenizer::TokenType::INT_CONST,    "integerConstant" },
            { Tokenizer::TokenType::KEYWORD,      "keyword" },
            { Tokenizer::TokenType::STRING_CONST, "stringConstant" },
            { Tokenizer::TokenType::SYMBOL,       "symbol" }
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

        const unordered_map<SymbolTable::SymbolKind, string> SYMBOL_KIND_TO_ATTRIBUTE_NAME{
            { SymbolTable::SymbolKind::ARG,    "argument"},
            { SymbolTable::SymbolKind::VAR,    "var"},
            { SymbolTable::SymbolKind::FIELD,  "field"},
            { SymbolTable::SymbolKind::STATIC, "static"}
        };


        template<typename T>
        string xmlElement(Tokenizer::TokenType type, const T& value, initializer_list<pair<string, string>> attributes = {}) {
            stringstream s;
            const auto& tag = TOKEN_TYPE_TO_TAG_NAME.at(type);
            s << "<" << tag;

            for(const auto& p : attributes) {
                s << " " << p.first << "=" << p.second;
            }

            s << "> " << value << " </" << tag << ">";
            return s.str();
        }

        string symbolToXmlValue(char c) {
            switch(c) {
                case '<': return "&lt;";
                case '>': return "&gt;";
                case '&': return "&amp;";
                default:  return string{c};
            }
        }
    }

    void CompilationEngine::compileClass() {
        tokenizer_.advance();

        openNonTerminalTag("class");

        parseKeyword(Tokenizer::KeyWordType::CLASS);
        tokenizer_.advance();
        parseIdentifierAsClassName();
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

        closeNonTerminalTag("class");

        if(tokenizer_.hasMoreTokens()) {
            throw runtime_error("Illegal occurence of tokens after a class definition.");
        }
    }

    void CompilationEngine::compileClassVarDec() {
        openNonTerminalTag("classVarDec");

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

        closeNonTerminalTag("classVarDec");
    }

    void CompilationEngine::compileSubroutineDec() {
        symbolTable_.startSubroutine();

        openNonTerminalTag("subroutineDec");

        parseKeyword({Tokenizer::KeyWordType::CONSTRUCTOR, Tokenizer::KeyWordType::FUNCTION, Tokenizer::KeyWordType::METHOD});
        tokenizer_.advance();
        parseSubroutineReturnType();
        tokenizer_.advance();
        parseIdentifierAsSubroutineDefinition();
        tokenizer_.advance();
        parseSymbol('(');
        tokenizer_.advance();

        compileParameterList();

        parseSymbol(')');
        tokenizer_.advance();

        compileSubroutineBody();

        closeNonTerminalTag("subroutineDec");
    }

    void CompilationEngine::compileParameterList() {
        openNonTerminalTag("parameterList");

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

        closeNonTerminalTag("parameterList");
    }

    void CompilationEngine::compileSubroutineBody() {
        openNonTerminalTag("subroutineBody");

        parseSymbol('{');
        tokenizer_.advance();

        while(varDecEncountered()) {
            compileVarDec();
        }

        compileStatements();

        parseSymbol('}');
        tokenizer_.advance();

        closeNonTerminalTag("subroutineBody");
    }

    void CompilationEngine::compileVarDec() {
        openNonTerminalTag("varDec");

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

        closeNonTerminalTag("varDec");
    }

    void CompilationEngine::compileStatements() {
        openNonTerminalTag("statements");

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

        closeNonTerminalTag("statements");
    }

    void CompilationEngine::compileLet() {
        openNonTerminalTag("letStatement");

        parseKeyword(Tokenizer::KeyWordType::LET);
        tokenizer_.advance();
        parseIdentifier();
        tokenizer_.advance();

        if(tryParseSymbol('[')) {
            tokenizer_.advance();

            compileExpression();

            parseSymbol(']');
            tokenizer_.advance();
        }

        parseSymbol('=');
        tokenizer_.advance();

        compileExpression();

        parseSymbol(';');
        tokenizer_.advance();

        closeNonTerminalTag("letStatement");
    }

    void CompilationEngine::compileIf() {
        openNonTerminalTag("ifStatement");

        parseKeyword(Tokenizer::KeyWordType::IF);
        tokenizer_.advance();
        parseSymbol('(');
        tokenizer_.advance();

        compileExpression();

        parseSymbol(')');
        tokenizer_.advance();
        parseSymbol('{');
        tokenizer_.advance();

        compileStatements();

        parseSymbol('}');
        tokenizer_.advance();

        if(tryParseKeyword(Tokenizer::KeyWordType::ELSE)) {
            tokenizer_.advance();
            parseSymbol('{');
            tokenizer_.advance();

            compileStatements();

            parseSymbol('}');
            tokenizer_.advance();
        }

        closeNonTerminalTag("ifStatement");
    }

    void CompilationEngine::compileWhile() {
        openNonTerminalTag("whileStatement");

        parseKeyword(Tokenizer::KeyWordType::WHILE);
        tokenizer_.advance();
        parseSymbol('(');
        tokenizer_.advance();

        compileExpression();

        parseSymbol(')');
        tokenizer_.advance();
        parseSymbol('{');
        tokenizer_.advance();

        compileStatements();

        parseSymbol('}');
        tokenizer_.advance();

        closeNonTerminalTag("whileStatement");
    }

    void CompilationEngine::compileDo() {
        openNonTerminalTag("doStatement");

        parseKeyword(Tokenizer::KeyWordType::DO);
        tokenizer_.advance();
        // Subroutine call

        processSubroutineCall();
        // End subroutine call
        parseSymbol(';');
        tokenizer_.advance();

        closeNonTerminalTag("doStatement");
    }

    void CompilationEngine::compileReturn() {
        openNonTerminalTag("returnStatement");

        parseKeyword(Tokenizer::KeyWordType::RETURN);
        tokenizer_.advance();

        if(!tryParseSymbol(';')) {
            compileExpression();
            parseSymbol(';');
        }

        tokenizer_.advance();

        closeNonTerminalTag("returnStatement");
    }

    void CompilationEngine::compileExpression() {
        openNonTerminalTag("expression");

        compileTerm();

        while(tryParseOpSymbol()) {
            tokenizer_.advance();

            compileTerm();
        }

        closeNonTerminalTag("expression");
    }

    void CompilationEngine::compileTerm() {
        openNonTerminalTag("term");

        if(tryParseSymbol('(')) {
            // (expression)
            tokenizer_.advance();

            compileExpression();

            parseSymbol(')');
            tokenizer_.advance();
        }
        else if(tryParseUnaryOpSymbol()) {
            // unaryOp term
            tokenizer_.advance();

            compileTerm();
        }
        else if(tokenizer_.tokenType() == Tokenizer::TokenType::IDENTIFIER) {
            if(symbolTable_.kindOf(tokenizer_.identifier()) != SymbolTable::SymbolKind::NONE) {
                // varName OR varName[expression] OR varName.subroutineName(expressionList)
                parseIdentifier();
                tokenizer_.advance();

                if(tryParseSymbol('[')) {
                    tokenizer_.advance();

                    compileExpression();

                    parseSymbol(']');
                    tokenizer_.advance();
                }
                else if(tryParseSymbol('.')) {
                    tokenizer_.advance();
                    parseIdentifierAsSubroutineName();
                    tokenizer_.advance();

                    parseSymbol('(');
                    tokenizer_.advance();

                    compileExpressionList();

                    parseSymbol(')');
                    tokenizer_.advance();
                }
            }
            else {
                // subroutineName(expressionList) OR className.subroutineName(expressionList)
                const auto identifier = tokenizer_.identifier();
                tokenizer_.advance();

                if(tokenizer_.tokenType() == Tokenizer::TokenType::SYMBOL && tokenizer_.symbol() == '.') {
                    outputStream_ << string(2 * currentIndentionLevel_, ' ')
                        << xmlElement(Tokenizer::TokenType::IDENTIFIER, identifier, {{ "category", "class" }}) << "\n";
                    parseSymbol('.');
                    tokenizer_.advance();
                    parseIdentifierAsSubroutineName();
                    tokenizer_.advance();
                }
                else {
                    outputStream_ << string(2 * currentIndentionLevel_, ' ')
                        << xmlElement(Tokenizer::TokenType::IDENTIFIER, identifier, {{ "category", "subroutine" }}) << "\n";
                }

                parseSymbol('(');
                tokenizer_.advance();

                compileExpressionList();

                parseSymbol(')');
                tokenizer_.advance();
            }
        }
        else {
            // keyWord OR integerConstant OR stringConstant

            if(!tryParseKeyword({Tokenizer::KeyWordType::TRUE, Tokenizer::KeyWordType::FALSE, Tokenizer::KeyWordType::NULL_, Tokenizer::KeyWordType::THIS}) && 
                !tryParseIntConst() && 
                !tryParseStringConst()) {
                throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Invalid term-construct.");
            }

            tokenizer_.advance();
        }

        closeNonTerminalTag("term");
    }

    void CompilationEngine::compileExpressionList() {
        openNonTerminalTag("expressionList");

        if(termEncountered()) {
            compileExpression();

            while(tryParseSymbol(',')) {
                tokenizer_.advance();
                compileExpression();
            }
        }

        closeNonTerminalTag("expressionList");
    }

    void CompilationEngine::processSubroutineCall() {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::IDENTIFIER) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Invalid subroutine-call.");
        }


        if(symbolTable_.kindOf(tokenizer_.identifier()) == SymbolTable::SymbolKind::NONE) {
            // The definition of the Jack-language implies that if in an error-free program, an identifier is not of type STATIC, FIELD, ARG or VAR
            // then it must be either a subroutine-name or a class-name
            const auto identifier = tokenizer_.identifier();
            tokenizer_.advance();

            if(tokenizer_.tokenType() == Tokenizer::TokenType::SYMBOL && tokenizer_.symbol() == '.') {
                outputStream_ << string(2 * currentIndentionLevel_, ' ')
                    << xmlElement(Tokenizer::TokenType::IDENTIFIER, identifier, {{ "category", "class" }}) << "\n";
                parseSymbol('.');
                tokenizer_.advance();
                parseIdentifierAsSubroutineName();
                tokenizer_.advance();
            }
            else {
                outputStream_ << string(2 * currentIndentionLevel_, ' ')
                    << xmlElement(Tokenizer::TokenType::IDENTIFIER, identifier, {{ "category", "subroutine" }}) << "\n";
            }
        }
        else {
            parseIdentifier();
            tokenizer_.advance();

            if(tryParseSymbol('.')) {
                tokenizer_.advance();
                parseIdentifierAsSubroutineName();
                tokenizer_.advance();
            }
            else {
                throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Invalid subroutine-call.");
            }
        }

        parseSymbol('(');
        tokenizer_.advance();

        compileExpressionList();

        parseSymbol(')');
        tokenizer_.advance();
    }

    void CompilationEngine::openNonTerminalTag(const string& name) {
        outputStream_ << string(2 * currentIndentionLevel_, ' ') << "<" << name << ">\n";
        ++currentIndentionLevel_;
    }

    void CompilationEngine::closeNonTerminalTag(const string& name) {
        --currentIndentionLevel_;
        outputStream_ << string(2 * currentIndentionLevel_, ' ') << "</" << name << ">\n";
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

        outputStream_ << string(2 * currentIndentionLevel_, ' ')
            << xmlElement(Tokenizer::TokenType::SYMBOL, symbolToXmlValue(expectedSymbol)) << "\n";
    }

    bool CompilationEngine::tryParseSymbol(char expectedSymbol) const {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::SYMBOL || tokenizer_.symbol() != expectedSymbol) {
            return false;
        }

        outputStream_ << string(2 * currentIndentionLevel_, ' ')
            << xmlElement(Tokenizer::TokenType::SYMBOL, symbolToXmlValue(expectedSymbol)) << "\n";

        return true;
    }

    void CompilationEngine::parseOpSymbol() const {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::SYMBOL) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected a symbol-token.");
        }

        const auto symbol = tokenizer_.symbol();
        
        if(find(OPS.cbegin(), OPS.cend(), symbol) == OPS.cend()) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected operation symbol.");
        }

        outputStream_ << string(2 * currentIndentionLevel_, ' ')
            << xmlElement(Tokenizer::TokenType::SYMBOL, symbolToXmlValue(symbol)) << "\n";
    }

    bool CompilationEngine::tryParseOpSymbol() const {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::SYMBOL ||
            find(OPS.cbegin(), OPS.cend(), tokenizer_.symbol()) == OPS.cend()) {
            return false;
        }

        outputStream_ << string(2 * currentIndentionLevel_, ' ')
            << xmlElement(Tokenizer::TokenType::SYMBOL, symbolToXmlValue(tokenizer_.symbol())) << "\n";

        return true;
    }

    void CompilationEngine::parseUnaryOpSymbol() const {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::SYMBOL) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected a symbol-token.");
        }

        const auto symbol = tokenizer_.symbol();
        
        if(find(UNARY_OPS.cbegin(), UNARY_OPS.cend(), symbol) == UNARY_OPS.cend()) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected unary-operation symbol.");
        }

        outputStream_ << string(2 * currentIndentionLevel_, ' ') << xmlElement(Tokenizer::TokenType::SYMBOL, symbol) << "\n";
    }

    bool CompilationEngine::tryParseUnaryOpSymbol() const {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::SYMBOL ||
            find(UNARY_OPS.cbegin(), UNARY_OPS.cend(), tokenizer_.symbol()) == UNARY_OPS.cend()) {
            return false;
        }

        outputStream_ << string(2 * currentIndentionLevel_, ' ')
            << xmlElement(Tokenizer::TokenType::SYMBOL, tokenizer_.symbol()) << "\n";

        return true;
    }

    void CompilationEngine::parseKeyword(Tokenizer::KeyWordType expectedKeywordType) const {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::KEYWORD) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected a keyword-token.");
        }

        if(const auto keyword = tokenizer_.keyWord(); keyword != expectedKeywordType) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected keyword \"" +
                KEYWORD_TYPE_TO_STRING.at(expectedKeywordType) + "\" but got \"" + KEYWORD_TYPE_TO_STRING.at(keyword) + "\".");
        }

        outputStream_ << string(2 * currentIndentionLevel_, ' ')
            << xmlElement(Tokenizer::TokenType::KEYWORD, KEYWORD_TYPE_TO_STRING.at(expectedKeywordType)) << "\n";
    }

    bool CompilationEngine::tryParseKeyword(Tokenizer::KeyWordType expectedKeywordType) const {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::KEYWORD || tokenizer_.keyWord() != expectedKeywordType) {
            return false;
        }

        outputStream_ << string(2 * currentIndentionLevel_, ' ')
            << xmlElement(Tokenizer::TokenType::KEYWORD, KEYWORD_TYPE_TO_STRING.at(expectedKeywordType)) << "\n";

        return true;
    }

    void CompilationEngine::parseKeyword(initializer_list<Tokenizer::KeyWordType> validKeywordTypes) const {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::KEYWORD) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected a keyword-token.");
        }

        const auto keyword = tokenizer_.keyWord();

        if( find(validKeywordTypes.begin(), validKeywordTypes.end(), keyword) == validKeywordTypes.end()) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Invalid keyword \"" + KEYWORD_TYPE_TO_STRING.at(keyword) + "\".");
        }

        outputStream_ << string(2 * currentIndentionLevel_, ' ')
            << xmlElement(Tokenizer::TokenType::KEYWORD, KEYWORD_TYPE_TO_STRING.at(keyword)) << "\n";
    }

    bool CompilationEngine::tryParseKeyword(initializer_list<Tokenizer::KeyWordType> validKeywordTypes) const {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::KEYWORD || 
            find(validKeywordTypes.begin(), validKeywordTypes.end(), tokenizer_.keyWord()) == validKeywordTypes.end()) {
            return false;
        }

        outputStream_ << string(2 * currentIndentionLevel_, ' ')
            << xmlElement(Tokenizer::TokenType::KEYWORD, KEYWORD_TYPE_TO_STRING.at(tokenizer_.keyWord())) << "\n";

        return true;
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

        outputStream_ << string(2 * currentIndentionLevel_, ' ')
            << xmlElement(Tokenizer::TokenType::IDENTIFIER, identifier,
                {{ "category", SYMBOL_KIND_TO_ATTRIBUTE_NAME.at(kind) }, {"index", to_string(symbolTable_.indexOf(identifier))}, {"def", "TRUE"}}) << "\n";
    }

    void CompilationEngine::parseIdentifierAsSubroutineDefinition() {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::IDENTIFIER) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected an identifier-token.");
        }

        const auto identifier = tokenizer_.identifier();

        if(const auto symbolKind = symbolTable_.kindOf(identifier); symbolKind != SymbolTable::SymbolKind::NONE) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Invalid definition of a subroutine with the same name as a static/field variable.");
        }

        outputStream_ << string(2 * currentIndentionLevel_, ' ')
            << xmlElement(Tokenizer::TokenType::IDENTIFIER, identifier,
                {{ "category", "subroutine" }, {"def", "TRUE"}}) << "\n";
    }

    void CompilationEngine::parseIdentifier() {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::IDENTIFIER) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected an identifier-token.");
        }

        const auto identifier = tokenizer_.identifier();

        outputStream_ << string(2 * currentIndentionLevel_, ' ')
            << xmlElement(Tokenizer::TokenType::IDENTIFIER, identifier, {{ "category", SYMBOL_KIND_TO_ATTRIBUTE_NAME.at(symbolTable_.kindOf(identifier)) },
                {"index", to_string(symbolTable_.indexOf(identifier))}, {"def", "FALSE"}}) << "\n";

    }

    void CompilationEngine::parseIdentifierAsClassName() {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::IDENTIFIER) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected an identifier-token.");
        }
        
        const auto identifier = tokenizer_.identifier();

        if(symbolTable_.kindOf(identifier) != SymbolTable::SymbolKind::NONE) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected an class-name.");
        }

        outputStream_ << string(2 * currentIndentionLevel_, ' ')
            << xmlElement(Tokenizer::TokenType::IDENTIFIER, identifier, {{ "category", "class" }}) << "\n";
    }

    bool CompilationEngine::tryParseIdentifierAsClassName() {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::IDENTIFIER || symbolTable_.kindOf(tokenizer_.identifier()) != SymbolTable::SymbolKind::NONE) {
            return false;
        }

        outputStream_ << string(2 * currentIndentionLevel_, ' ')
            << xmlElement(Tokenizer::TokenType::IDENTIFIER, tokenizer_.identifier(), {{ "category", "class" }}) << "\n";

        return true;
    }

    void CompilationEngine::parseIdentifierAsSubroutineName() {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::IDENTIFIER) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected an identifier-token.");
        }

        const auto identifier = tokenizer_.identifier();
        
        if(symbolTable_.kindOf(identifier) != SymbolTable::SymbolKind::NONE) {
            throw runtime_error("Error on line " + to_string(tokenizer_.getCurrentLine()) + ": Expected an subroutine-name.");
        }

        outputStream_ << string(2 * currentIndentionLevel_, ' ')
            << xmlElement(Tokenizer::TokenType::IDENTIFIER, identifier, {{ "category", "subroutine" }}) << "\n";
    }

    bool CompilationEngine::tryParseIntConst() const {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::INT_CONST) {
            return false;
        }

        outputStream_ << string(2 * currentIndentionLevel_, ' ')
            << xmlElement(Tokenizer::TokenType::INT_CONST, tokenizer_.intVal()) << "\n";

        return true;
    }

    bool CompilationEngine::tryParseStringConst() const {
        if(tokenizer_.tokenType() != Tokenizer::TokenType::STRING_CONST) {
            return false;
        }

        outputStream_ << string(2 * currentIndentionLevel_, ' ')
            << xmlElement(Tokenizer::TokenType::STRING_CONST, tokenizer_.stringVal()) << "\n";

        return true;
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
