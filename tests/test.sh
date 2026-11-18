#!/bin/sh

set -eu

wombat=${WOMBAT:-build/wombat}
temporary_directory=$(mktemp -d "${TMPDIR:-/tmp}/wombat-test.XXXXXX")
trap 'rm -rf "$temporary_directory"' EXIT HUP INT TERM

printf 'if(true) {  \r\n    return 0;\t\r\n}\r\n\r\n' > "$temporary_directory/sample.cpp"
printf '#Heading  \n-item\n' > "$temporary_directory/sample.md"

"$wombat" --write "$temporary_directory/sample.cpp" "$temporary_directory/sample.md"

expected_cpp='if (true) {
	return 0;
}'
expected_markdown='# Heading
- item'

[ "$(cat "$temporary_directory/sample.cpp")" = "$expected_cpp" ]
[ "$(cat "$temporary_directory/sample.md")" = "$expected_markdown" ]
"$wombat" --check "$temporary_directory/sample.cpp" "$temporary_directory/sample.md"

printf 'if(false) {}  ' > "$temporary_directory/untidy.cpp"
if "$wombat" --check "$temporary_directory/untidy.cpp" >/dev/null; then
	echo "check mode did not detect an untidy file" >&2
	exit 1
fi

printf 'Wombat tests passed.\n'
