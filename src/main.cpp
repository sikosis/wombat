#include "Formatter.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {

void PrintUsage() {
	std::cout << "Usage: wombat [--write | --check] FILE...\n"
		<< "       wombat --stdin TYPE\n\n"
		<< "Options:\n"
		<< "  -w, --write   Tidy files in place\n"
		<< "  -c, --check   Report files that need tidying\n"
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
	for (const std::string& path : paths) {
		std::string input;
		if (!ReadFile(path, input)) {
			failed = true;
			continue;
		}
		std::string output = TidyText(input, DetectFileType(path));
		if (output == input) {
			continue;
		}
		changed = true;
		if (writeFiles) {
			failed = !WriteFile(path, output) || failed;
		} else if (checkFiles) {
			std::cout << path << '\n';
		} else {
			std::cout << output;
		}
	}

	return failed ? 2 : (checkFiles && changed ? 1 : 0);
}
//---------------------------------------------------------------------------------------------------------------------------------//
