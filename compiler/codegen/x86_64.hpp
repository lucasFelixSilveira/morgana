#pragma once

#include "../codegen.hpp"
#include <sstream>
#include <string>
#include <variant>
#include <vector>
#include <array>

struct X86_64 : public CodeGen {
public:

    // Makes the setup of the x86 architecture
    X86_64() {
        symbolTable.push_back(std::vector<Symbol>{});
        stackPos = 0;
        funcid = 0;

        registerMap[RETURN] = (BitRegister) {"%rax", "%eax", "%ax", "%axl"};

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

    std::string store(type t, BitRegister reg, int& sub) override {
        std::stringstream ss;

        int pos = t.matrixPos();

        ss << '\t' << mov(t.bytes()) << ' ' << reg.at(pos) << ", " << sub << "(" << std::get<std::string>(registerMap[STACK_PTR]) << ")\n";
        sub += t.bytes();

        return ss.str();
    }

    // Creates the function prologue to the x86 architecture
    std::string prologue(function func) override {
        std::stringstream ss;

        ss << "\n.text\n";

        #ifdef UNIX_LIKE
        ss << ".globl " << func.name << "\n"
           << ".type " << func.name << ", @function\n";
        #else
        ss << ".globl " << func.name << "\n"
           << ".def " << func.name << "; .scl 2; .type " << (func.ret.bytes() * 8) << "; .endef\n"
        #endif
        ss << func.name << ":\n"
           << ".LFP" << (funcid++) <<":\n"
           << "\tpushq %rbp\n"
           << "\tmovq %rsp, %rbp\n";

        int sub = 0;
        for( auto type : func.argst ) {
            sub += type.bytes();
        }

        stackPos -= sub;
        if( sub != 0 ) ss << "\tsubq $" << sub << ", %rsp\n";

        sub = stackPos;

        if( func.argst.size() <=MAX_REGISTER_ARGUMENTS ) {
            int i = 0;
            for(auto type : func.argst) {
                ss << store(type, std::get<RegisterBitMatrix>(registerMap[ARGUMENTS]).at(i++), sub);
            }
        }

        return ss.str();
    }

    std::string epilogue() override {
        std::stringstream ss;

        ss << ".LFE" << (funcid - 1) << ":\n"
           << "\tmovq %rbp, %rsp\n"
           << "\tpopq %rbp\n"
           << "\tmovq %rdi, %rax\n"
           << "\tret\n";

        return ss.str();
    }

    std::string alloc(allocation alloc) override {
        std::stringstream ss;

        ss << "\tsubq $" << alloc.data.bytes() << ", %rsp\n";
        stackPos -= alloc.data.bytes();

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

            ss << '\t' << mov(type.bytes()) << " " << addr << "(%rbp), " << args.at(0).at(type.matrixPos()) << "\n";
        }

        return ss.str();
    }

    std::string store(std::string name, std::string value) override {
        std::stringstream ss;

        auto symbol = getSymbol(name);
        if( std::holds_alternative<SymbolTypeId>(symbol) ) {

            auto typeId = std::get<SymbolTypeId>(symbol);
            auto type = std::get<1>(typeId);
            auto addr = std::get<2>(typeId);
            auto args = std::get<RegisterBitMatrix>(registerMap[ARGUMENTS]);

            ss << '\t' << mov(type.bytes()) << " $" << value << ", " << addr << "(%rbp)\n";

        }

        return ss.str();
    }

    std::string ret() override {
        std::stringstream ss;

        ss << "\tmovq %rdi, %rax\n"
           << "\tjmp .LFE" << funcid << "\n";

        return ss.str();
    };

};
