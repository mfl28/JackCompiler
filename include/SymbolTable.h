#pragma once
#include <string>
#include <unordered_map>

namespace JackCompiler {
    class SymbolTable {
    public:
        /**
         * \brief The different kinds of symbols/variables.
         * Note: STATIC and FIELD variables always have class-scope.
         *       ARG and VAR variables always have subroutine-scope.
         */
        enum class SymbolKind { STATIC, FIELD, ARG, VAR, NONE };

        /**
         * \brief Starts a new subroutine scope by resetting the subroutine table and
         * variable counts.
         */
        void startSubroutine();

        /**
         * \brief Defines a new identifier of the given name, type and kind and
         * assigns it a running index.
         * \param name 
         * \param type 
         * \param kind 
         */
        void define(const std::string& name, const std::string& type, SymbolKind kind);

        /**
         * \brief Gets the number of variables of the given type defined in the
         * current scope.
         * \param kind 
         * \return The number of variables.
         */
        int varCount(SymbolKind kind) const;

        /**
         * \brief Gets the kind of the named identifier in the current scope.
         * \param name 
         * \return The kind of the identifier
         */
        SymbolKind kindOf(const std::string& name);

        /**
         * \brief Gets the type of the named identifier in the current scope.
         * \param name 
         * \return The type of the identifier
         */
        std::string typeOf(const std::string& name);

        /**
         * \brief The index that was assigned to the named identifier.
         * \param name 
         * \return The index of the identifier
         */
        int indexOf(const std::string& name);

    private:
        struct IdentifierEntry {
            SymbolKind kind{};
            std::string type;
            int index{};
        };

        bool classScope_ = true;

        std::unordered_map<std::string, IdentifierEntry> classScopeTable_;
        std::unordered_map<SymbolKind, int> classScopeVarCount_;

        std::unordered_map<std::string, IdentifierEntry> subroutineScopeTable_;
        std::unordered_map<SymbolKind, int> subroutineScopeVarCount_;
    };
}
