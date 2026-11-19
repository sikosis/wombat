#include "BashParser.h"

#include <cstring>
#include <sstream>

#include <tree_sitter/api.h>
#include <tree_sitter/tree-sitter-bash.h>

namespace {

void MarkProtectedLines(TSNode node, BashParseInfo& parseInfo) {
	const char* type = ts_node_type(node);
	if (std::strcmp(type, "heredoc_body") == 0 || std::strcmp(type, "heredoc_end") == 0) {
		TSPoint start = ts_node_start_point(node);
		TSPoint end = ts_node_end_point(node);
		for (uint32_t row = start.row; row <= end.row && row < parseInfo.protectedLines.size(); ++row) {
			parseInfo.protectedLines[row] = true;
		}
		return;
	}

	uint32_t childCount = ts_node_child_count(node);
	for (uint32_t index = 0; index < childCount; ++index) {
		MarkProtectedLines(ts_node_child(node, index), parseInfo);
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//

bool FindParseError(TSNode node, TSNode& errorNode) {
	if (ts_node_is_error(node) || ts_node_is_missing(node)) {
		errorNode = node;
		return true;
	}

	uint32_t childCount = ts_node_child_count(node);
	for (uint32_t index = 0; index < childCount; ++index) {
		if (FindParseError(ts_node_child(node, index), errorNode)) {
			return true;
		}
	}
	return false;
}
//---------------------------------------------------------------------------------------------------------------------------------//

std::size_t CountLines(const std::string& input) {
	std::size_t lines = 1;
	for (char character : input) {
		if (character == '\n') {
			++lines;
		}
	}
	return lines;
}
//---------------------------------------------------------------------------------------------------------------------------------//

} // namespace

bool AnalyseBash(const std::string& input, BashParseInfo& parseInfo, std::string& error) {
	TSParser* parser = ts_parser_new();
	if (parser == nullptr) {
		error = "could not create the Bash parser";
		return false;
	}
	if (!ts_parser_set_language(parser, tree_sitter_bash())) {
		ts_parser_delete(parser);
		error = "the Bash grammar is incompatible with the parser runtime";
		return false;
	}

	TSTree* tree = ts_parser_parse_string(parser, nullptr, input.data(), static_cast<uint32_t>(input.size()));
	ts_parser_delete(parser);
	if (tree == nullptr) {
		error = "the Bash parser did not return a syntax tree";
		return false;
	}

	TSNode root = ts_tree_root_node(tree);
	TSNode errorNode;
	if (ts_node_has_error(root) && FindParseError(root, errorNode)) {
		TSPoint point = ts_node_start_point(errorNode);
		std::ostringstream message;
		message << "Bash syntax error at " << point.row + 1 << ':' << point.column + 1;
		error = message.str();
		ts_tree_delete(tree);
		return false;
	}

	parseInfo.protectedLines.assign(CountLines(input), false);
	MarkProtectedLines(root, parseInfo);
	ts_tree_delete(tree);
	return true;
}
//---------------------------------------------------------------------------------------------------------------------------------//
