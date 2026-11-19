#include "Formatter.h"
#include "Version.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

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

std::string RenderDiff(const std::string& path, const std::string& original, const std::string& formatted,
	bool colour) {
	std::ostringstream rendered;
	rendered << Coloured("--- " + path + " (original)", "\033[36m", colour) << '\n'
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
			rendered << Coloured("@@ ... @@", "\033[36m", colour) << '\n';
		}
		std::string line(1, diff[index].marker);
		line += diff[index].text;
		const char* lineColour = diff[index].marker == '-' ? "\033[31m"
			: (diff[index].marker == '+' ? "\033[32m" : "\033[2m");
		rendered << Coloured(line, lineColour, colour) << '\n';
		previousVisible = true;
	}
	return rendered.str();
}
//---------------------------------------------------------------------------------------------------------------------------------//

bool HasHum() {
	return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)
		&& std::system("command -v hum >/dev/null 2>&1") == 0;
}
//---------------------------------------------------------------------------------------------------------------------------------//

void ReviewDiff(const std::string& path, const std::string& original, const std::string& formatted,
	bool preferPager) {
	bool colour = SupportsColour(STDOUT_FILENO);
	std::string rendered = RenderDiff(path, original, formatted, colour);
	if (preferPager && HasHum()) {
		FILE* pager = popen("hum pager --show-line-numbers --no-soft-wrap", "w");
		if (pager != nullptr) {
			std::size_t written = std::fwrite(rendered.data(), 1, rendered.size(), pager);
			int result = pclose(pager);
			if (written == rendered.size() && result == 0) {
				return;
			}
		}
	}
	std::cout << rendered;
}
//---------------------------------------------------------------------------------------------------------------------------------//

void PrintStatus(const std::string& label, const std::string& path, const char* colour) {
	std::cout << Coloured(label, colour, SupportsColour(STDOUT_FILENO)) << ' ' << path << '\n';
}
//---------------------------------------------------------------------------------------------------------------------------------//

void PrintUsage() {
	std::cout << "Wombat " << WOMBAT_VERSION << "\n\n"
		<< "Usage: wombat [--write | --check] PATH...\n"
		<< "       wombat --stdin TYPE\n\n"
		<< "Options:\n"
		<< "  -w, --write   Tidy files or directories in place and report each result\n"
		<< "  -c, --check   Report whether files or directories are clean\n"
		<< "  --stdin TYPE   Tidy standard input as cpp, html, markdown, shell, or plain\n"
		<< "  -v, --version Show the Wombat version\n"
		<< "  -h, --help    Show this help\n";
}
//---------------------------------------------------------------------------------------------------------------------------------//

bool ReadFile(const std::string& path, std::string& contents) {
	struct stat fileInfo;
	if (stat(path.c_str(), &fileInfo) != 0) {
		std::cerr << "wombat: cannot inspect '" << path << "': " << std::strerror(errno) << '\n';
		return false;
	}
	if (S_ISDIR(fileInfo.st_mode)) {
		std::cerr << "wombat: '" << path << "' is a directory; recursive formatting is not implemented yet\n";
		return false;
	}
	if (!S_ISREG(fileInfo.st_mode)) {
		std::cerr << "wombat: '" << path << "' is not a regular file\n";
		return false;
	}

	try {
		std::ifstream file(path, std::ios::binary);
		if (!file) {
			std::cerr << "wombat: cannot read '" << path << "'\n";
			return false;
		}
		contents.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		if (file.bad()) {
			std::cerr << "wombat: failed while reading '" << path << "'\n";
			return false;
		}
	} catch (const std::ios_base::failure& exception) {
		std::cerr << "wombat: cannot read '" << path << "': " << exception.what() << '\n';
		return false;
	}
	return true;
}
//---------------------------------------------------------------------------------------------------------------------------------//

