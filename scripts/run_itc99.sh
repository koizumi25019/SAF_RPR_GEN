#!/bin/bash
# ITC'99 SAFの既定セット。引数があれば設定名を置き換える。
set -u
if [ "$#" -eq 0 ]; then
    set -- b14 b17
fi
exec "$(dirname "$0")/run.sh" saf "$@"
