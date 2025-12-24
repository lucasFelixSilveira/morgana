#pragma once

// ss << '\t' << movresize(rad.bytes()) << ' ' << std::get<BitRegister>(registerMap[UTILS_REG]).at(t.matrixPos()) << ", " << sub << std::get<BitRegister>(registerMap[UTILS_REG]).at(pi64) << "\n";

#include "../codegen.hpp"
#include <cstdlib>
#include <sstream>
#include <string>
#include <variant>
#include <vector>
#include <array>

#define pi64 0
#define PSIZE sizeof(char*)

// this is needed to keep track of the position of the stack pointer
int alreadypos = 0;

struct X86_64 : public CodeGen {
public:

    // Makes the setup of the x86 architecture
    X86_64() {
        symbolTable.push_back(std::vector<Symbol>{});
        stackPos = 0;
        funcid = 0;

        registerMap[RETURN] = "%rax";
        registerMap[STACK_PTR] = "%rbp";

        registerMap[UTILS_REG] = (BitRegister) { "%rax", "%eax", "%ax", "%al" };

        #ifdef UNIX_LIKE
        #define MAX_REGISTER_ARGUMENTS 6
        registerMap[ARGUMENTS] = (RegisterBitMatrix) {
            {"%rdi", "%edi", "%di", "%dil"},
            {"%rsi", "%esi", "%si", "%sil"},
            {"%rdx", "%edx", "%dx", "%dl"},
            {"%rcx", "%ecx", "%cx", "%cl"},
            {"%r8", "%r8d", "%r8w", "%r8b"},
            {"%r9", "%r9d", "%r9w", "%r9b"}
        };
        #else
        #define MAX_REGISTER_ARGUMENTS 4
        registerMap[ARGUMENTS] = (RegisterBitMatrix) {
            {"%rcx", "%ecx", "%cx", "%cl"},
            {"%rdx", "%edx", "%dx", "%dl"},
            {"%r8", "%r8d", "%r8w", "%r8b"},
            {"%r9", "%r9d", "%r9w", "%r9b"},
            {"", "", "", ""},  // stack
            {"", "", "", ""}   // stack
        };
        #endif
    }

    std::string entry() override {
        std::stringstream ss;

        #ifdef UNIX_LIKE
        ss  << ".text\n"
            << ".globl _start\n"
            << "_start:\n"
            << "\tcall main\n"
            << "\tmovq %rax, %rdi\n"
            << "\tmovq $60, %rax\n"
            << "\tsyscall\n";
        #endif

        return ss.str();
    }

    std::string mov(int bytes = 0) override {
        std::stringstream ss;

        ss << "mov";
        if(bytes == 1) {
            ss << 'b';
        } else if(bytes == 2) {
            ss << 'w';
        } else if(bytes == 4) {
            ss << 'l';
        } else if(bytes == 8) {
            ss << 'q';
        }

        return ss.str();
    }

    std::string lea(int bytes = 0) override {
        std::stringstream ss;

        ss << "lea";
        if(bytes == 1) {
            ss << 'b';
        } else if(bytes == 2) {
            ss << 'w';
        } else if(bytes == 4) {
            ss << 'l';
        } else if(bytes == 8) {
            ss << 'q';
        }

        return ss.str();
    }

    std::string sub(int bytes = 0) override {
        std::stringstream ss;

        ss << "sub";
        if(bytes == 1) {
            ss << 'b';
        } else if(bytes == 2) {
            ss << 'w';
        } else if(bytes == 4) {
            ss << 'l';
        } else if(bytes == 8) {
            ss << 'q';
        }

        return ss.str();
    }

    std::string add(int bytes = 0) override {
        std::stringstream ss;

        ss << "add";
        if(bytes == 1) {
            ss << 'b';
        } else if(bytes == 2) {
            ss << 'w';
        } else if(bytes == 4) {
            ss << 'l';
        } else if(bytes == 8) {
            ss << 'q';
        }

        return ss.str();
    }

    std::string movresize(int bytes = 0) override {
        std::stringstream ss;

        ss << "movz";
        if(bytes == 1) {
            ss << 'b';
        } else if(bytes == 2) {
            ss << 'w';
        } else if(bytes == 4) {
            ss << 'l';
        } else if(bytes == 8) {
            ss << 'q';
        }

        ss << "q";

        return ss.str();
    }

    std::string loadarg(type t, int& sub, std::string reg) {
        std::stringstream ss;

        ss << mov(t.bytes()) << " " << sub << "(" << reg << "), " << std::get<BitRegister>(registerMap[UTILS_REG]).at(t.matrixPos()) << "\n";

        return ss.str();
    }

    std::string store(type t, BitRegister reg, int& sub) override {
        std::stringstream ss;

        int pos = t.matrixPos();

        if( t.kind == type::radical::Common ) {
            sub -= t.bytes();
            ss << "\t" << mov(t.bytes()) << ' ' << reg.at(pos) << ", " << sub << "(" << std::get<std::string>(registerMap[STACK_PTR]) << ")\n";
        }

        if( t.kind == type::radical::Array ) {
            type rad = type::common(false, t.value);
            sub -= PSIZE;
            ss << '\t' << mov(PSIZE) << ' ' << reg.at(i64) << ", " << sub << '(' << std::get<std::string>(registerMap[STACK_PTR]) << ") # " << t.value << ( t.kind == type::radical::Array ? "*" : "") << "\n";
        }

        return ss.str();
    }

