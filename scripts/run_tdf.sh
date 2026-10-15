#!/bin/bash
# LOC TDFの既定セット。s27_tdf_listなどの設定名も指定可能。
set -u
if [ "$#" -eq 0 ]; then
    set -- s9234 s35932 s38417 b14 b17
fi
exec "$(dirname "$0")/run.sh" tdf "$@"
