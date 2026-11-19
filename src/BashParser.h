#ifndef WOMBAT_BASH_PARSER_H
#define WOMBAT_BASH_PARSER_H

#include <string>
#include <vector>

struct BashParseInfo {
	std::vector<bool> protectedLines;
};

bool AnalyseBash(const std::string& input, BashParseInfo& parseInfo, std::string& error);

#endif
