#include "SymbolTable.h"

namespace JackCompiler {
    void SymbolTable::startSubroutine() {
        subroutineScopeTable_.clear();
        subroutineScopeVarCount_.clear();
        classScope_ = false;
    }

    void SymbolTable::define(const std::string & name, const std::string & type, SymbolKind kind) {
        if(kind == SymbolKind::STATIC || kind == SymbolKind::FIELD) {
            classScopeTable_.emplace(name, IdentifierEntry{kind, type, classScopeVarCount_[kind]++});
        }
        else {
            subroutineScopeTable_.emplace(name, IdentifierEntry{kind, type, subroutineScopeVarCount_[kind]++});
        }
    }

    int SymbolTable::varCount(SymbolKind kind) const {
        return (kind == SymbolKind::STATIC || kind == SymbolKind::FIELD) ?
            classScopeVarCount_.at(kind) : subroutineScopeVarCount_.at(kind);
    }

    SymbolTable::SymbolKind SymbolTable::kindOf(const std::string & name) {
        if(!classScope_) {
            if(const auto it = subroutineScopeTable_.find(name); it != subroutineScopeTable_.cend()) {
                return it->second.kind;
            }
        }

        if(const auto it = classScopeTable_.find(name); it != classScopeTable_.cend()) {
            return it->second.kind;
        }

        return SymbolKind::NONE;
    }

    std::string SymbolTable::typeOf(const std::string & name) {
        if(!classScope_) {
            if(const auto it = subroutineScopeTable_.find(name); it != subroutineScopeTable_.cend()) {
                return it->second.type;
            }
            
        }

        if(const auto it = classScopeTable_.find(name); it != classScopeTable_.cend()) {
            return it->second.type;
        }

        throw std::runtime_error("Symbol-Table Error: " + name + " was not found.");
    }

    int SymbolTable::indexOf(const std::string & name) {
        if(!classScope_) {
            if(const auto it = subroutineScopeTable_.find(name); it != subroutineScopeTable_.cend()) {
                return it->second.index;
            }
        }

        if(const auto it = classScopeTable_.find(name); it != classScopeTable_.cend()) {
            return it->second.index;
        }

        return -1;
    }
}