    // Creates the function prologue to the x86 architecture
    std::string prologue(function func) override {
        std::stringstream ss;

        ss << "\n.text\n"
           << ".globl " << func.name << "\n";

        #ifdef UNIX_LIKE
        ss << ".type " << func.name << ", @function\n";
        #else
        ss << ".def " << func.name << "; .scl 2; .type " << (func.ret.bytes() * 8) << "; .endef\n";
        #endif
        ss << func.name << ":\n"
           << ".LFP" << funcid <<":\n"
           << "\tpushq %rbp\n"
           << "\tmovq %rsp, %rbp\n"
           << "\tsubq $16, %rsp\n";

        int sub = 0;
        for( auto type : func.argst ) {
            if( type.kind == type::radical::Array ) sub += PSIZE;
            else sub += type.bytes();
        }

        stackPos -= sub;
        if( sub != 0 ) ss << "\tsubq $" << sub << ", %rsp\n";

        int ssub = 0;

        if( func.argst.size() <=MAX_REGISTER_ARGUMENTS ) {
            int i = 0;
            for( auto type : func.argst ) {
                ss << store(type, std::get<RegisterBitMatrix>(registerMap[ARGUMENTS]).at(i++), ssub);
                alreadypos += type.bytes();
            }
        }

        return ss.str();
    }

    std::string epilogue() override {
        std::stringstream ss;

        ss << ".LFE" << funcid << ":\n"
           << "\tmovq %rbp, %rsp\n"
           << "\tpopq %rbp\n"
           << "\tret\n";

        alreadypos = 0;

        return ss.str();
    }

    std::string alloc(allocation alloc) override {
        std::stringstream ss;
        int size = typesize(alloc.data);

        ss << "\t" << mov(size) << " $0, " << stackPos << "(%rbp)\n";
        stackPos -= size;

        // ss << '\t' << mov(alloc.data.bytes()) << " $0, " << stackPos - alloc.data.bytes() << "(%rbp)\n";

        return ss.str();
    };

    std::string load(std::string name) override {
        std::stringstream ss;

        auto symbol = getSymbol(name);
        if( std::holds_alternative<SymbolTypeId>(symbol) ) {
            auto typeId = std::get<SymbolTypeId>(symbol);
            auto type = std::get<1>(typeId);
            auto addr = std::get<2>(typeId);
            auto args = std::get<RegisterBitMatrix>(registerMap[ARGUMENTS]);

            int size = typesize(type);
            ss << '\t' << lea(size) << " " << addr << "(%rbp), " << args.at(0).at(matrixPosByBytes(size)) << "\n";
        }

        return ss.str();
    }

    std::string store(std::string name, std::string value) override {
        std::stringstream ss;

        auto symbol = getSymbol(name);
        if( value == "temp" && std::get<1>(lastTemp) == "vec" ) {
            auto id = std::get<SymbolTypeId>(symbol);
            removeSymbol(name);
            addSymbol(Symbol {
                name,
                SymbolTypeId {
                    SymbolType::VARIABLE,
                    std::get<1>(id),
                    std::get<0>(lastTemp)
                }
            });

            return "";
        }

        if( std::holds_alternative<SymbolTypeId>(symbol) ) {
            auto typeId = std::get<SymbolTypeId>(symbol);
            auto type = std::get<1>(typeId);
            auto addr = std::get<2>(typeId);
            auto args = std::get<RegisterBitMatrix>(registerMap[ARGUMENTS]);

            ss << '\t' << mov(type.bytes()) << " $" << value << ", " << addr << "(%rbp)\n";

        }

        return ss.str();
    }

    std::string store(type t, int literal, int pos) override {
        std::stringstream ss;

        ss << '\t' << mov(t.bytes()) << " $" << literal << ", " << pos << "(%rbp)\n";
        stackPos -= t.bytes();

        return ss.str();
    }

    std::string ret() override {
        std::stringstream ss;

        ss << "\tmovq %rdi, %rax\n"
           << "\tjmp .LFE" << funcid << "\n";

        return ss.str();
    };

    std::string mock(std::string data) override {
        std::stringstream ss;
        ss << "\tcall morg." << data << '\n';
        return ss.str();
    }

    std::string getPointerElement(std::string name, std::string index) override {
        std::stringstream ss;

        auto symbol = getSymbol(name);
        if( std::holds_alternative<SymbolTypeId>(symbol) ) {
            auto typeId = std::get<SymbolTypeId>(symbol);
            auto type = std::get<1>(typeId);
            auto addr = std::get<2>(typeId);
            auto args = std::get<RegisterBitMatrix>(registerMap[ARGUMENTS]);

            auto x = is_type(type.value);
            auto y = std::get<1>(second(x));

            int size = typesize(type);
            ss << '\t' << lea(size) << " " << addr << '+' << (std::atoi(index.c_str()) * y.bytes()) << "(%rbp), " << args.at(1).at(matrixPosByBytes(size)) << "\n";
            ss << '\t' << mov(y.bytes()) << " (" << args.at(1).at(matrixPosByBytes(size)) << "), " << args.at(0).at(matrixPosByBytes(y.bytes())) << "\n";
        }

        return ss.str();
    }

};
