#!/bin/sh

set -eu

script_directory=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repository_directory=$(dirname -- "$script_directory")
runtime_header="$repository_directory/third_party/tree-sitter/lib/include/tree_sitter/api.h"
bash_parser="$repository_directory/third_party/tree-sitter-bash/src/parser.c"

if [ -f "$runtime_header" ] && [ -f "$bash_parser" ]; then
	exit 0
fi

if ! command -v git >/dev/null 2>&1; then
	echo "error: Wombat's parser dependencies are missing and Git is unavailable" >&2
	echo "Clone Wombat with --recurse-submodules, then build again." >&2
	exit 1
fi

if ! git -C "$repository_directory" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
	echo "error: Wombat's parser dependencies are missing from this source archive" >&2
	echo "Use a recursive Git clone or include the third_party submodules in the archive." >&2
	exit 1
fi

echo "Initialising Wombat's Tree-sitter dependencies..."
git -C "$repository_directory" submodule update --init --recursive

if [ ! -f "$runtime_header" ] || [ ! -f "$bash_parser" ]; then
	echo "error: Tree-sitter dependencies could not be initialised" >&2
	exit 1
fi
