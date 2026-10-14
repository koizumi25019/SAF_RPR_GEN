#!/usr/bin/env bash
# Run the s5378 SAF experiment with the SB-XID build in build/main_release.
set -euo pipefail

repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
binary="$repo_dir/build/main_release"
if [[ ! -x "$binary" ]]; then
    printf 'Build build/main_release with clasp before running this script.\n' >&2
    exit 1
fi

mkdir -p -- "$repo_dir/output/sbxid/limit30/fdp" "$repo_dir/output/sbxid/limit30/log"
cd -- "$repo_dir/build"
exec "$binary" -set ../input/script/s5378_C_sbxid.set
