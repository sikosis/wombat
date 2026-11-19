# Wombat

<p align="center">
  <img src="assets/wombat-icon-v2.png" alt="Wombat mascot tidying code" width="320">
</p>

Wombat is a quick, conservative source-code tidy tool for Haiku. It cleans up
common whitespace problems without trying to rewrite the structure of a file.

## Build

Clone Wombat with its parser dependencies, then build it:

```sh
git clone --recurse-submodules <wombat-repository-url>
cd wombat
make
```

The resulting executable is `build/wombat`.

For an existing checkout, initialise the dependencies before building:

```sh
git submodule update --init --recursive
```

## Install

Install Wombat into Haiku's non-packaged command directory:

```sh
make install
```

The default destination is `/boot/system/non-packaged/bin/wombat`. `PREFIX`,
`BINDIR`, and `DESTDIR` can be overridden for custom installations or package
staging:

```sh
make install PREFIX=/custom/prefix
make install DESTDIR=/temporary/package-root
```

## Usage

```sh
# Preview a tidied file, then confirm whether to apply the changes
build/wombat source.cpp

# Tidy one or more files in place
build/wombat --write source.cpp page.html README.md script.sh

# Report whether files are clean (exit status 1 when changes are needed)
build/wombat --check source.cpp

# Tidy standard input
cat source.cpp | build/wombat --stdin cpp
```

Wombat chooses a formatter from the filename extension. Supported file types
are C and C++, HTML, Markdown, and shell scripts. Unknown extensions still get
safe whitespace cleanup. Extensionless files with a Bash or POSIX shell
shebang are recognised as shell scripts.

### Bash formatting

Wombat uses the Tree-sitter Bash grammar to parse every Bash and POSIX shell
script before and after formatting. It understands functions, conditionals,
loops, `case`, arrays, arithmetic, command and process substitutions, tests,
redirects, and heredocs. Syntax-sensitive heredoc content is preserved exactly.

Formatting is transactional: if the original script has a syntax error, or if
the formatted result does not parse, Wombat reports the location and refuses
to write the file. It tidies block indentation, pipelines, command separators,
logical operators, trailing whitespace, and line endings while preserving
quoted strings, comments, test expressions, array subscripts, and heredocs.

## Formatting rules

- Unix line endings and exactly one final newline.
- No trailing spaces or tabs.
- Leading groups of four spaces become tabs in code files.
- C and C++ control statements, operators, commas, semicolons, and opening
  braces use consistent spacing outside strings and comments.
- HTML attributes have consistent spacing around `=`.
- Markdown list markers, block quotes, numbered lists, and headings use
  consistent spacing.
- Shell keywords, pipelines, command lists, and logical operators use
  consistent spacing outside quotes and comments.

Write mode reports `formatted:` or `unchanged:` for every file. Check mode
reports `needs formatting:` or `clean:` so a successful command never looks
like it silently did nothing. With no mode option, Wombat previews the result
as a colour-coded diff: original lines are red, formatted lines are green, and
file headers are cyan. Colour is disabled automatically when output is piped,
when the terminal is unsupported, or when `NO_COLOR` is set. Wombat then asks
whether to apply the changes. When Haiku's `hum` is installed and the command
is running in a terminal, Wombat uses its interactive confirmation selector;
otherwise it falls back to a built-in `y/N` prompt.

Wombat uses parsing to understand Bash structure, but deliberately avoids
rewriting commands or expressions. This reduces the chance of changing program
behaviour while still providing useful, consistent formatting.

## Third-party components

Wombat statically builds the MIT-licensed Tree-sitter runtime and official
Tree-sitter Bash grammar from pinned Git submodules. See
[`THIRD_PARTY.md`](THIRD_PARTY.md) for versions and licence links.
