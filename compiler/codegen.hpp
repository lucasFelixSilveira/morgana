#pragma once

#include <linux/limits.h>
#include <sstream>
#include <string>

#include "compiler_outputs.hpp"
#include "params.hpp"
#include "parser.hpp"
#include "sys.hpp"

std::map<std::string, std::string> morgana_alias;
SystemInfo sys;

std::string codegen(CompilerParams& params, ParseResults ast) {
    std::stringstream ss;

    bool optimized = params.optimized;
    if( optimized ) ss << "#define limited_hardware\n";

    ss << "#include <stdint.h>\n";
    ss << "#if not(defined(limited_hardware))\n";
    ss << "#include <array>\n";
    ss << "#include <vector>\n";
    ss << "#endif\n\n";

    ss << "struct Morgana {\n";
    ss << "#   if defined(limited_hardware)\n";
    ss << "    Morgana() = default;\n";
    ss << "#   else\n";
    ss << "    char **argv;\n";
    ss << "    Morgana(char **argv) : argv(argv) {}\n";
    ss << "#   endif\n";

    for( auto& node : ast ) {
        if( first(node) == ParseResultKind::Alias ) {
            alias data = std::get<alias>(second(node));

            ss << "    using " << data.name << " = " << data.data.string(optimized) << ";\n";
        }
    }

    for( int i = 0; i < ast.size(); i++ ) {
        auto& node = ast[i];

        if( first(node) == ParseResultKind::Function ) {
            function func = std::get<function>(second(node));

            if( func.name == "main" && !( func.argst.empty() ) ) CompilerOutputs::Fatal("Syntax error - Main function cannot have arguments.");

            ss << "\n    " << func.ret.string(optimized) << ' ' << func.name << '(';

            if( (i + 1) >= ast.size() ) CompilerOutputs::Fatal("Syntax error - Incomplete function.");
            if( first(ast[i + 1]) != ParseResultKind::Desconstructor ) CompilerOutputs::Fatal("Syntax error - All functions need a destructor.");

            auto d = std::get<desconstructor>(second(ast[i + 1]));

            if( d.why == desconstructor::reason::that ) {
                auto& argst = func.argst;
                auto& argsv = d.identifiers;

                if( argsv.size() != argst.size() ) CompilerOutputs::Fatal("Syntax error - mismatched argument count. (" + std::to_string(argst.size()) + " != " + std::to_string(argsv.size()) + ")");

                for( int j = 0; j < func.argst.size(); j++ ) {
                    ss << argst[j].string(optimized, argsv[j]);
                    if( j != func.argst.size() - 1 ) ss << ", ";
                }
            }

            ss << ") {\n";

            ss << "        return 0;\n";
            ss << "    }\n";
        }
    }

    ss << "};\n\n";

    ss << "#if defined(limited_hardware)\n";
    ss << "int main() {\n";
    ss << "    Morgana run;\n";
    ss << "    return run.main();\n";
    ss << "}\n";
    ss << "#else\n";
    ss << "int main(int argc, char **argv) {\n";
    ss << "    Morgana run(argv);\n";
    ss << "    return run.main();\n";
    ss << "}\n";
    ss << "#endif\n";

    return ss.str();
}
