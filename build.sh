#!/usr/bin/env bash
set -euo pipefail

script_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
buildbot_dir="${MM_BUILDBOT_DIR:-}"
buildbot_repo="${MM_BUILDBOT_REPO:-https://github.com/XK9274/mm-buildbot.git}"
buildbot_ref="${MM_BUILDBOT_REF:-main}"
cache_dir="${MM_BUILDBOT_CACHE_DIR:-$(dirname "$script_dir")/.mm-buildbot-cache/mm-buildbot}"
package_id="sdl2-benchmarks-mmiyoo"

usage() {
  cat <<EOF
Usage: $0 [OPTIONS]

Build the Miyoo SDL2 benchmark app through mm-buildbot.

Options:
  -d, --debug    Build with debug symbols and reduced optimisation
  -v, --verbose  Keep verbose build output enabled
  -h, --help     Show this help

Environment:
  MM_BUILDBOT_DIR    Existing mm-buildbot checkout to use
  MM_BUILDBOT_REPO   Buildbot repository URL (default: $buildbot_repo)
  MM_BUILDBOT_REF    Buildbot branch or ref (default: $buildbot_ref)
  MM_BUILDBOT_CACHE_DIR  Clone cache location
  MMIYOO_SDL2_PREFIX  Existing sdl2-mmiyoo-lib provider prefix
  MMIYOO_SDL2_ADDONS_PREFIX  Existing sdl2-mmiyoo-addons provider prefix
  TITLE_GIT_VERSION  Version embedded in the title binary
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -d|--debug)
      export DEBUG=1
      shift
      ;;
    -v|--verbose)
      export VERBOSE=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      printf 'Unknown option: %s\n' "$1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [[ -z "$buildbot_dir" && -d "$script_dir/../mm-buildbot/.git" ]]; then
  buildbot_dir="$script_dir/../mm-buildbot"
fi

if [[ -z "$buildbot_dir" ]]; then
  require_tool() {
    command -v "$1" >/dev/null 2>&1 || {
      printf 'Missing required tool: %s\n' "$1" >&2
      exit 1
    }
  }
  require_tool git

  if [[ ! -d "$cache_dir/.git" ]]; then
    mkdir -p "$(dirname "$cache_dir")"
    printf 'Cloning mm-buildbot (%s) into %s\n' "$buildbot_ref" "$cache_dir"
    git clone --branch "$buildbot_ref" "$buildbot_repo" "$cache_dir"
  else
    printf 'Updating cached mm-buildbot checkout (%s): %s\n' "$buildbot_ref" "$cache_dir"
    git -C "$cache_dir" fetch origin "$buildbot_ref"
    git -C "$cache_dir" checkout --force --detach FETCH_HEAD
    git -C "$cache_dir" clean -fdx
  fi
  buildbot_dir="$cache_dir"
fi

[[ -x "$buildbot_dir/scripts/build-package.sh" ]] || {
  printf 'Invalid mm-buildbot checkout: %s\n' "$buildbot_dir" >&2
  exit 1
}

buildbot_dir="$(CDPATH= cd -- "$buildbot_dir" && pwd)"
printf 'Building benchmark package through mm-buildbot at %s\n' "$buildbot_dir"

BENCHMARK_SOURCE_DIR="$script_dir" \
  "$buildbot_dir/scripts/build-package.sh" "$package_id"

artifact="$buildbot_dir/artifacts/$package_id.zip"
[[ -f "$artifact" ]] || {
  printf 'Buildbot did not produce the expected artifact: %s\n' "$artifact" >&2
  exit 1
}

command -v unzip >/dev/null 2>&1 || {
  printf 'Missing required tool: unzip\n' >&2
  exit 1
}

printf 'Refreshing generated app-dist output\n'
rm -rf "$script_dir/app-dist/sdl_bench/bin" "$script_dir/app-dist/sdl_bench/lib"
mkdir -p "$script_dir/app-dist"
unzip -q -o "$artifact" -d "$script_dir/app-dist"

printf 'Build complete: %s/app-dist/sdl_bench\n' "$script_dir"
