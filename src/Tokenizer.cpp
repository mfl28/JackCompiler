#include "Tokenizer.h"
#include <vector>
#include <regex>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <iostream>

using std::vector;
using std::pair;
using std::regex;
using std::string;
using std::sregex_token_iterator;
using std::runtime_error;
using std::unordered_map;
using std::ostream;
using std::stringstream;
using std::to_string;

namespace JackCompiler {
    namespace {
        const vector<pair<Tokenizer::TokenType, regex>> tokenTypeToPattern = {
            { Tokenizer::TokenType::KEYWORD,      regex{"^(class|constructor|function|method|field|static|var|int|char|boolean|void"
                                                        "|true|false|null|this|let|do|if|else|while|return)$", regex::optimize | regex::nosubs} },
            { Tokenizer::TokenType::SYMBOL,       regex{R"(^(\{|\}|\(|\)|\[|\]|\.|,|;|\+|-|\*|/|&|\||<|>|=|~)$)", regex::optimize | regex::nosubs} },
            { Tokenizer::TokenType::IDENTIFIER,   regex{"^([[:alpha:]_][[:alnum:]]*)$", regex::optimize | regex::nosubs} },
            { Tokenizer::TokenType::INT_CONST,    regex{"^([[:digit:]]+)$", regex::optimize | regex::nosubs} },
            { Tokenizer::TokenType::STRING_CONST, regex{"^\"(.*)\"$", regex::optimize | regex::nosubs} }
        };

        const regex tokenDelimiterPattern{R"(( |\{|\}|\(|\)|\[|\]|\.|,|;|\+|-|\*|/|&|\||<|>|=|~))", regex::optimize};

        const unordered_map<string, Tokenizer::KeyWordType> keywordToType{
            { "class",       Tokenizer::KeyWordType::CLASS },
            { "constructor", Tokenizer::KeyWordType::CONSTRUCTOR },
            { "function",    Tokenizer::KeyWordType::FUNCTION },
            { "method",      Tokenizer::KeyWordType::METHOD },
            { "field",       Tokenizer::KeyWordType::FIELD },
            { "static",      Tokenizer::KeyWordType::STATIC },
            { "var",         Tokenizer::KeyWordType::VAR },
            { "int",         Tokenizer::KeyWordType::INT },
            { "char",        Tokenizer::KeyWordType::CHAR },
            { "boolean",     Tokenizer::KeyWordType::BOOLEAN },
            { "void",        Tokenizer::KeyWordType::VOID },
            { "true",        Tokenizer::KeyWordType::TRUE },
            { "false",       Tokenizer::KeyWordType::FALSE },
            { "null",        Tokenizer::KeyWordType::NULL_ },
            { "this",        Tokenizer::KeyWordType::THIS },
            { "let",         Tokenizer::KeyWordType::LET },
            { "do",          Tokenizer::KeyWordType::DO },
            { "if",          Tokenizer::KeyWordType::IF },
            { "else",        Tokenizer::KeyWordType::ELSE },
            { "while",       Tokenizer::KeyWordType::WHILE },
            { "return",      Tokenizer::KeyWordType::RETURN }
        };

        const unordered_map<Tokenizer::TokenType, string> tokenTypeToTagName{
            { Tokenizer::TokenType::IDENTIFIER,   "identifier"},
            { Tokenizer::TokenType::INT_CONST,    "integerConstant"},
            { Tokenizer::TokenType::KEYWORD,      "keyword"},
            { Tokenizer::TokenType::STRING_CONST, "stringConstant"},
            { Tokenizer::TokenType::SYMBOL,       "symbol"}
        };


        void trimWhitespaceAndComments(string& line, bool& inBlockComment) {
            if(const auto firstNonWhitespaceIndex = line.find_first_not_of(" \t"); firstNonWhitespaceIndex != string::npos) {
                stringstream s;
                const auto length = line.size();
                auto inStringLiteral = false;
                auto ignoreWhitespace = false;

                for(auto i = firstNonWhitespaceIndex; i != length; ++i) {
                    if(!inBlockComment) {
                        if(!inStringLiteral) {
                            if(line[i] == ' ' || line[i] == '\t') {
                                if(ignoreWhitespace) {
                                    continue;
                                }

                                if(line[i] == '\t') {
                                    s << ' ';
                                    ignoreWhitespace = true;
                                    continue;
                                }

                                ignoreWhitespace = true;
                            }
                            else {
                                if(line[i] == '\"') {
                                    inStringLiteral = true;
                                }
                                else if(line[i] == '/') {
                                    if(i + 1 < length) {
                                        if(line[i + 1] == '/') {
                                            break;
                                        }
                                        else if(line[i + 1] == '*') {
                                            inBlockComment = true;
                                            ++i;
                                            continue;
                                        }
                                    }
                                }

                                ignoreWhitespace = false;
                            }
                        }
                        else if(line[i] == '\"') {
                            inStringLiteral = false;
                        }

                        s << line[i];
                    }
                    else if((line[i] == '*') && (i + 1 < length) && (line[i + 1] == '/')) {
                        inBlockComment = false;
                        ++i;
                    }
                }

                if(inStringLiteral) {
                    throw runtime_error("Malformed string literal. Did you forget closing '\"'?");
                }

                line = s.str();

                if(!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
                    line.pop_back();
                }
            }
            else {
                line = "";
            }
        }