bool IsSkippedDirectory(const std::string& name) {
	static const std::vector<std::string> skippedDirectories = {
		".git", ".hg", ".svn", "build", "node_modules", "third_party", "vendor"
	};
	return std::find(skippedDirectories.begin(), skippedDirectories.end(), name) != skippedDirectories.end();
}
//---------------------------------------------------------------------------------------------------------------------------------//

bool HasShellShebang(const std::string& path) {
	try {
		std::ifstream file(path, std::ios::binary);
		std::string firstLine;
		if (!file || !std::getline(file, firstLine) || firstLine.compare(0, 2, "#!") != 0) {
			return false;
		}
		return firstLine.find("/bash") != std::string::npos || firstLine.find("/sh") != std::string::npos
			|| firstLine.find(" sh") != std::string::npos || firstLine.find(" bash") != std::string::npos;
	} catch (const std::ios_base::failure&) {
		return false;
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//

bool IsRecursiveCandidate(const std::string& path) {
	return DetectFileType(path) != FileType::Plain || HasShellShebang(path);
}
//---------------------------------------------------------------------------------------------------------------------------------//

bool CollectInputPath(const std::string& path, bool fromDirectory, std::vector<std::string>& paths) {
	struct stat fileInfo;
	if (lstat(path.c_str(), &fileInfo) != 0) {
		std::cerr << "wombat: cannot inspect '" << path << "': " << std::strerror(errno) << '\n';
		return false;
	}
	if (S_ISLNK(fileInfo.st_mode)) {
		if (!fromDirectory) {
			std::cerr << "wombat: refusing symbolic link '" << path << "'\n";
			return false;
		}
		return true;
	}
	if (S_ISREG(fileInfo.st_mode)) {
		if (!fromDirectory || IsRecursiveCandidate(path)) {
			paths.push_back(path);
		}
		return true;
	}
	if (!S_ISDIR(fileInfo.st_mode)) {
		std::cerr << "wombat: '" << path << "' is not a regular file or directory\n";
		return false;
	}

	DIR* directory = opendir(path.c_str());
	if (directory == nullptr) {
		std::cerr << "wombat: cannot open directory '" << path << "': " << std::strerror(errno) << '\n';
		return false;
	}
	std::vector<std::string> names;
	while (dirent* entry = readdir(directory)) {
		std::string name = entry->d_name;
		if (name != "." && name != ".." && !IsSkippedDirectory(name)) {
			names.push_back(name);
		}
	}
	closedir(directory);
	std::sort(names.begin(), names.end());

	bool success = true;
	for (const std::string& name : names) {
		std::string child = path == "/" ? path + name : path + "/" + name;
		success = CollectInputPath(child, true, paths) && success;
	}
	return success;
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
	if (HasHum()) {
		return std::system("hum confirm 'Apply changes to this file?'") == 0;
	}

	std::cout << "Apply changes to this file? [y/N] " << std::flush;
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
		if (argument == "-v" || argument == "--version") {
			std::cout << "wombat " << WOMBAT_VERSION << '\n';
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
	std::vector<std::string> requestedPaths = paths;
	paths.clear();
	bool pathFailed = false;
	for (const std::string& path : requestedPaths) {
		pathFailed = !CollectInputPath(path, false, paths) || pathFailed;
	}
	std::sort(paths.begin(), paths.end());
	paths.erase(std::unique(paths.begin(), paths.end()), paths.end());
	if (paths.empty()) {
		std::cerr << "wombat: no supported files found\n";
		return 2;
	}

	bool failed = pathFailed;
	bool changed = false;
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
			ReviewDiff(path, input, output, paths.size() > 1);
			if (ConfirmChanges()) {
				if (WriteFile(path, output)) {
					PrintStatus("formatted:", path, "\033[32m");
				} else {
					failed = true;
				}
			} else {
				PrintStatus("skipped:", path, "\033[33m");
			}
		}
	}

	return failed ? 2 : (checkFiles && changed ? 1 : 0);
}
//---------------------------------------------------------------------------------------------------------------------------------//
