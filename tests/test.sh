#!/bin/sh

set -eu

wombat=${WOMBAT:-build/wombat}
temporary_directory=$(mktemp -d "${TMPDIR:-/tmp}/wombat-test.XXXXXX")
trap 'rm -rf "$temporary_directory"' EXIT HUP INT TERM

printf 'if(true) {  \r\n    return 0;\t\r\n}\r\n\r\n' > "$temporary_directory/sample.cpp"
printf '#Heading  \n-item\n' > "$temporary_directory/sample.md"
printf '#!/bin/bash\nif[ "$name"!="Wombat" ];then\nprintf "hello"|sed "s/h/H/"&&echo done\nelse\n        echo "a|b" # keep quoted pipe\nfi\n' > "$temporary_directory/sample.sh"
printf '#!/bin/sh\nif[ -n "$value" ];then\necho yes\nfi\n' > "$temporary_directory/extensionless"

write_output=$("$wombat" --write "$temporary_directory/sample.cpp" "$temporary_directory/sample.md" "$temporary_directory/sample.sh" "$temporary_directory/extensionless")

expected_cpp='if (true) {
	return 0;
}'
expected_markdown='# Heading
- item'
expected_shell='#!/bin/bash
if [ "$name" != "Wombat" ]; then
	printf "hello" | sed "s/h/H/" && echo done
else
	echo "a|b" # keep quoted pipe
fi'
expected_extensionless='#!/bin/sh
if [ -n "$value" ]; then
	echo yes
fi'

[ "$(cat "$temporary_directory/sample.cpp")" = "$expected_cpp" ]
[ "$(cat "$temporary_directory/sample.md")" = "$expected_markdown" ]
[ "$(cat "$temporary_directory/sample.sh")" = "$expected_shell" ]
[ "$(cat "$temporary_directory/extensionless")" = "$expected_extensionless" ]
[ "$(printf '%s\n' "$write_output" | grep -c '^formatted: ')" -eq 4 ]
check_output=$("$wombat" --check "$temporary_directory/sample.cpp" "$temporary_directory/sample.md" "$temporary_directory/sample.sh" "$temporary_directory/extensionless")
[ "$(printf '%s\n' "$check_output" | grep -c '^clean: ')" -eq 4 ]

printf 'if(false) {}  ' > "$temporary_directory/untidy.cpp"
if untidy_output=$("$wombat" --check "$temporary_directory/untidy.cpp"); then
	echo "check mode did not detect an untidy file" >&2
	exit 1
fi
[ "$untidy_output" = "needs formatting: $temporary_directory/untidy.cpp" ]

printf 'Wombat tests passed.\n'
