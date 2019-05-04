#include "SymbolTable.h"

using std::string;

namespace JackCompiler {
    void SymbolTable::startSubroutine() {
        subroutineScopeTable_.clear();
        subroutineScopeVarCount_.clear();
        classScope_ = false;
    }

    void SymbolTable::define(const string& name, const string& type, SymbolKind kind) {
        if(kind == SymbolKind::STATIC || kind == SymbolKind::FIELD) {
            classScopeTable_.emplace(name, IdentifierEntry{kind, type, classScopeVarCount_[kind]++});
        }
        else {
            subroutineScopeTable_.emplace(name, IdentifierEntry{kind, type, subroutineScopeVarCount_[kind]++});
        }
    }

    int SymbolTable::varCount(SymbolKind kind) const {
        if(kind == SymbolKind::STATIC || kind == SymbolKind::FIELD) {
            // return classScopeVarCount_[kind];
            if(const auto it = classScopeVarCount_.find(kind); it != classScopeVarCount_.cend()) {
                return it->second;
            }
        }
        else if(const auto it = subroutineScopeVarCount_.find(kind); it != subroutineScopeVarCount_.cend()) {
            return it->second;
        }
        // return subroutineScopeVarCount_[kind];

        return 0;
    }

    SymbolTable::SymbolKind SymbolTable::kindOf(const string& name) {
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

    std::string SymbolTable::typeOf(const string& name) {
        if(!classScope_) {
            if(const auto it = subroutineScopeTable_.find(name); it != subroutineScopeTable_.cend()) {
                return it->second.type;
            }
        }
        
        if(const auto it = classScopeTable_.find(name); it != classScopeTable_.cend()) {
            return it->second.type;
        }

        throw std::runtime_error{"Symbol-Table: " + name + " does not exist."};
    }

    int SymbolTable::indexOf(const string & name) {
        if(!classScope_) {
            if(const auto it = subroutineScopeTable_.find(name); it != subroutineScopeTable_.cend()) {
                return it->second.index;
            }
        }
        
        if(const auto it = classScopeTable_.find(name); it != classScopeTable_.cend()) {
            return it->second.index;
        }

        throw std::runtime_error{"Symbol-Table: " + name + " does not exist."};
    }
}
