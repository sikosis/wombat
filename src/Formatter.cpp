#include "Formatter.h"

#include "BashParser.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

namespace {

std::string Lowercase(std::string value) {
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
		return static_cast<char>(std::tolower(character));
	});
	return value;
}
//---------------------------------------------------------------------------------------------------------------------------------//

std::string TrimTrailingWhitespace(std::string line) {
	while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
		line.pop_back();
	}
	return line;
}
//---------------------------------------------------------------------------------------------------------------------------------//

std::string ConvertLeadingSpacesToTabs(const std::string& line) {
	std::size_t spaces = 0;
	while (spaces < line.size() && line[spaces] == ' ') {
		++spaces;
	}

	return std::string(spaces / 4, '\t') + std::string(spaces % 4, ' ') + line.substr(spaces);
}
//---------------------------------------------------------------------------------------------------------------------------------//

void RemoveOutputSpaces(std::string& output) {
	while (!output.empty() && output.back() == ' ') {
		output.pop_back();
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//

void AddOutputSpace(std::string& output) {
	if (!output.empty() && output.back() != ' ' && output.back() != '\t') {
		output.push_back(' ');
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//

bool IsCppOperator(const std::string& line, std::size_t index, std::size_t& length) {
	static const std::vector<std::string> doubleOperators = {
		"==", "!=", "<=", ">=", "+=", "-=", "*=", "/=", "%=", "&&", "||", "<<", ">>"
	};
	for (const std::string& operatorText : doubleOperators) {
		if (line.compare(index, operatorText.size(), operatorText) == 0) {
			length = operatorText.size();
			return true;
		}
	}
	if (line[index] == '=') {
		length = 1;
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------------------------------------------------------------//

std::string TidyCppLine(std::string line) {
	static const std::vector<std::string> keywords = {"if", "for", "while", "switch", "catch"};
	std::size_t indentationEnd = line.find_first_not_of("\t ");
	if (indentationEnd == std::string::npos) {
		return line;
	}

	for (const std::string& keyword : keywords) {
		const std::string compact = keyword + "(";
		if (line.compare(indentationEnd, compact.size(), compact) == 0) {
			line.insert(indentationEnd + keyword.size(), " ");
		}
	}

	std::string output = line.substr(0, indentationEnd);
	bool singleQuoted = false;
	bool doubleQuoted = false;
	bool escaped = false;
	for (std::size_t index = indentationEnd; index < line.size(); ++index) {
		char character = line[index];
		if (!singleQuoted && !doubleQuoted && character == '/' && index + 1 < line.size() && line[index + 1] == '/') {
			AddOutputSpace(output);
			output += line.substr(index);
			break;
		}
		if (!escaped && character == '\'' && !doubleQuoted) {
			singleQuoted = !singleQuoted;
		} else if (!escaped && character == '"' && !singleQuoted) {
			doubleQuoted = !doubleQuoted;
		}
		if (singleQuoted || doubleQuoted) {
			output.push_back(character);
			escaped = !escaped && character == '\\';
			continue;
		}
		escaped = false;

		std::size_t operatorLength = 0;
		if (IsCppOperator(line, index, operatorLength)) {
			RemoveOutputSpaces(output);
			AddOutputSpace(output);
			output.append(line, index, operatorLength);
			AddOutputSpace(output);
			index += operatorLength - 1;
			while (index + 1 < line.size() && line[index + 1] == ' ') {
				++index;
			}
		} else if (character == ',') {
			RemoveOutputSpaces(output);
			output += ", ";
			while (index + 1 < line.size() && line[index + 1] == ' ') {
				++index;
			}
		} else if (character == ';') {
			RemoveOutputSpaces(output);
			output.push_back(';');
			std::size_t next = line.find_first_not_of(' ', index + 1);
			if (next != std::string::npos && line[next] != ')' && line[next] != '}') {
				output.push_back(' ');
				index = next - 1;
			}
		} else if (character == '{') {
			RemoveOutputSpaces(output);
			if (!output.empty() && output.back() != '(' && output.back() != '[' && output.back() != '{') {
				AddOutputSpace(output);
			}
			output.push_back('{');
		} else {
			output.push_back(character);
		}
	}
	return TrimTrailingWhitespace(output);
}
//---------------------------------------------------------------------------------------------------------------------------------//

std::string TidyMarkdownLine(std::string line) {
	std::size_t content = line.find_first_not_of("\t ");
	if (content == std::string::npos) {
		return line;
	}

	if (line[content] == '#') {
		std::size_t headingEnd = line.find_first_not_of('#', content);
		if (headingEnd != std::string::npos && line[headingEnd] != ' ') {
			line.insert(headingEnd, " ");
		}
	}

	if (content + 1 < line.size() && (line[content] == '-' || line[content] == '*' || line[content] == '+')
		&& line[content + 1] != ' ') {
		line.insert(content + 1, " ");
	}
	if (line[content] == '>' && content + 1 < line.size() && line[content + 1] != ' ') {
		line.insert(content + 1, " ");
	}
	std::size_t numberEnd = content;
	while (numberEnd < line.size() && std::isdigit(static_cast<unsigned char>(line[numberEnd]))) {
		++numberEnd;
	}
	if (numberEnd > content && numberEnd + 1 < line.size() && line[numberEnd] == '.' && line[numberEnd + 1] != ' ') {
		line.insert(numberEnd + 1, " ");
	}
	return line;
}
//---------------------------------------------------------------------------------------------------------------------------------//

std::string TidyHtmlLine(const std::string& line) {
	std::string output;
	bool singleQuoted = false;
	bool doubleQuoted = false;
	for (std::size_t index = 0; index < line.size(); ++index) {
		char character = line[index];
		if (character == '\'' && !doubleQuoted) {
			singleQuoted = !singleQuoted;
		} else if (character == '"' && !singleQuoted) {
			doubleQuoted = !doubleQuoted;
		}
		if (character == '=' && !singleQuoted && !doubleQuoted) {
			RemoveOutputSpaces(output);
			output.push_back('=');
			while (index + 1 < line.size() && line[index + 1] == ' ') {
				++index;
			}
		} else {
			output.push_back(character);
		}
	}
	return output;
}
//---------------------------------------------------------------------------------------------------------------------------------//

std::string TidyShellLine(std::string line) {
	static const std::vector<std::string> keywords = {"if", "for", "while", "until", "case"};
	std::size_t content = line.find_first_not_of("\t ");
	if (content == std::string::npos || line[content] == '#') {
		return line;
	}

	for (const std::string& keyword : keywords) {
		if (line.compare(content, keyword.size(), keyword) == 0 && content + keyword.size() < line.size()
			&& line[content + keyword.size()] != ' ' && line[content + keyword.size()] != '\t') {
			line.insert(content + keyword.size(), " ");
		}
	}

	std::string output = line.substr(0, content);
	bool singleQuoted = false;
	bool doubleQuoted = false;
	bool escaped = false;
	bool testExpression = false;
	for (std::size_t index = content; index < line.size(); ++index) {
		char character = line[index];
		if (!singleQuoted && !doubleQuoted && character == '#'
			&& (index == content || std::isspace(static_cast<unsigned char>(line[index - 1])))) {
			AddOutputSpace(output);
			output += line.substr(index);
			break;
		}
		if (!escaped && character == '\'' && !doubleQuoted) {
			singleQuoted = !singleQuoted;
		} else if (!escaped && character == '"' && !singleQuoted) {
			doubleQuoted = !doubleQuoted;
		}
		if (singleQuoted || doubleQuoted) {
			output.push_back(character);
			escaped = !escaped && character == '\\';
			continue;
		}
		escaped = false;

		bool startsDoubleTest = character == '[' && index + 1 < line.size() && line[index + 1] == '[';
		bool startsSingleTest = character == '[' && (index == content
			|| std::isspace(static_cast<unsigned char>(line[index - 1])) || line[index - 1] == ';')
			&& index + 1 < line.size() && std::isspace(static_cast<unsigned char>(line[index + 1]));
		if (startsDoubleTest || startsSingleTest) {
			testExpression = true;
			output.push_back(character);
			if (startsDoubleTest) {
				output.push_back('[');
				++index;
			}
			if (index + 1 < line.size() && line[index + 1] != ' ') {
				output.push_back(' ');
			}
			continue;
		}
		if (character == ']' && testExpression) {
			RemoveOutputSpaces(output);
			AddOutputSpace(output);
			if (index + 1 < line.size() && line[index + 1] == ']') {
				output += "]]";
				++index;
			} else {
				output.push_back(']');
			}
			testExpression = false;
			continue;
		}
		if (testExpression) {
			output.push_back(character);
			continue;
		}

		std::size_t operatorLength = 0;
		if ((character == '&' || character == '|') && index + 1 < line.size() && line[index + 1] == character) {
			operatorLength = 2;
		} else if (character == '|') {
			operatorLength = 1;
		}
		if (operatorLength > 0) {
			RemoveOutputSpaces(output);
			AddOutputSpace(output);
			output.append(line, index, operatorLength);
			output.push_back(' ');
			index += operatorLength - 1;
			while (index + 1 < line.size() && line[index + 1] == ' ') {
				++index;
			}
		} else if (character == ';' && (index + 1 >= line.size() || line[index + 1] != ';')) {
			RemoveOutputSpaces(output);
			output += "; ";
			while (index + 1 < line.size() && line[index + 1] == ' ') {
				++index;
			}
		} else {
			output.push_back(character);
		}
	}
	return TrimTrailingWhitespace(output);
}
//---------------------------------------------------------------------------------------------------------------------------------//

bool EndsWith(const std::string& value, const std::string& suffix) {
	return value.size() >= suffix.size()
		&& value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}
//---------------------------------------------------------------------------------------------------------------------------------//

bool StartsWith(const std::string& value, const std::string& prefix) {
	return value.compare(0, prefix.size(), prefix) == 0;
}
//---------------------------------------------------------------------------------------------------------------------------------//

bool IsShellClosingLine(const std::string& line) {
	return line == "fi" || StartsWith(line, "fi ") || line == "done" || StartsWith(line, "done ")
		|| line == "esac" || StartsWith(line, "esac ") || line == "}"
		|| line == "else" || line.compare(0, 5, "elif ") == 0;
}
//---------------------------------------------------------------------------------------------------------------------------------//

bool IsShellOpeningLine(const std::string& line) {
	return line == "else" || line == "do" || line == "then" || EndsWith(line, "; then")
		|| EndsWith(line, "; do") || EndsWith(line, " in") || EndsWith(line, " {");
}
//---------------------------------------------------------------------------------------------------------------------------------//

} // namespace

FileType DetectFileType(const std::string& path) {
	std::size_t dot = path.find_last_of('.');
	std::string extension = dot == std::string::npos ? "" : Lowercase(path.substr(dot + 1));
	if (extension == "c" || extension == "cc" || extension == "cpp" || extension == "cxx"
		|| extension == "h" || extension == "hh" || extension == "hpp") {
		return FileType::Cpp;
	}
	if (extension == "html" || extension == "htm") {
		return FileType::Html;
	}
	if (extension == "md" || extension == "markdown") {
		return FileType::Markdown;
	}
	if (extension == "sh" || extension == "bash") {
		return FileType::Shell;
	}
	return FileType::Plain;
}
//---------------------------------------------------------------------------------------------------------------------------------//

FileType ParseFileType(const std::string& name) {
	std::string type = Lowercase(name);
	if (type == "c" || type == "cpp" || type == "c++") {
		return FileType::Cpp;
	}
	if (type == "html") {
		return FileType::Html;
	}
	if (type == "md" || type == "markdown") {
		return FileType::Markdown;
	}
	if (type == "sh" || type == "shell" || type == "bash") {
		return FileType::Shell;
	}
	return FileType::Plain;
}
//---------------------------------------------------------------------------------------------------------------------------------//

bool FormatText(const std::string& input, FileType fileType, std::string& output, std::string& error) {
	std::string normalised;
	normalised.reserve(input.size());
	for (std::size_t index = 0; index < input.size(); ++index) {
		if (input[index] == '\r') {
			if (index + 1 < input.size() && input[index + 1] == '\n') {
				continue;
			}
			normalised.push_back('\n');
		} else {
			normalised.push_back(input[index]);
		}
	}

	BashParseInfo bashParseInfo;
	if (fileType == FileType::Shell && !AnalyseBash(normalised, bashParseInfo, error)) {
		return false;
	}

	std::istringstream stream(normalised);
	output.clear();
	std::string line;
	int shellIndentation = 0;
	std::vector<int> caseIndentations;
	std::size_t lineNumber = 0;
	while (std::getline(stream, line)) {
		if (fileType == FileType::Shell && lineNumber < bashParseInfo.protectedLines.size()
			&& bashParseInfo.protectedLines[lineNumber]) {
			output += line + '\n';
			++lineNumber;
			continue;
		}
		line = TrimTrailingWhitespace(line);
		if (fileType != FileType::Plain && fileType != FileType::Markdown) {
			line = ConvertLeadingSpacesToTabs(line);
		}
		if (fileType == FileType::Cpp) {
			line = TidyCppLine(line);
		} else if (fileType == FileType::Html) {
			line = TidyHtmlLine(line);
		} else if (fileType == FileType::Markdown) {
			line = TidyMarkdownLine(line);
		} else if (fileType == FileType::Shell) {
			line = TidyShellLine(line);
			std::size_t content = line.find_first_not_of("\t ");
			if (content != std::string::npos && line.compare(content, 2, "#!") != 0) {
				std::string shellContent = line.substr(content);
				bool caseTerminator = StartsWith(shellContent, ";;") || StartsWith(shellContent, ";&");
				bool casePattern = !caseIndentations.empty()
					&& shellIndentation == caseIndentations.back() + 1 && EndsWith(shellContent, ")");
				if (IsShellClosingLine(shellContent) && shellIndentation > 0) {
					--shellIndentation;
				}
				if (shellIndentation > 0 || IsShellClosingLine(shellContent) || IsShellOpeningLine(shellContent)) {
					line = std::string(static_cast<std::size_t>(shellIndentation), '\t') + shellContent;
				}
				if (StartsWith(shellContent, "case ") && EndsWith(shellContent, " in")) {
					caseIndentations.push_back(shellIndentation);
				}
				if (IsShellOpeningLine(shellContent)) {
					++shellIndentation;
				}
				if (casePattern) {
					shellIndentation = caseIndentations.back() + 2;
				} else if (caseTerminator && !caseIndentations.empty()) {
					shellIndentation = caseIndentations.back() + 1;
				} else if (StartsWith(shellContent, "esac") && !caseIndentations.empty()) {
					caseIndentations.pop_back();
				}
			}
		}
		output += line + '\n';
		++lineNumber;
	}

	while (output.size() > 1 && output.back() == '\n' && output[output.size() - 2] == '\n') {
		output.pop_back();
	}
	if (output.empty()) {
		output = "\n";
	}
	if (fileType == FileType::Shell) {
		BashParseInfo formattedParseInfo;
		std::string formattedError;
		if (!AnalyseBash(output, formattedParseInfo, formattedError)) {
			error = "formatter produced invalid Bash: " + formattedError;
			return false;
		}
	}
	return true;
}
//---------------------------------------------------------------------------------------------------------------------------------//

std::string TidyText(const std::string& input, FileType fileType) {
	std::string output;
	std::string error;
	if (!FormatText(input, fileType, output, error)) {
		return input;
	}
	return output;
}
//---------------------------------------------------------------------------------------------------------------------------------//
