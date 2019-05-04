#pragma once
#include <regex>

namespace JackCompiler {
    class Tokenizer;
}

class JackCompiler::Tokenizer {
public:
    /**
     * \brief The types of tokens defined in the Jack language.
     */
    enum class TokenType { KEYWORD, SYMBOL, IDENTIFIER, INT_CONST, STRING_CONST };

    /**
     * \brief The possible types of keywords defined in the Jack language.
     */
    enum class KeyWordType {
        CLASS, METHOD, FUNCTION, CONSTRUCTOR, INT, BOOLEAN, CHAR, VOID, VAR, STATIC,
        FIELD, LET, DO, IF, ELSE, WHILE, RETURN, TRUE, FALSE, NULL_, THIS
    };

    /**
     * \brief Creates a new tokenizer for a provided input-stream and gets ready to parse the
     * first token (if one exists).
     * \param inputStream
     */
    explicit Tokenizer(std::istream& inputStream) : inputStream_{inputStream} { updateNextToken(); }

    /**
     * \brief Checks if there exists another valid token in the input-stream.
     * \return True if another token exists, otherwise false
     */
    bool hasMoreTokens() const { return !nextToken_.empty(); }

    /**
     * \brief Sets the current token to the next token encountered
     * in the input-stream. Will throw a runtime_error if no next token exists.
     */
    void advance();

    /**
     * \brief Gets the type of the current token.
     * \return The token-type
     */
    TokenType tokenType() const { return currentTokenType_; }

    /**
     * \brief Gets the keyword-type of the current token. Must only
     * be called if the current token's type if KEYWORD.
     * \return The keyword-type
     */
    KeyWordType keyWord() const { return currentKeyWordType_; }

    /**
     * \brief Gets the symbol that is the current token. Must only
     * be called if the current token's type is SYMBOL.
     * \return The symbol
     */
    char symbol() const { return currentToken_.front(); }

    /**
     * \brief Gets the identifier that is the current token. Most
     * only be called if the current token's type is IDENTIFIER.
     * \return The identifier
     */
    std::string identifier() const { return currentToken_; }

    /**
     * \brief Gets the integer-value that is represented by the current token.
     * Must only be called if the current token's type is INT_CONST.
     * \return The integer-value
     */
    int intVal() const { return std::stoi(currentToken_); }

    /**
     * \brief Gets the string-value that is represented by the current token 
     * (without the enclosing double quotes). Most only be called if the current
     * token's type is STRING_CONST.
     * \return The string-value
     */
    std::string stringVal() const { return currentToken_.substr(1, currentToken_.size() - 2); }
    
    /**
     * \brief Gets the line number of the current token.
     * \return The line number
     */
    size_t getCurrentLine() const { return currentLineNr_; }

private:
    std::istream& inputStream_;
    std::string currentToken_;
    std::string currentLine_;
    std::smatch currentKeywordMatch_;
    std::sregex_token_iterator currentLineTokenIterator_;
    size_t currentLineNr_{};
    TokenType currentTokenType_{};
    KeyWordType currentKeyWordType_{};
    std::string nextToken_;

    void parseCurrentToken();
    void updateNextToken();
};

