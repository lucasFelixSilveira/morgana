#pragma once

#include <string>
#include <vector>

struct desconstructor {
    enum reason { that };
    reason why;
    std::vector<std::string> identifiers;

    desconstructor(std::vector<std::string> identifiers) : identifiers(identifiers) {};
};


#define DESCONSTRUCTOR \
if( token[0] == '(' ) { \
    i += 1; \
    std::stringstream ss; \
    ss << token; \
 \
    if( token != "()" ) for(; i < tokens.size(); i++) { \
        ss << tokens[i]; \
        if( tokens[i][tokens[i].length()-1] == ')' ) break; \
    } \
 \
    std::vector<std::string> identifiers; \
    std::string nparen = ss.str().substr(1, ss.str().length()-2); \
 \
    ss.str(""); \
    for( int j = 0; j < nparen.length(); j++ ) { \
        if( nparen[j] == ',' ) { \
            identifiers.push_back(ss.str()); \
            ss.str(""); \
        } else ss << nparen[j]; \
    } \
 \
    desconstructor d(identifiers); \
    if( next == "@_" ) d.why = desconstructor::reason::that; \
    i++; \
    results.push_back({ ParseResultKind::Desconstructor, d }); \
    continue; \
}
