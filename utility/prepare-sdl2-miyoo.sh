#!/bin/bash
set -euo pipefail

destination="${1:?usage: utility/prepare-sdl2-miyoo.sh <destination>}"
repo_url="${SDL2_MIYOO_REPO:-https://github.com/XK9274/sdl2_miyoo.git}"
ref="${SDL2_MIYOO_REF:-main}"

if [ -e "$destination" ] && ! git -C "$destination" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    echo "ERROR: Refusing to replace non-Git path: $destination" >&2
    exit 1
fi

if [ ! -e "$destination" ]; then
    echo "Cloning sdl2_miyoo from $repo_url..."
    git clone --depth 1 "$repo_url" "$destination"
fi

git -C "$destination" remote set-url origin "$repo_url"
git -C "$destination" fetch --depth 1 origin "$ref"
git -C "$destination" reset --hard FETCH_HEAD
git -C "$destination" clean -fdx

if [ ! -f "$destination/build-scripts/mk_miyoo.sh" ] || [ ! -f "$destination/libEGL.so" ] || [ ! -f "$destination/libGLESv2.so" ]; then
    echo "ERROR: sdl2_miyoo checkout is incomplete: $destination" >&2
    exit 1
fi

commit="$(git -C "$destination" rev-parse HEAD)"
echo "sdl2_miyoo ready at $commit ($repo_url, ref $ref)"
