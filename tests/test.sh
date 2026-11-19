#!/bin/sh

set -eu

wombat=${WOMBAT:-build/wombat}
temporary_directory=$(mktemp -d "${TMPDIR:-/tmp}/wombat-test.XXXXXX")
trap 'rm -rf "$temporary_directory"' EXIT HUP INT TERM

printf 'if(true) {  \r\n    return 0;\t\r\n}\r\n\r\n' > "$temporary_directory/sample.cpp"
printf '#Heading  \n-item\n' > "$temporary_directory/sample.md"
printf '#!/bin/bash\nif [ "$name" != "Wombat" ];then\nprintf "hello"|sed "s/h/H/"&&echo done\nelse\n        echo "a|b" # keep quoted pipe\nfi\n' > "$temporary_directory/sample.sh"
printf '#!/bin/sh\nif [ -n "$value" ];then\necho yes\nfi\n' > "$temporary_directory/extensionless"

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

printf '#!/bin/bash\nif [ -n "$value" ];then\necho yes\nfi\n' > "$temporary_directory/prompt.sh"
decline_output=$(printf 'n\n' | "$wombat" "$temporary_directory/prompt.sh")
[ "$(sed -n '2p' "$temporary_directory/prompt.sh")" = 'if [ -n "$value" ];then' ]
printf '%s\n' "$decline_output" | grep -q "^--- $temporary_directory/prompt.sh (original)$"
printf '%s\n' "$decline_output" | grep -q "^+++ $temporary_directory/prompt.sh (formatted)$"
printf '%s\n' "$decline_output" | grep -q '^-if \[ -n "\$value" \];then$'
printf '%s\n' "$decline_output" | grep -q '^+if \[ -n "\$value" \]; then$'
printf '%s\n' "$decline_output" | grep -q 'No files changed.'

accept_output=$(printf 'y\n' | "$wombat" "$temporary_directory/prompt.sh")
[ "$(sed -n '2p' "$temporary_directory/prompt.sh")" = 'if [ -n "$value" ]; then' ]
printf '%s\n' "$accept_output" | grep -q "formatted: $temporary_directory/prompt.sh"

cp tests/fixtures/bash-comprehensive.bash "$temporary_directory/comprehensive.bash"
"$wombat" --write "$temporary_directory/comprehensive.bash" >/dev/null
"$wombat" --check "$temporary_directory/comprehensive.bash" | grep -q '^clean: '
grep -Fq '  Keep these spaces | && ; exactly.' "$temporary_directory/comprehensive.bash"
grep -Fq '    ${this_must_not_expand}' "$temporary_directory/comprehensive.bash"
grep -Fq '[[ $item =~ ^two ]]' "$temporary_directory/comprehensive.bash"
grep -Fq 'done < <(printf' "$temporary_directory/comprehensive.bash"
grep -Fq 'colours=([sky]="blue" [leaf]="green")' "$temporary_directory/comprehensive.bash"
grep -Fq '${colours[sky]}' "$temporary_directory/comprehensive.bash"

printf '#!/bin/bash\nif true; then\necho broken\n' > "$temporary_directory/invalid.bash"
cp "$temporary_directory/invalid.bash" "$temporary_directory/invalid-original.bash"
if "$wombat" --write "$temporary_directory/invalid.bash" >/dev/null 2>&1; then
	echo "invalid Bash was not rejected" >&2
	exit 1
fi
cmp "$temporary_directory/invalid.bash" "$temporary_directory/invalid-original.bash"

mkdir -p "$temporary_directory/directory/nested" "$temporary_directory/directory/build"
printf '#!/bin/bash\nif true;then\necho recursive\nfi\n' > "$temporary_directory/directory/nested/script"
printf '#!/bin/bash\nif true;then\necho ignored\nfi\n' > "$temporary_directory/directory/build/ignored.sh"
printf 'plain text  ' > "$temporary_directory/directory/notes.txt"
"$wombat" --write "$temporary_directory/directory" | grep -q 'formatted: .*nested/script'
[ "$(sed -n '2p' "$temporary_directory/directory/nested/script")" = 'if true; then' ]
[ "$(sed -n '2p' "$temporary_directory/directory/build/ignored.sh")" = 'if true;then' ]
[ "$(cat "$temporary_directory/directory/notes.txt")" = 'plain text  ' ]
"$wombat" --check "$temporary_directory/directory" | grep -q 'clean: .*nested/script'

printf 'Wombat tests passed.\n'
