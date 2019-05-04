#pragma once
#include <string>
#include <unordered_map>

namespace JackCompiler {
    class SymbolTable;
}


class JackCompiler::SymbolTable {
public:
    enum class SymbolKind { STATIC, FIELD, ARG, VAR, NONE };

    void startSubroutine();
    void define(const std::string& name, const std::string& type, SymbolKind kind);
    int varCount(SymbolKind kind) const;
    SymbolKind kindOf(const std::string& name);
    std::string typeOf(const std::string& name);
    int indexOf(const std::string& name);

private:
    struct IdentifierEntry {
        SymbolKind kind{};
        std::string type;
        int index = 0;
    };

    bool classScope_ = true;

    std::unordered_map<std::string, IdentifierEntry> classScopeTable_;
    std::unordered_map<SymbolKind, int> classScopeVarCount_;

    std::unordered_map<std::string, IdentifierEntry> subroutineScopeTable_;
    std::unordered_map<SymbolKind, int> subroutineScopeVarCount_;
};

