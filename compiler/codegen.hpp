#pragma once

#include <linux/limits.h>
#include <map>
#include <sstream>
#include <stack>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include "params.hpp"
#include "parser.hpp"

#if defined(__linux__) || defined(__APPLE__)
#define UNIX_LIKE
#endif

struct CodeGen {
public:
    enum Type : int { i64 = 0, i32, i16, i8, f32, f64 };

    enum SymbolType { VARIABLE, FUNCTION };
    using SymbolTypeId = std::tuple<SymbolType, Type, int>;
    using SymbolId = std::tuple<SymbolType, int>;
    using SymbolBody = std::variant<SymbolId, SymbolTypeId>;
    using Symbol = std::tuple<std::string, SymbolBody>;
    using SymbolTable = std::stack<std::vector<Symbol>>;

    enum Register { RETURN, STACK, STACK_PTR, ARGUMENTS };
    using BitRegister = std::array<std::string, 4>;
    using RegisterBitMatrix = std::vector<BitRegister>;

    std::map<Register, std::variant<std::string, RegisterBitMatrix>> registerMap;
    SymbolTable symbolTable;

    int scope = 0;
    int stackPos;
    int funcid;

    virtual ~CodeGen() = default;
    virtual std::string entry() = 0;
    virtual std::string mov(int bytes = 0) = 0;
    virtual std::string store(type t, BitRegister, int& sub) = 0;
    virtual std::string prologue(function func) = 0;
    virtual std::string epilogue() = 0;

    void addSymbolEntry(Symbol symbol) {
        symbolTable.top().push_back(symbol);
        symbolTable.push(std::vector<Symbol>{});
    }

    void addSymbol(Symbol symbol) {
        symbolTable.top().push_back(symbol);
    }

    void leave() {
        symbolTable.pop();
    }
};

#include "codegen/x86_64.hpp"

std::string archGen(std::unique_ptr<CodeGen>& cg, ParseResults ast);

CompilerParams gparams;
std::string codegen(CompilerParams& params, ParseResults ast) {
    gparams = params;
    if( params.target == "x86_64" ) {
        std::unique_ptr<CodeGen> backend = std::make_unique<X86_64>();
        return archGen(backend, ast);
    }

    return "Fail to generate the Assembly Code";
}

bool fAccess = true;
std::string archGen(std::unique_ptr<CodeGen>& backend, ParseResults ast) {
    std::stringstream ss;
    if( fAccess ) {
        ss << backend->entry();
        fAccess = false;
    }

    for(int i = 0; i < ast.size(); i++) {
        auto& node = ast[i];

        if( first(node) == ParseResultKind::Desconstructor ) continue;

        if( first(node) == ParseResultKind::Function ) {
            // Make function prologue
            auto func = std::get<function>(second(node));
            ss << backend->prologue(func);

            // Add function to symtable
            backend->addSymbolEntry(CodeGen::Symbol{
                func.name,
                CodeGen::SymbolId {
                    CodeGen::SymbolType::FUNCTION,
                    (backend->funcid-1)
                }
            });

            // calculate stack size
            int sub = 0;
            for( auto type : func.argst ) {
                sub -= type.bytes();
            }

            ParseResults body = parse(gparams, func.body);

            // Add arguments on symtable
            auto desc = std::get<desconstructor>(second(body[0]));
            for( int j = 0; j < desc.identifiers.size(); j++ ) {
                auto& id = desc.identifiers[j];
                backend->addSymbol(CodeGen::Symbol{
                    id,
                    CodeGen::SymbolTypeId {
                        CodeGen::SymbolType::VARIABLE,
                        (CodeGen::Type) func.argst[j].matrixPos(),
                        (sub)
                    }
                });
                sub = (sub + func.argst[j].bytes());
            }

            // Code of the function body
            ss << codegen(gparams, body);

            // Generate the epilogue
            ss << backend->epilogue();
        }
    }

    return ss.str();
}
