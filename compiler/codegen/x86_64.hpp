#pragma once

#include "../codegen.hpp"
#include <array>
#include <sstream>
#include <string>
#include <vector>

struct X86_64 : public CodeGen {
public:

    // Makes the setup of the x86 architecture
    X86_64() {
        symbolTable.push(std::vector<Symbol>{});
        stackPos = 0;
        funcid = 0;

        registerMap[RETURN] = "%rax";
        registerMap[STACK] = "%rsp";
        registerMap[STACK_PTR] = "%rbp";

        registerMap[ARGUMENTS] = RegisterBitMatrix {
            // [64-bit,    32-bit,    16-bit,    8-bit]
            {"%rdi",    "%edi",    "%di",     "%dil"},
            {"%rsi",    "%esi",    "%si",     "%sil"},
            {"%rdx",    "%edx",    "%dx",     "%dl"},
            {"%rcx",    "%ecx",    "%cx",     "%cl"},
            {"%r8",     "%r8d",    "%r8w",    "%r8b"},
            {"%r9",     "%r9d",    "%r9w",    "%r9b"}
        };
    }

    std::string entry() override {
        std::stringstream ss;

        ss << ".text\n"
           << ".globl _start\n"
           << "_start:\n"
           << "\tcall main\n"
           << "\tmovq %rax, %rdi\n"
           << "\tmovq $60, %rax\n"
           << "\tsyscall\n";

        return ss.str();
    }

    std::string mov(int bytes = 0) override {
        std::stringstream ss;

        ss << "mov";
        if(bytes == 1) {
            ss << "b";
        } else if(bytes == 2) {
            ss << "w";
        } else if(bytes == 4) {
            ss << "l";
        } else if(bytes == 8) {
            ss << "q";
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

        ss << "\n.text\n"
           << ".globl " << func.name << "\n"
           << func.name << ":\n"
           << ".LFP" << (funcid++) <<":\n"
           << "\tpushq %rbp\n"
           << "\tmovq %rsp, %rbp\n";

        int sub = 0;
        for( auto type : func.argst ) {
            sub += type.bytes();
        }

        stackPos -= sub;
        ss << "\tsubq $8, %rsp\n";

        sub = stackPos;

        if( func.argst.size() <=6 ) {
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
           << "\tret\n";

        return ss.str();
    }
};
