#!/bin/bash
# ISCAS SAFの既定セット。引数があれば設定名を置き換える。
set -u
if [ "$#" -eq 0 ]; then
    set -- s5378_C s9234_C s13207_C s15850_C s35932_C s38417_C s38584_C
fi
exec "$(dirname "$0")/run.sh" saf "$@"
