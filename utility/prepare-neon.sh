#!/bin/bash
set -euo pipefail

destination="${1:?usage: utility/prepare-neon.sh <destination>}"
repo_url="${NEON_ARM_REPO_URL:-https://github.com/XK9274/neon-arm-library-miyoo.git}"
ref="${NEON_ARM_REF:-main}"

if [ -e "$destination" ] && ! git -C "$destination" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    echo "ERROR: Refusing to replace non-Git path: $destination" >&2
    exit 1
fi

if [ ! -e "$destination" ]; then
    echo "Cloning NEON ARM library from $repo_url..."
    git clone --depth 1 "$repo_url" "$destination"
fi

git -C "$destination" remote set-url origin "$repo_url"
git -C "$destination" fetch --depth 1 origin "$ref"
git -C "$destination" reset --hard FETCH_HEAD
git -C "$destination" clean -fdx

if [ ! -f "$destination/include/neon.h" ] || [ ! -f "$destination/makefile" ]; then
    echo "ERROR: NEON ARM library checkout is incomplete: $destination" >&2
    exit 1
fi

commit="$(git -C "$destination" rev-parse HEAD)"
echo "NEON ARM library ready at $commit ($repo_url, ref $ref)"
