# Wombat

Wombat is a quick, conservative source-code tidy tool for Haiku. It cleans up
common whitespace problems without trying to rewrite the structure of a file.

## Build

```sh
make
```

The resulting executable is `build/wombat`.

## Usage

```sh
# Print a tidied file without changing it
build/wombat source.cpp

# Tidy one or more files in place
build/wombat --write source.cpp page.html README.md script.sh

# Report files that need tidying (exit status 1 when changes are needed)
build/wombat --check source.cpp

# Tidy standard input
cat source.cpp | build/wombat --stdin cpp
```

Wombat chooses a formatter from the filename extension. Supported file types
are C and C++, HTML, Markdown, and shell scripts. Unknown extensions still get
safe whitespace cleanup.

## Formatting rules

- Unix line endings and exactly one final newline.
- No trailing spaces or tabs.
- Leading groups of four spaces become tabs in code files.
- C and C++ control statements use a space before `(`.
- Markdown list markers and headings use consistent spacing.
- Shell keywords such as `if`, `for`, and `while` use consistent spacing.

Wombat deliberately avoids parsing or rearranging code. This keeps it useful
for quick cleanup while reducing the chance of changing program behaviour.

