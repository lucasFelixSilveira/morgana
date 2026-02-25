#pragma once

#include <algorithm>
#include <cstdlib>
#include <linux/limits.h>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <variant>
#include <vector>
#include <regex>

#include "main.hpp"
#include "compiler_outputs.hpp"
#include "parser/comp.hpp"
#include "parser/contexts.hpp"
#include "parser/make_it_integer.hpp"
#include "parser/checkout.hpp"
#include "parser/loop.hpp"
#include "parser/function.hpp"
#include "parser/call.hpp"
#include "parser/keywords.hpp"
#include "parser/turn.hpp"
#include "parser/declaration.hpp"
#include "parser/storage.hpp"
#include "parser/mcu/read.hpp"
#include "parser/mcu/gpio.hpp"
#include "parser/symbols.hpp"
#include "parser/type.hpp"
#include "parser/types/integer.hpp"
#include "parser/types/bool.hpp"
#include "parser/types/tuple.hpp"
#include "parser/types/validator.hpp"

std::vector<std::string> mocks;
int ctx;

enum ParseResultKind {
    none = -1,

    Desconstructor     = 100,
    Function           = 101,
    Call               = 102,
    Ret                = 103,

    Wait               = 200,
    WaitMS             = 201,

    Label              = 300,
    Branch             = 301,
    BranchNotEqualZero = 302,
    BranchEqualZero    = 303,
    BranchGrant        = 304,
    BranchLess         = 305,
    BranchGrantEqual   = 306,
    BranchLessEqual    = 307,

    Alloc              = 400,
    Store              = 401,
    Load               = 402,

    Operation          = 500,
    Alert              = 501,

    Loop               = 700,

    AddInPtr           = 800,

    // MCUs nodes
    GPIO               = 10001,
    Turn               = 10002,
    Read               = 10003,
};

using ParseResult = std::tuple<ParseResultKind, std::variant<
    std::string,
    int,
    function,
    storage,
    ret,
    call,
    turn,
    loop,
    simplebranch,
    branchmeasure,
    tuple,
    declaration<gpio>,
    declaration<call>,
    declaration<mcu_read>,
    declaration<morgana_operation>,
    declaration<symbol>,
    declaration<addinptr>
>>;

using ParseResults = std::vector<ParseResult>;

std::string getnext(std::vector<std::string>& tokens, int& i) {
    if( i >= tokens.size() ) return "";
    return tokens[i++];
}

void sys_err(const std::string& instruction, const std::string& value, const symbol& data, bool cbid = true) {
    std::string regex;
    if( std::holds_alternative<morgana_integer>(data) ) regex = std::get<morgana_integer>(data).regex;
    if( std::holds_alternative<morgana_bool>(data) ) regex = std::get<morgana_integer>(data).regex;

    CompilerOutputs::Fatal("Error When you use `" + instruction + "` the value expect need match " + regex + (cbid ? " or be a identifier" : "") + " but is " + value);
}

bool fparse = true;