        const sregex_token_iterator TOKEN_IT_END;
    }

    void Tokenizer::updateNextToken() {
        if(currentLineTokenIterator_ == TOKEN_IT_END) {
            currentLine_.clear();

            auto inBlockComment = false;
            auto blockCommentStartLine = currentLineNr_;

            while(inputStream_ && currentLine_.empty()) {
                getline(inputStream_, currentLine_);
                try {
                    trimWhitespaceAndComments(currentLine_, inBlockComment);
                }
                catch(const runtime_error& e) {
                    throw runtime_error("Tokenizer Error on line " + to_string(currentLineNr_) + ": " +  e.what());
                }
                

                ++currentLineNr_;

                if(!inBlockComment) {
                    blockCommentStartLine = currentLineNr_;
                }
            }

            if(inBlockComment) {
                throw runtime_error("Tokenizer Error: A block-comment starting on line " + to_string(blockCommentStartLine) + " was never closed.");
            }

            currentLineTokenIterator_ = sregex_token_iterator(currentLine_.cbegin(), currentLine_.cend(), tokenDelimiterPattern, {-1, 0});
        }

        currentLineTokenIterator_ = std::find_if_not(currentLineTokenIterator_, TOKEN_IT_END,
            [] (const auto& match) { return match == "" || match == " "; });

        if(currentLineTokenIterator_ != TOKEN_IT_END) {
            // collect const string token
            nextToken_ = currentLineTokenIterator_->str();
            ++currentLineTokenIterator_;

            if(nextToken_.front() == '\"' && nextToken_.back() != '\"') {
                while(currentLineTokenIterator_ != TOKEN_IT_END) {
                    nextToken_.append(*currentLineTokenIterator_);
                    ++currentLineTokenIterator_;
                    if(nextToken_.back() == '\"') {
                        break;
                    }
                }
            }
        }
        else {
            nextToken_ = "";
        }
    }

    void Tokenizer::advance() {
        if(!hasMoreTokens()) {
            throw runtime_error("Unexpected end of input.");
        }

        currentToken_ = nextToken_;
        parseCurrentToken();
        updateNextToken();
    }

    void Tokenizer::parseCurrentToken() {
        if(const auto it = std::find_if(tokenTypeToPattern.cbegin(), tokenTypeToPattern.cend(),
            [&currentToken_ = std::as_const(currentToken_)](const auto& item) { return regex_match(currentToken_, item.second); });
            it != tokenTypeToPattern.cend()) {
            currentTokenType_ = it->first;
        }
        else {
            throw runtime_error("Invalid token in line " + std::to_string(currentLineNr_) + ": >>" + currentToken_ + "<<");
        }

        if(currentTokenType_ == Tokenizer::TokenType::KEYWORD) {
            currentKeyWordType_ = keywordToType.at(currentToken_);
        }
    }

    string Tokenizer::xmlTaggedToken() const {
        string value;

        switch(currentTokenType_) {
            case Tokenizer::TokenType::IDENTIFIER:
                value = identifier();
                break;
            case Tokenizer::TokenType::INT_CONST:
                value = std::to_string(intVal());
                break;
            case Tokenizer::TokenType::KEYWORD:
                value = currentToken_;
                break;
            case Tokenizer::TokenType::STRING_CONST:
                value = stringVal();
                break;
            case Tokenizer::TokenType::SYMBOL:
                value = symbol();
                if(value == "<") { value = "&lt;"; }
                else if(value == ">") { value = "&gt;"; }
                else if(value == "&") { value = "&amp;"; }
        }

        const auto& tag = tokenTypeToTagName.at(currentTokenType_);
        return "<" + tag + "> " + value + " </" + tag + ">\n";
    }
}
