#pragma once
#include <regex>

namespace JackCompiler {
    class Tokenizer;
}

class JackCompiler::Tokenizer {
public:
    enum class TokenType { KEYWORD, SYMBOL, IDENTIFIER, INT_CONST, STRING_CONST };
    enum class KeyWordType {
        CLASS, METHOD, FUNCTION, CONSTRUCTOR, INT, BOOLEAN,
        CHAR, VOID, VAR, STATIC, FIELD, LET, DO, IF, ELSE, WHILE, RETURN, TRUE, FALSE,
        NULL_, THIS
    };

    explicit Tokenizer(std::istream& input) : inputStream_(input) { updateNextToken(); }

    bool hasMoreTokens() const { return !nextToken_.empty(); }
    void advance();
    TokenType tokenType() const { return currentTokenType_; }
    KeyWordType keyWord() const { return currentKeyWordType_; }
    char symbol() const { return currentToken_.front(); }
    std::string identifier() const { return currentToken_; }
    int intVal() const { return std::stoi(currentToken_); }
    std::string stringVal() const { return currentToken_.substr(1, currentToken_.size() - 2); }
    std::string xmlTaggedToken() const;
    size_t getCurrentLine() const { return currentLineNr_; }

private:
    std::istream& inputStream_;
    std::string currentToken_;
    std::string currentLine_;
    std::smatch currentKeywordMatch_;
    std::sregex_token_iterator currentLineTokenIterator_;
    size_t currentLineNr_ = 0;
    std::string nextToken_;

    TokenType currentTokenType_{};
    KeyWordType currentKeyWordType_{};
    void parseCurrentToken();
    void updateNextToken();
};

