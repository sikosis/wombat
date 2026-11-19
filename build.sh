#!/bin/bash

clear
cd '/boot/home/projects/wombat' || exit 1

if ! command -v hum >/dev/null 2>&1; then
	echo "This build file requires hum." >&2
	exit 127
fi

if [ ! -x /boot/home/projects/ttfx-haiku/ttfx ]; then
	echo "This build file requires TTFX at /boot/home/projects/ttfx-haiku/ttfx." >&2
	exit 127
fi

echo "
▗▖ ▗▖ ▗▄▖ ▗▖  ▗▖▗▄▄▖  ▗▄▖▗▄▄▄▖
▐▌ ▐▌▐▌ ▐▌▐▛▚▞▜▌▐▌ ▐▌▐▌ ▐▌ █
▐▌ ▐▌▐▌ ▐▌▐▌  ▐▌▐▛▀▚▖▐▛▀▜▌ █
▐▙█▟▌▝▚▄▞▘▐▌  ▐▌▐▙▄▞▘▐▌ ▐▌ █
	Builder v0.1 " | /boot/home/projects/ttfx-haiku/ttfx --random-effect
echo ""

if hum confirm "Continue?"; then

	if [ -x /boot/home/config/non-packaged/apps/Shinko ]; then
		/boot/home/config/non-packaged/apps/Shinko "$PWD" &
		shinko_team=$!
		trap '/boot/home/config/non-packaged/apps/Shinko --quit "$shinko_team"' EXIT
	fi

	hum style --foreground "#ffcad4" --bold "==> Step 1: Update Code"
	hum spin --title "Updating..." -- git pull
	git show --name-status --oneline

	hum style --foreground "#f4acb7" --bold "==> Step 2: Make Clean"
	make clean

	hum style --foreground "#9d8189" --bold "==> Step 3: Make"
	make

	hum style --foreground "#7d4e57" --bold "==> Step 4: Make Install"
	make install

	echo ""
	hum style --foreground "#5c374c" --bold "==> Step 5: Run / Test"
	/boot/system/non-packaged/bin/wombat
	# ./build/wombat --help

fi
