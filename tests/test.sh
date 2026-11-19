#!/bin/sh

set -eu

wombat=${WOMBAT:-build/wombat}
temporary_directory=$(mktemp -d "${TMPDIR:-/tmp}/wombat-test.XXXXXX")
trap 'rm -rf "$temporary_directory"' EXIT HUP INT TERM

printf 'if(true) {  \r\n    return 0;\t\r\n}\r\n\r\n' > "$temporary_directory/sample.cpp"
printf 'void Add() {\n    values.push_back({\x27-\x27, 1});\n}\n' > "$temporary_directory/initializer.cpp"
printf '#Heading \n-item\n' > "$temporary_directory/sample.md"
printf '#!/bin/bash\nif [ "$name" != "Wombat" ];then\nprintf "hello"|sed "s/h/H/"&&echo done\nelse\n        echo "a|b" # keep quoted pipe\nfi\n' > "$temporary_directory/sample.sh"
printf '#!/bin/sh\nif [ -n "$value" ];then\necho yes\nfi\n' > "$temporary_directory/extensionless"

write_output=$("$wombat" --write "$temporary_directory/sample.cpp" "$temporary_directory/initializer.cpp" "$temporary_directory/sample.md" "$temporary_directory/sample.sh" "$temporary_directory/extensionless")

expected_cpp='if (true) {
	return 0;
}'
expected_initializer='void Add() {
	values.push_back({'\''-'\'', 1});
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
[ "$(cat "$temporary_directory/initializer.cpp")" = "$expected_initializer" ]
[ "$(cat "$temporary_directory/sample.md")" = "$expected_markdown" ]
[ "$(cat "$temporary_directory/sample.sh")" = "$expected_shell" ]
[ "$(cat "$temporary_directory/extensionless")" = "$expected_extensionless" ]
[ "$(printf '%s\n' "$write_output" | grep -c '^formatted: ')" -eq 5 ]
check_output=$("$wombat" --check "$temporary_directory/sample.cpp" "$temporary_directory/initializer.cpp" "$temporary_directory/sample.md" "$temporary_directory/sample.sh" "$temporary_directory/extensionless")
[ "$(printf '%s\n' "$check_output" | grep -c '^clean: ')" -eq 5 ]

printf '%s\n' \
	'---' \
	'title:   Keep exact' \
	'---' \
	'' \
	'#Markdown' \
	'' \
	'Keep this hard break  ' \
	'and trim this one ' \
	'' \
	'>>Nested quote' \
	'1.first item' \
	'' \
	'|Name|Status|' \
	'|:---|---:|' \
	'|Wombat|Ready|' \
	'|Hum|Good|' \
	'' \
	'```bash' \
	'if[ "$name"="Wombat" ];then  ' \
	'echo "#Not a heading"' \
	'fi' \
	'```' > "$temporary_directory/comprehensive.md"
"$wombat" --write "$temporary_directory/comprehensive.md" >/dev/null
[ "$(sed -n '5p' "$temporary_directory/comprehensive.md")" = '# Markdown' ]
[ "$(sed -n '10p' "$temporary_directory/comprehensive.md")" = '> > Nested quote' ]
[ "$(sed -n '11p' "$temporary_directory/comprehensive.md")" = '1. first item' ]
grep -Fq '| Name   | Status |' "$temporary_directory/comprehensive.md"
grep -Fq '| :----- | -----: |' "$temporary_directory/comprehensive.md"
grep -Fq 'if[ "$name"="Wombat" ];then  ' "$temporary_directory/comprehensive.md"
[ "$(sed -n '7p' "$temporary_directory/comprehensive.md")" = 'Keep this hard break  ' ]
[ "$(sed -n '8p' "$temporary_directory/comprehensive.md")" = 'and trim this one' ]
"$wombat" --check "$temporary_directory/comprehensive.md" | grep -q '^clean: '

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
printf '%s\n' "$decline_output" | grep -q "skipped: $temporary_directory/prompt.sh"

accept_output=$(printf 'y\n' | "$wombat" "$temporary_directory/prompt.sh")
[ "$(sed -n '2p' "$temporary_directory/prompt.sh")" = 'if [ -n "$value" ]; then' ]
printf '%s\n' "$accept_output" | grep -q "formatted: $temporary_directory/prompt.sh"

printf '#!/bin/bash\necho one|cat\n' > "$temporary_directory/confirm-a.sh"
printf '#!/bin/bash\necho two|cat\n' > "$temporary_directory/confirm-b.sh"
multiple_output=$(printf 'y\nn\n' | "$wombat" "$temporary_directory/confirm-a.sh" "$temporary_directory/confirm-b.sh")
[ "$(sed -n '2p' "$temporary_directory/confirm-a.sh")" = 'echo one | cat' ]
[ "$(sed -n '2p' "$temporary_directory/confirm-b.sh")" = 'echo two|cat' ]
printf '%s\n' "$multiple_output" | grep -q "formatted: $temporary_directory/confirm-a.sh"
printf '%s\n' "$multiple_output" | grep -q "skipped: $temporary_directory/confirm-b.sh"

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
