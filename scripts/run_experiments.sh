#!/bin/bash
# ISCAS SAFの既定セット。引数があれば設定名を置き換える。
set -u
if [ "$#" -eq 0 ]; then
    set -- s5378_C
fi
exec "$(dirname "$0")/run.sh" saf "$@"
