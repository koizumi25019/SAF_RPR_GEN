#!/bin/bash
# b15 SAF専用のショートカット。
set -u
exec "$(dirname "$0")/run.sh" saf b15
