#ifndef WOMBAT_FORMATTER_H
#define WOMBAT_FORMATTER_H

#include <string>

enum class FileType {
	Cpp,
	Html,
	Markdown,
	Shell,
	Plain
};

FileType DetectFileType(const std::string& path);
FileType ParseFileType(const std::string& name);
bool FormatText(const std::string& input, FileType fileType, std::string& output, std::string& error);
std::string TidyText(const std::string& input, FileType fileType);

#endif