ParseResults parse(std::vector<std::string>& tokens) {
    bool mcu = (params.target == "xtensa" ? true : (params.target == "avr"));

    fparse = false;
    ParseResults results = {};

    // Contexto de função atual (para gerenciar escopos)

    for( int i = 0; i < tokens.size(); i++ ) {
        std::string token = tokens[i], next = "";
        if( (i + 1) < tokens.size() ) next = tokens[i + 1];

        auto checkout = make_it_integer(token);
        switch(checkout) {
            case DEFINE_LABEL: {
                std::string label = token.substr(2, token.size() - 4);
                if(! is_identifier(label) ) CompilerOutputs::Fatal("Label statement needs a identifier");
                results.push_back({ ParseResultKind::Label, label });
            } continue;

            case RET_KEYWORD: {
                ctx = context::ALL_RETURN_INSTRUCTION;
                results.push_back({ ParseResultKind::Ret, std::monostate() });
            } continue;

            case ALERT_KEYWORD: {
                ctx = context::ALL_ALERT_INSTRUCTION;
                results.push_back({ ParseResultKind::Alert, std::monostate() });
            } continue;


            case LOOP_KEYWORD: {
                ctx = context::ALL_LOOP_STATEMENT;
                i++;
                if( getnext(tokens, i) != "{" ) CompilerOutputs::Fatal("Loop statement needs a code-block");

                std::vector<std::string> body;
                for(; i < tokens.size(); i++ ) {
                    body.push_back(tokens[i]);
                    if( tokens[i] == "}" ) break;
                }

                if( i >= tokens.size() ) CompilerOutputs::Fatal("Bad loop instruction");

                results.push_back({ ParseResultKind::Loop, loop{ .body = body } });
            } continue;

            case WAIT_KEYWORD:
            case WAITMS_KEYWORD: {
                ctx = context::ALL_WAIT_STATEMENT;
                i++;
                std::string number = getnext(tokens, i);
                if(! is_number(number) ) CompilerOutputs::Fatal("Wait statement needs a numeric complement");
                results.push_back({
                    (checkout == WAIT_KEYWORD ? ParseResultKind::Wait : ParseResultKind::WaitMS),
                    atoi(number.c_str())
                });
                i--;
            } continue;

            case BRANCH_EQUAL_ZERO_KEYWORD:
            case BRANCH_NOT_EQUAL_ZERO_KEYWORD: {
                ctx = (checkout == BRANCH_EQUAL_ZERO_KEYWORD)
                    ? context::ALL_BRANCH_IF_EQUALS_ZERO_INSTRUCTION
                    : context::ALL_BRANCH_IF_NOT_EQUALS_ZERO_INSTRUCTION;

                std::string isnt = (checkout == BRANCH_EQUAL_ZERO_KEYWORD)
                    ? "is"
                    : "isn't";

                i++;
                std::string identifier = getnext(tokens, i);
                if(! is_identifier(identifier) ) CompilerOutputs::Fatal("Branch if " + isnt + " equal zero statement needs an identifier");

                auto opt_data = symbol_table.lookup(identifier);
                if(! opt_data ) CompilerOutputs::Fatal("Branch if " + isnt + " equal zero statement needs a valid identifier");

                symbol data = *opt_data;
                if(!(
                       std::holds_alternative<morgana_load>(data)
                    || std::holds_alternative<morgana_operation>(data)
                )) CompilerOutputs::Fatal("The Branch if " + isnt + " equal zero statement needs be a valid symbol. Like a MORGANA_LOAD instruction.");

                std::string label = getnext(tokens, i);
                if(! is_identifier(label) ) CompilerOutputs::Fatal("Branch if " + isnt + " equal zero statement also needs an identifier on the second argument");

                results.push_back({
                    (checkout == BRANCH_EQUAL_ZERO_KEYWORD) ? ParseResultKind::BranchEqualZero
                                                            :  ParseResultKind::BranchNotEqualZero,
                    std::tuple<std::string, std::string> { identifier, label }
                });
                i--;
            } continue;

            case BRANCH_GRANT_KEYWORD:
            case BRANCH_LESS_KEYWORD:
            case BRANCH_GRANT_EQUAL_KEYWORD:
            case BRANCH_LESS_EQUAL_KEYWORD: {
                if( checkout == BRANCH_GRANT_KEYWORD ) ctx = context::ALL_BRANCH_IF_GREATER_INSTRUCTION;
                if( checkout == BRANCH_LESS_KEYWORD ) ctx = context::ALL_BRANCH_IF_LESS_INSTRUCTION;
                if( checkout == BRANCH_GRANT_EQUAL_KEYWORD ) ctx = context::ALL_BRANCH_IF_GREATER_EQUAL_INSTRUCTION;
                if( checkout == BRANCH_LESS_EQUAL_KEYWORD ) ctx = context::ALL_BRANCH_IF_LESS_EQUAL_INSTRUCTION;

                i++;
                std::string first = getnext(tokens, i);
                if(! is_identifier(first) ) CompilerOutputs::Fatal("Multi-State Branch statement needs a valid identifier (0)");

                auto first_data = symbol_table.lookup(first);
                if(! first_data ) CompilerOutputs::Fatal("Multi-State Branch statement needs a valid identifier (0)");

                if(!(
                       std::holds_alternative<morgana_load>(*first_data)
                    || std::holds_alternative<morgana_operation>(*first_data)
                )) CompilerOutputs::Fatal("Multi-State Branch statement needs a valid identifier (0)");

                std::string second = getnext(tokens, i);
                if(! is_identifier(second) ) CompilerOutputs::Fatal("Multi-State Branch statement needs a valid identifier (1)");

                auto second_data = symbol_table.lookup(second);
                if(! second_data ) CompilerOutputs::Fatal("Multi-State Branch statement needs a valid identifier (1)");

                if(!(
                       std::holds_alternative<morgana_load>(*second_data)
                    || std::holds_alternative<morgana_operation>(*second_data)
                )) CompilerOutputs::Fatal("Multi-State Branch statement needs a valid identifier (1)");

                std::string label = getnext(tokens, i);
                if(! is_identifier(label) ) CompilerOutputs::Fatal("Multi-State Branch statement needs also needs an identifier on the third argument");

                results.push_back({
                    ([&]() -> ParseResultKind {
                        if( checkout == BRANCH_GRANT_KEYWORD ) return ParseResultKind::BranchGrant;
                        if( checkout == BRANCH_LESS_KEYWORD ) return ParseResultKind::BranchLess;
                        if( checkout == BRANCH_GRANT_EQUAL_KEYWORD ) return ParseResultKind::BranchGrantEqual;
                        if( checkout == BRANCH_LESS_EQUAL_KEYWORD ) return ParseResultKind::BranchLessEqual;
                        return ParseResultKind::none;
                    })(),
                    std::tuple<std::string, std::string, std::string> { first, second, label }
                });
                i--;
            } continue;

            case BRANCH_KEYWORD: {
                ctx = context::ALL_BRANCH_INSTRUCTION;
                i++;
                std::string label = getnext(tokens, i);
                if(! is_identifier(label) ) CompilerOutputs::Fatal("Label statement needs an identifier");
                results.push_back({ ParseResultKind::Branch, label });
                i--;
            } continue;

            case STORE_KEYWORD: {
                ctx = context::ALL_STORE_INSTRUCTION;
                i++;
                std::string identifier = getnext(tokens, i);
                if(! is_identifier(identifier) ) CompilerOutputs::Fatal("Store statement needs an identifier");

                auto opt_data = symbol_table.lookup(identifier);
                if(! opt_data ) CompilerOutputs::Fatal("Invalid symbol: " + identifier);

                symbol data = *opt_data;

                if( std::holds_alternative<morgana_allocation>(data) ) {
                    morgana_allocation allocation = std::get<morgana_allocation>(data);

                    std::string value = getnext(tokens, i);
                    validation check;
                    if( check = check_valid(*allocation.type, value, ctx); !std::get<0>(check) ) sys_err("store", value, *allocation.type, true);

                    results.push_back({ ParseResultKind::Store, storage(identifier, value) });
                    i--;
                    continue;
                }

                if( std::holds_alternative<morgana_addinptr>(data) ) {
                    addinptr address = *std::get<morgana_addinptr>(data);

                    std::string allocation_identifier = first(address);
                    auto opt_data = symbol_table.lookup(allocation_identifier);
                    if(! std::holds_alternative<morgana_allocation>(*opt_data) ) CompilerOutputs::Fatal("Invalid symbol: " + identifier);

                    morgana_allocation allocation = std::get<morgana_allocation>(*opt_data);
                    if(! std::holds_alternative<morgana_tuple>(*allocation.type) ) CompilerOutputs::Fatal("Invalid symbol: " + identifier);

                    morgana_tuple into = std::get<morgana_tuple>(*allocation.type);

                    std::string value = getnext(tokens, i);
                    validation check;
                    if( check = check_valid(*into.types.at(second(address)), value, ctx); !std::get<0>(check) ) sys_err("store", value, *into.types.at(second(address)), true);

                    results.push_back({ ParseResultKind::Store, storage(identifier, value) });
                    i--;
                    continue;
                }

                CompilerOutputs::Fatal("Invalid symbol: " + identifier);

                i--;
            } continue;

            case CALL_KEYWORD: {
                ctx = context::ALL_CALL_INSTRUCTION;
                std::string name;
                std::vector<std::string> args;
                std::string callStr;

                int j = i + 1;
                for(; j < tokens.size(); j++ ) {
                    callStr += tokens[j] + " ";
                    if( tokens[j].find(')') != std::string::npos ) break;
                }

                if( j >= tokens.size() ) CompilerOutputs::Fatal("Bad call instruction");

                callStr = std::regex_replace(callStr, std::regex("\\s+"), "");
                std::regex r(R"((\w+)\((.*)\))");

                std::smatch match;
                if( std::regex_search(callStr, match, r) ) {
                    name = match[1].str();

                    std::stringstream ss(match[2].str());
                    std::string arg;
                    while (std::getline(ss, arg, ',')) args.push_back(arg);
                }

                auto opt_data = symbol_table.lookup(name);
                if(! opt_data ) CompilerOutputs::Fatal("Invalid symbol: " + name);

                symbol data = *opt_data;

                if(! std::holds_alternative<function_data>(data) ) CompilerOutputs::Fatal("Invalid function: " + name);
                function_data func_info = std::get<function_data>(data);

                j = 0;
                for( std::string& type : func_info.types ) {
                    validation data;
                    if( data = check_valid(type, args[j++], ctx); !std::get<0>(data) ) sys_err("call", args[j - 1], std::get<1>(data));
                }

                call c(name, args);
                results.push_back({ ParseResultKind::Call, c });
                i = j;
            } continue;

            case TURN_KEYWORD: {
                ctx = context::MCU_TURN_INSTRUCTION;

                i++;
                std::string identifier = getnext(tokens, i);

                auto opt_data = symbol_table.lookup(identifier);
                if(! opt_data ) CompilerOutputs::Fatal("Invalid GPIO symbol: " + identifier);

                symbol data = *opt_data;

                if(! std::holds_alternative<named_integers>(data)) CompilerOutputs::Fatal("Invalid symbol type: " + identifier);
                auto [check, pin] = std::get<named_integers>(data);

                if( check != "gpio_pin" ) CompilerOutputs::Fatal("Invalid symbol type: " + identifier);

                std::string toggle = getnext(tokens, i);
                if( toggle != "on" && toggle != "off" ) CompilerOutputs::Fatal("Invalid complement, you just can put the pin " + identifier + " as on/off");

                turn t(pin, toggle == "on");
                results.push_back({ ParseResultKind::Turn, t });
                i--;
            } continue;

            default: {
                auto [is, info] = is_type(token);
                if( is && is_identifier(next) ) {
                    std::string name;
                    std::vector<std::string> args;

                    std::string callStr;
                    int j = i + 1;
                    for(; j < tokens.size(); ) {
                        callStr += getnext(tokens, j) + " ";
                        if( tokens[j-1].find(')') != std::string::npos ) break;
                    }
                    i = j + 1;

                    std::regex r("([a-zA-Z0-9_]+)\\(([^)]*)[\\)]?");
                    std::smatch match;
                    if( std::regex_search(callStr, match, r) ) {
                        name = match[1].str();

                        std::stringstream ss(match[2].str());
                        std::string arg;
                        while(std::getline(ss, arg, ',')) {
                            auto n = std::remove(arg.begin(), arg.end(), ' ');
                            args.push_back(arg.substr(0, n - arg.begin()));
                        }
                    }

                    std::vector<symbol> types;
                    for( std::string& arg : args ) {
                        auto opt_data = symbol_table.lookup(arg);
                        if(! opt_data ) CompilerOutputs::Fatal("Invalid symbol: " + arg);
                        types.push_back(*opt_data);
                    }

                    std::vector<std::string> body;
                    unsigned int into = 0;

                    for( auto& arg : args ) {
                        auto opt_param_type = symbol_table.lookup(arg);
                        if( opt_param_type ) symbol_table.insert(arg, *opt_param_type);
                    }

                    while(true) {
                        if( tokens[i] == "{" ) into++;
                        else if( tokens[i] == "}" && into == 0 ) break;
                        else if( tokens[i] == "}" ) into--;
                        body.push_back(tokens[i++]);
                    }

                    if(! symbol_table.insert(name, function_data{ .types = args }) ) CompilerOutputs::Fatal("Function " + name + " already defined");

                    function f(name, types, std::get<type>(info), body);
                    results.push_back({ ParseResultKind::Function, f });

                    continue;
                }

                if( is_identifier(token) && next == "=" ) {
                    i += 2;
                    std::string instruction = getnext(tokens, i);
                    auto checkout = make_it_integer(instruction);

                    switch (checkout) {
                        case GPIO_INSTRUCTION: {
                            if(! mcu ) CompilerOutputs::Fatal("GPIO instructions are only available for MCUs");

                            std::string pin = getnext(tokens, i);
                            if(! is_number(pin) ) CompilerOutputs::Fatal("Enter with a valid GPIO. `id = gpio 12`");

                            int ipin = std::atoi(pin.c_str());
                            if(! symbol_table.insert(token, named_integers("gpio_pin", ipin)) ) CompilerOutputs::Fatal("Symbol " + token + " already defined in this scope");
                            results.push_back({ ParseResultKind::GPIO, declaration<gpio>{ token, ipin } });

                            i--;
                        } continue;

                        case READ_INSTRUCTION: {
                            ctx = context::MCU_READ_INSTRUCTION;
                            if(! mcu ) CompilerOutputs::Fatal("Digital read instructions are only available for MCUs");

                            std::string identifier = getnext(tokens, i);
                            auto opt_data = symbol_table.lookup(identifier);
                            if(! opt_data ) CompilerOutputs::Fatal("Invalid GPIO symbol: " + identifier);

                            symbol data = *opt_data;

                            if(! std::holds_alternative<std::tuple<std::string, int>>(data)) CompilerOutputs::Fatal("Invalid symbol type: " + identifier);
                            auto [check, pin] = std::get<std::tuple<std::string, int>>(data);

                            if( check != "gpio_pin" ) CompilerOutputs::Fatal("Invalid symbol type: " + identifier);

                            results.push_back({ ParseResultKind::Read, declaration<mcu_read>{ token, mcu_read { pin, is_digital(pin) } } });
                            i--;
                        } continue;

                        case ALLOC_INSTRUCTION: {
                            ctx = context::ALL_ALLOC_INSTRUCTION;
                            std::string type = getnext(tokens, i);

                            auto opt_data = symbol_table.lookup(type);
                            if(! opt_data ) CompilerOutputs::Fatal("Invalid symbol: " + type);

                            symbol data = *opt_data;
                            symbol_table.insert(token, morgana_allocation { token, std::make_shared<symbol>(data) });

                            if(! first(is_type(type)) && !std::holds_alternative<morgana_tuple>(*opt_data) ) CompilerOutputs::Fatal("Invalid symbol type: " + type);
                            results.push_back({ ParseResultKind::Alloc, declaration<symbol> { token, data } });
                            i--;
                        } continue;

                        case LOAD_INSTRUCTION: {
                            ctx = context::ALL_LOAD_INSTRUCTION;
                            std::string alloc = getnext(tokens, i);
                            if(! is_identifier(alloc) ) CompilerOutputs::Fatal("Invalid symbol: " + alloc);

                            auto opt_data = symbol_table.lookup(alloc);
                            if(! opt_data ) CompilerOutputs::Fatal("Invalid symbol: " + alloc);

                            symbol data = *opt_data;
                            if(!(
                                   std::holds_alternative<morgana_allocation>(data)
                                || std::holds_alternative<morgana_addinptr>(data)
                            )) CompilerOutputs::Fatal("Invalid symbol: " + alloc);

                            symbol_table.insert(token, alloc);
                            results.push_back({ ParseResultKind::Load, declaration<std::string> { token, alloc } });
                            i--;
                        } continue;

                        case SUB_INSTRUCTION:
                        case DIV_INSTRUCTION:
                        case MUL_INSTRUCTION:
                        case ADD_INSTRUCTION: {
                            ctx = context::ALL_OPERATION_INSTRUCTION;
                            std::string lhs = getnext(tokens, i);
                            if(! (is_identifier(lhs) || is_number(lhs)) ) CompilerOutputs::Fatal("Invalid symbol: " + lhs);

                            std::string rhs = getnext(tokens, i);
                            if(! (is_identifier(rhs) || is_number(rhs)) ) CompilerOutputs::Fatal("Invalid symbol: " + rhs);

                            if(! is_number(lhs) ) {
                                auto opt_data = symbol_table.lookup(lhs);
                                symbol data = *opt_data;
                                if( std::holds_alternative<morgana_operation>(data) ) goto already_checked_1;
                                if(! std::holds_alternative<morgana_load>(data) ) CompilerOutputs::Fatal("Invalid symbol: " + lhs);

                                auto identifier = std::get<morgana_load>(data);
                                opt_data = symbol_table.lookup(identifier);
                                if(! opt_data ) CompilerOutputs::Fatal("Invalid symbol: " + lhs);

                                symbol alloc = *opt_data;
                                if( std::holds_alternative<morgana_allocation>(alloc) ) {
                                    morgana_allocation allocation = std::get<morgana_allocation>(alloc);

                                    if(! std::holds_alternative<morgana_integer>(*allocation.type) ) CompilerOutputs::Fatal("Invalid symbol: " + lhs);
                                }

                                 if(! std::holds_alternative<morgana_addinptr>(alloc) ) CompilerOutputs::Fatal("Invalid symbol: " + lhs);
                            }

                            already_checked_1: {};

                            if(! is_number(rhs) ) {
                                auto opt_data = symbol_table.lookup(rhs);
                                symbol data = *opt_data;
                                if( std::holds_alternative<morgana_operation>(data) ) goto already_checked;
                                if(! std::holds_alternative<morgana_load>(data) ) CompilerOutputs::Fatal("Invalid symbol: " + rhs);

                                auto identifier = std::get<morgana_load>(data);
                                opt_data = symbol_table.lookup(identifier);
                                if(! opt_data ) CompilerOutputs::Fatal("Invalid symbol: " + rhs);

                                symbol alloc = *opt_data;
                                if( std::holds_alternative<morgana_allocation>(alloc) ) {
                                    morgana_allocation allocation = std::get<morgana_allocation>(alloc);

                                    if(! std::holds_alternative<morgana_integer>(*allocation.type) ) CompilerOutputs::Fatal("Invalid symbol: " + rhs);
                                }

                                if(! std::holds_alternative<morgana_addinptr>(alloc) ) CompilerOutputs::Fatal("Invalid symbol: " + rhs);
                            }

                            already_checked: {};

                            morgana_operation operation(instruction, lhs, rhs);
                            symbol_table.insert(token, operation);
                            results.push_back({ ParseResultKind::Operation, declaration<morgana_operation> { token, operation } });
                            i--;
                        } continue;

                        case ADDINPTR_INSTRUCTION: {
                            ctx = context::ALL_ADDINPTR_INSTRUCTION;

                            std::string identifier = getnext(tokens, i);
                            if(! is_identifier(identifier) ) CompilerOutputs::Fatal("Invalid symbol: " + identifier);

                            auto opt_data = symbol_table.lookup(identifier);
                            if( !opt_data ) CompilerOutputs::Fatal("Invalid symbol: " + identifier);
                            if(! std::holds_alternative<morgana_allocation>(*opt_data) ) CompilerOutputs::Fatal("Invalid symbol: " + identifier);

                            std::string bytes = getnext(tokens, i);
                            if(! is_number(bytes) ) CompilerOutputs::Fatal("Invalid addinptr instruction. addinptr instruction needs the number of bytes who will be added in the pointer in the second position");

                            declaration<addinptr> info = { token, { identifier, std::stoi(bytes) } };
                            symbol_table.insert(token, std::make_shared<addinptr>(second(info)));
                            results.push_back({ ParseResultKind::AddInPtr, info });
                            i--;
                        } continue;

                        case TUPLE_INSTRUCTION: {
                            ctx = context::ALL_TUPLE_INSTRUCTION;

                            std::vector<std::shared_ptr<symbol>> types;
                            std::vector<int> bytes;
                            int size = 0;
                            auto delimiter = [&](){ CompilerOutputs::Fatal("Invalid tuple declaration. All tuples need a block, delimited with '[' and ']'. with the types into it."); };

                            std::string open = getnext(tokens, i);
                            if( open != "[") delimiter();

                            while(true) {
                                std::string data = getnext(tokens, i);
                                if( data == "]" ) break;

                                auto opt_data = symbol_table.lookup(data);
                                types.push_back(std::make_shared<symbol>(*opt_data));
                                if(! opt_data ) CompilerOutputs::Fatal("Invalid symbol: " + data);
                                if( std::holds_alternative<morgana_integer>(*opt_data) ) {
                                    auto type = std::get<morgana_integer>(*opt_data);
                                    bytes.push_back(type.bits / 8);
                                    size += type.bits / 8;
                                }
                            }

                            symbol_table.insert(token, morgana_tuple{ bytes, types, size });
                            i--;
                        } continue;

                        case CALL_KEYWORD: {
                            ctx = context::ALL_CALL_INSTRUCTION;
                            std::string name;
                            std::vector<std::string> args;
                            std::string callStr;

                            int j = i + 1;
                            for(; j < tokens.size(); j++ ) {
                                callStr += getnext(tokens, i) + " ";
                                if (tokens[j].find(')') != std::string::npos) break;
                            }

                            std::regex r("([a-zA-Z0-9_]+)\\(([^)]*)\\)");
                            std::smatch match;
                            if( std::regex_search(callStr, match, r) ) {
                                name = match[1].str();

                                std::stringstream ss(match[2].str());
                                std::string arg;
                                while (std::getline(ss, arg, ',')) args.push_back(arg);
                            }

                            auto opt_data = symbol_table.lookup(name);
                            if(! opt_data ) CompilerOutputs::Fatal("Invalid symbol: " + name);

                            symbol data = *opt_data;

                            if(! std::holds_alternative<function_data>(data) ) CompilerOutputs::Fatal("Invalid function: " + name);
                            function_data func_info = std::get<function_data>(data);

                            j = 0;
                            for( std::string& type : func_info.types ) {
                                validation data;
                                if( data = check_valid(type, args[j++], ctx); !std::get<0>(data) ) {
                                    sys_err("call", args[j - 1], std::get<1>(data));
                                }
                            }

                            i--;
                            call c(name, args);
                            results.push_back({ ParseResultKind::Call, declaration<call>{ token, c } });
                        } continue;

                        default: CompilerOutputs::Fatal("Unknown instruction (" + instruction + ")");
                    }
                }

                continue;
            }
        }
    }

    return results;
}
