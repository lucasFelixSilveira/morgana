#pragma once

#include <linux/limits.h>
#include <map>
#include <memory>
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

int typesize(type t) {
    if( t.ptr ) return sizeof(char*);
    if( t.kind == type::radical::Array ) return sizeof(char*);
    return t.bytes();
}

int matrixPosByBytes(int bytes) {
    switch(bytes) {
        case 8: return 0;
        case 4: return 1;
        case 2: return 2;
        case 1: return 3;
        default: return -1;
    }
}

struct CodeGen {
public:
    enum Type : int { i64 = 0, i32, i16, i8, f32, f64 };

    enum SymbolType { VARIABLE, FUNCTION };
    using SymbolTypeId = std::tuple<SymbolType, type, int>;
    using SymbolId = std::tuple<SymbolType, int>;
    using SymbolBody = std::variant<SymbolId, SymbolTypeId>;
    using Symbol = std::tuple<std::string, SymbolBody>;
    using SymbolTable = std::vector<std::vector<Symbol>>;

    enum Register { RETURN, STACK, STACK_PTR, ARGUMENTS, UTILS_REG };
    using BitRegister = std::array<std::string, 4>;
    using RegisterBitMatrix = std::vector<BitRegister>;

    std::map<Register, std::variant<std::string, RegisterBitMatrix, BitRegister>> registerMap;
    SymbolTable symbolTable;

    std::tuple<int, std::string> lastTemp;
    int scope = 0;
    int stackPos = 0;
    int funcid = 0;

    virtual ~CodeGen() = default;
    virtual std::string entry() = 0;
    virtual std::string mov(int bytes = 0) = 0;
    virtual std::string lea(int bytes = 0) = 0;
    virtual std::string movresize(int bytes = 0) = 0;
    virtual std::string store(type t, BitRegister, int& sub) = 0;
    virtual std::string prologue(function func) = 0;
    virtual std::string epilogue() = 0;
    virtual std::string alloc(allocation alloc) = 0;
    virtual std::string load(std::string name) = 0;
    virtual std::string store(std::string name, std::string value) = 0;
    virtual std::string store(type t, int literal, int pos) = 0;
    virtual std::string ret() = 0;
    virtual std::string mock(std::string data) = 0;

    void addSymbolEntry(Symbol symbol) {
        symbolTable.at(symbolTable.size()-1).push_back(symbol);
        symbolTable.push_back(std::vector<Symbol>{});
    }

    SymbolBody getSymbol(std::string name) {
        for( auto& table : symbolTable ) {
            for( auto& symbol : table ) {
                if(std::get<0>(symbol) == name) {
                    return std::get<1>(symbol);
                }
            }
        }
    }

    bool removeSymbol(const std::string& name) {
        for (auto it = symbolTable.rbegin(); it != symbolTable.rend(); ++it) {
            auto& currentScope = *it;

            auto symbol_it = std::find_if(currentScope.begin(), currentScope.end(),
                [&name](const Symbol& symbol) {
                    return std::get<0>(symbol) == name;
                });

            if (symbol_it != currentScope.end()) {
                currentScope.erase(symbol_it);
                return true;
            }
        }

        // Símbolo não encontrado em nenhum escopo
        return false;
    }
    void addSymbol(Symbol symbol) {
        symbolTable.at(symbolTable.size()-1).push_back(symbol);
    }

    void leave() {
        symbolTable.pop_back();
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

std::string codegen(CompilerParams& params, ParseResults ast, std::unique_ptr<CodeGen>& backend) {
    gparams = params;
    if( params.target == "x86_64" ) {
        return archGen(backend, ast);
    }

    return "Fail to generate the Assembly Code";
}

bool sample = false;
bool fAccess = true;
std::string archGen(std::unique_ptr<CodeGen>& backend, ParseResults ast) {
    std::stringstream ss;
    if( fAccess ) {
        ss << backend->entry();
        fAccess = false;
    }

    for(int i = 0; i < ast.size(); i++) {
        auto& node = ast[i];

        if( first(node) == ParseResultKind::Sample ) {
            sample = true;
            continue;
        }

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
                if( type.ptr || type.kind == type::radical::Array ) sub -= sizeof(char*);
                else sub -= type.bytes();
            }

            backend->stackPos = sub;
            ParseResults body = parse(gparams, func.body);

            // Add arguments on symtable
            auto desc = std::get<desconstructor>(second(body[0]));
            for( int j = 0; j < desc.identifiers.size(); j++ ) {
                auto& id = desc.identifiers[j];
                backend->addSymbol(CodeGen::Symbol{
                    id,
                    CodeGen::SymbolTypeId {
                        CodeGen::SymbolType::VARIABLE,
                        func.argst[j],
                        (sub)
                    }
                });
                sub = (sub + func.argst[j].bytes());
            }

            // Code of the function body
            ss << codegen(gparams, body, backend);

            // Generate the epilogue
            ss << backend->epilogue();

            continue;
        }

        if( first(node) == ParseResultKind::Allocation ) {
            auto alloc = std::get<allocation>(second(node));
            int pos = backend->stackPos;
            if(! sample ) ss << backend->alloc(alloc);
            else sample = false;

            backend->addSymbol(CodeGen::Symbol{
                alloc.name,
                CodeGen::SymbolTypeId {
                    CodeGen::SymbolType::VARIABLE,
                    alloc.data,
                    pos
                }
            });

            continue;
        }

        if( first(node) == ParseResultKind::Store ) {
            auto data = std::get<store>(second(node));
            ss << backend->store(data.identifier, data.value);
            continue;
        }

        if( first(node) == ParseResultKind::Load ) {
            auto data = std::get<std::string>(second(node));
            ss << backend->load(data);
            continue;
        }

        if( first(node) == ParseResultKind::Ret ) {
            ss << backend->ret();
            continue;
        }

        if( first(node) == ParseResultKind::Mock ) {
            auto data = std::get<std::string>(second(node));
            ss << backend->mock(data);
            continue;
        }

        if( first(node) == ParseResultKind::VectorAllocation ) {
            auto data = std::get<std::tuple<type, std::vector<int>>>(second(node));
            auto vec = std::get<1>(data);
            backend->stackPos -= vec.size() * typesize(std::get<0>(data));
            int indices = backend->stackPos;
            backend->lastTemp = { backend->stackPos, "vec" };
            for( int i = 0; i < vec.size(); i++ ) {
                ss << backend->store(std::get<0>(data), vec[i], indices);
                indices += typesize(std::get<0>(data));
            }
        }

    }

    return ss.str();
}
