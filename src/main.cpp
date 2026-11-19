#include "Formatter.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

namespace {

struct PendingChange {
	std::string path;
	std::string contents;
};

struct DiffLine {
	char marker;
	std::string text;
};

bool SupportsColour(int descriptor) {
	const char* terminal = std::getenv("TERM");
	return isatty(descriptor) && std::getenv("NO_COLOR") == nullptr
		&& (terminal == nullptr || std::string(terminal) != "dumb");
}
//---------------------------------------------------------------------------------------------------------------------------------//

std::string Coloured(const std::string& text, const char* colour, bool enabled) {
	return enabled ? std::string(colour) + text + "\033[0m" : text;
}
//---------------------------------------------------------------------------------------------------------------------------------//

std::vector<std::string> SplitLines(const std::string& text) {
	std::vector<std::string> lines;
	std::istringstream stream(text);
	std::string line;
	while (std::getline(stream, line)) {
		lines.push_back(line);
	}
	return lines;
}
//---------------------------------------------------------------------------------------------------------------------------------//

std::vector<DiffLine> BuildDiff(const std::string& original, const std::string& formatted) {
	std::vector<std::string> oldLines = SplitLines(original);
	std::vector<std::string> newLines = SplitLines(formatted);
	std::vector<DiffLine> diff;
	if (oldLines.size() * newLines.size() > 2000000) {
		for (const std::string& line : oldLines) {
			diff.push_back({'-', line});
		}
		for (const std::string& line : newLines) {
			diff.push_back({'+', line});
		}
		return diff;
	}

	std::size_t columns = newLines.size() + 1;
	std::vector<unsigned int> lengths((oldLines.size() + 1) * columns, 0);
	for (std::size_t oldIndex = oldLines.size(); oldIndex-- > 0;) {
		for (std::size_t newIndex = newLines.size(); newIndex-- > 0;) {
			std::size_t position = oldIndex * columns + newIndex;
			if (oldLines[oldIndex] == newLines[newIndex]) {
				lengths[position] = lengths[(oldIndex + 1) * columns + newIndex + 1] + 1;
			} else {
				lengths[position] = std::max(lengths[(oldIndex + 1) * columns + newIndex],
					lengths[oldIndex * columns + newIndex + 1]);
			}
		}
	}

	std::size_t oldIndex = 0;
	std::size_t newIndex = 0;
	while (oldIndex < oldLines.size() || newIndex < newLines.size()) {
		if (oldIndex < oldLines.size() && newIndex < newLines.size()
			&& oldLines[oldIndex] == newLines[newIndex]) {
			diff.push_back({' ', oldLines[oldIndex]});
			++oldIndex;
			++newIndex;
		} else if (newIndex >= newLines.size() || (oldIndex < oldLines.size()
			&& lengths[(oldIndex + 1) * columns + newIndex] >= lengths[oldIndex * columns + newIndex + 1])) {
			diff.push_back({'-', oldLines[oldIndex++]});
		} else {
			diff.push_back({'+', newLines[newIndex++]});
		}
	}
	return diff;
}
//---------------------------------------------------------------------------------------------------------------------------------//

void PrintDiff(const std::string& path, const std::string& original, const std::string& formatted) {
	bool colour = SupportsColour(STDOUT_FILENO);
	std::cout << Coloured("--- " + path + " (original)", "\033[36m", colour) << '\n'
		<< Coloured("+++ " + path + " (formatted)", "\033[36m", colour) << '\n';
	std::vector<DiffLine> diff = BuildDiff(original, formatted);
	std::vector<bool> visible(diff.size(), false);
	for (std::size_t index = 0; index < diff.size(); ++index) {
		if (diff[index].marker == ' ') {
			continue;
		}
		std::size_t start = index > 2 ? index - 2 : 0;
		std::size_t end = std::min(diff.size(), index + 3);
		for (std::size_t context = start; context < end; ++context) {
			visible[context] = true;
		}
	}

	bool previousVisible = false;
	for (std::size_t index = 0; index < diff.size(); ++index) {
		if (!visible[index]) {
			previousVisible = false;
			continue;
		}
		if (!previousVisible && index > 0) {
			std::cout << Coloured("@@ ... @@", "\033[36m", colour) << '\n';
		}
		std::string line(1, diff[index].marker);
		line += diff[index].text;
		const char* lineColour = diff[index].marker == '-' ? "\033[31m"
			: (diff[index].marker == '+' ? "\033[32m" : "\033[2m");
		std::cout << Coloured(line, lineColour, colour) << '\n';
		previousVisible = true;
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//

void PrintStatus(const std::string& label, const std::string& path, const char* colour) {
	std::cout << Coloured(label, colour, SupportsColour(STDOUT_FILENO)) << ' ' << path << '\n';
}
//---------------------------------------------------------------------------------------------------------------------------------//

void PrintUsage() {
	std::cout << "Usage: wombat [--write | --check] FILE...\n"
		<< "       wombat --stdin TYPE\n\n"
		<< "Options:\n"
		<< "  -w, --write   Tidy files in place and report each result\n"
		<< "  -c, --check   Report whether each file is clean\n"
		<< "  --stdin TYPE   Tidy standard input as cpp, html, markdown, shell, or plain\n"
		<< "  -h, --help    Show this help\n";
}
//---------------------------------------------------------------------------------------------------------------------------------//

bool ReadFile(const std::string& path, std::string& contents) {
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		std::cerr << "wombat: cannot read '" << path << "'\n";
		return false;
	}
	contents.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	return true;
}
//---------------------------------------------------------------------------------------------------------------------------------//

bool WriteFile(const std::string& path, const std::string& contents) {
	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	if (!file || !file.write(contents.data(), static_cast<std::streamsize>(contents.size()))) {
		std::cerr << "wombat: cannot write '" << path << "'\n";
		return false;
	}
	return true;
}
//---------------------------------------------------------------------------------------------------------------------------------//

FileType FileTypeForContents(const std::string& path, const std::string& contents) {
	FileType fileType = DetectFileType(path);
	if (fileType != FileType::Plain || contents.compare(0, 2, "#!") != 0) {
		return fileType;
	}
	std::size_t firstLineEnd = contents.find('\n');
	std::string shebang = contents.substr(0, firstLineEnd);
	if (shebang.find("/bash") != std::string::npos || shebang.find("/sh") != std::string::npos
		|| shebang.find(" sh") != std::string::npos || shebang.find(" bash") != std::string::npos) {
		return FileType::Shell;
	}
	return fileType;
}
//---------------------------------------------------------------------------------------------------------------------------------//

bool ConfirmChanges() {
	if (isatty(STDIN_FILENO)) {
		if (std::system("command -v hum >/dev/null 2>&1") == 0) {
			return std::system("hum confirm 'Apply these formatting changes?'") == 0;
		}
	}

	std::cout << "Apply these formatting changes? [y/N] " << std::flush;
	std::string answer;
	if (!std::getline(std::cin, answer)) {
		std::cout << '\n';
		return false;
	}
	return answer == "y" || answer == "Y" || answer == "yes" || answer == "YES" || answer == "Yes";
}
//---------------------------------------------------------------------------------------------------------------------------------//

} // namespace

int main(int argc, char** argv) {
	bool writeFiles = false;
	bool checkFiles = false;
	std::vector<std::string> paths;

	for (int index = 1; index < argc; ++index) {
		std::string argument = argv[index];
		if (argument == "-h" || argument == "--help") {
			PrintUsage();
			return 0;
		}
		if (argument == "-w" || argument == "--write") {
			writeFiles = true;
		} else if (argument == "-c" || argument == "--check") {
			checkFiles = true;
		} else if (argument == "--stdin") {
			if (index + 1 >= argc) {
				std::cerr << "wombat: --stdin requires a file type\n";
				return 2;
			}
			std::string input{std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>()};
			std::string output;
			std::string error;
			if (!FormatText(input, ParseFileType(argv[++index]), output, error)) {
				std::cerr << "wombat: " << error << '\n';
				return 2;
			}
			std::cout << output;
			return 0;
		} else if (!argument.empty() && argument[0] == '-') {
			std::cerr << "wombat: unknown option '" << argument << "'\n";
			return 2;
		} else {
			paths.push_back(argument);
		}
	}

	if (writeFiles && checkFiles) {
		std::cerr << "wombat: --write and --check cannot be used together\n";
		return 2;
	}
	if (paths.empty()) {
		PrintUsage();
		return 2;
	}

	bool failed = false;
	bool changed = false;
	std::vector<PendingChange> pendingChanges;
	for (const std::string& path : paths) {
		std::string input;
		if (!ReadFile(path, input)) {
			failed = true;
			continue;
		}
		std::string output;
		std::string formattingError;
		if (!FormatText(input, FileTypeForContents(path, input), output, formattingError)) {
			std::cerr << "wombat: " << path << ": " << formattingError << '\n';
			failed = true;
			continue;
		}
		if (output == input) {
			if (writeFiles) {
				PrintStatus("unchanged:", path, "\033[33m");
			} else if (checkFiles) {
				PrintStatus("clean:", path, "\033[32m");
			} else {
				PrintStatus("unchanged:", path, "\033[33m");
			}
			continue;
		}
		changed = true;
		if (writeFiles) {
			if (WriteFile(path, output)) {
				PrintStatus("formatted:", path, "\033[32m");
			} else {
				failed = true;
			}
		} else if (checkFiles) {
			PrintStatus("needs formatting:", path, "\033[31m");
		} else {
			PrintDiff(path, input, output);
			pendingChanges.push_back({path, output});
		}
	}

	if (!writeFiles && !checkFiles && !pendingChanges.empty()) {
		if (ConfirmChanges()) {
			for (const PendingChange& change : pendingChanges) {
				if (WriteFile(change.path, change.contents)) {
					PrintStatus("formatted:", change.path, "\033[32m");
				} else {
					failed = true;
				}
			}
		} else {
			std::cout << "No files changed.\n";
		}
	}

	return failed ? 2 : (checkFiles && changed ? 1 : 0);
}
//---------------------------------------------------------------------------------------------------------------------------------//
