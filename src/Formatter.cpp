#include "Formatter.h"

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
	return line;
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
	return line;
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
	return line;
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

std::string TidyText(const std::string& input, FileType fileType) {
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

	std::istringstream stream(normalised);
	std::string output;
	std::string line;
	while (std::getline(stream, line)) {
		line = TrimTrailingWhitespace(line);
		if (fileType != FileType::Plain && fileType != FileType::Markdown) {
			line = ConvertLeadingSpacesToTabs(line);
		}
		if (fileType == FileType::Cpp) {
			line = TidyCppLine(line);
		} else if (fileType == FileType::Markdown) {
			line = TidyMarkdownLine(line);
		} else if (fileType == FileType::Shell) {
			line = TidyShellLine(line);
		}
		output += line + '\n';
	}

	while (output.size() > 1 && output.back() == '\n' && output[output.size() - 2] == '\n') {
		output.pop_back();
	}
	return output.empty() ? "\n" : output;
}
//---------------------------------------------------------------------------------------------------------------------------------//
