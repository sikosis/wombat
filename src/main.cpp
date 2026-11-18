#include "Formatter.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <unistd.h>

namespace {

struct PendingChange {
	std::string path;
	std::string contents;
};

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
			std::cout << TidyText(input, ParseFileType(argv[++index]));
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
		std::string output = TidyText(input, FileTypeForContents(path, input));
		if (output == input) {
			if (writeFiles) {
				std::cout << "unchanged: " << path << '\n';
			} else if (checkFiles) {
				std::cout << "clean: " << path << '\n';
			} else {
				std::cout << "unchanged: " << path << '\n';
			}
			continue;
		}
		changed = true;
		if (writeFiles) {
			if (WriteFile(path, output)) {
				std::cout << "formatted: " << path << '\n';
			} else {
				failed = true;
			}
		} else if (checkFiles) {
			std::cout << "needs formatting: " << path << '\n';
		} else {
			if (paths.size() > 1) {
				std::cout << "--- " << path << " (formatted preview) ---\n";
			}
			std::cout << output;
			pendingChanges.push_back({path, output});
		}
	}

	if (!writeFiles && !checkFiles && !pendingChanges.empty()) {
		if (ConfirmChanges()) {
			for (const PendingChange& change : pendingChanges) {
				if (WriteFile(change.path, change.contents)) {
					std::cout << "formatted: " << change.path << '\n';
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
