#!/bin/bash

set -euo pipefail

declare -A colours=([sky]="blue" [leaf]="green")
items=("one" "two words" "three")
counter=0

cleanup() {
rm -f -- "${temporary_file:-}"
}

trap cleanup EXIT HUP INT TERM

for item in "${items[@]}";do
if [[ $item =~ ^two ]];then
printf '%s\n' "$item"|sed 's/two/TWO/'
elif (( counter > 2 ));then
break
else
((counter += 1))
fi
done

case ${colours[sky]} in
blue|cyan)
echo "cool"&&echo "colour"
;;
*)
echo "unknown"
;;
esac

while read -r line;do
printf '%s\n' "${line^^}"
done < <(printf '%s\n' "${items[@]}")

result=$(printf '%s\n' "${items[*]}"|tr ' ' ':')

cat <<'LITERAL'
  Keep these spaces | && ; exactly.
    ${this_must_not_expand}
LITERAL

cat <<-TABBED
	Tabbed heredoc content
	TABBED

printf '%s\n' "$result"
