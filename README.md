# Wombat

<p align="center">
  <img src="assets/wombat-icon-v2.png" alt="Wombat mascot tidying code" width="320">
</p>

Wombat is a quick, conservative source-code tidy tool for Haiku. It cleans up
common whitespace problems without trying to rewrite the structure of a file.

## Build

```sh
make
```

The resulting executable is `build/wombat`.

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

Wombat currently gives Bash and POSIX shell scripts the most complete
formatting pass. It tidies indentation for `if`, `else`, `for`, `while`, and
function blocks; spaces test expressions, pipelines, command separators, and
logical operators; and leaves quoted strings and comments intact.

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
and asks whether to apply it. When Haiku's `hum` is installed and the command
is running in a terminal, Wombat uses its interactive confirmation selector.
It falls back to a built-in `y/N` prompt when `hum` is unavailable.

Wombat deliberately avoids parsing or rearranging code. This keeps it useful
for quick cleanup while reducing the chance of changing program behaviour.
