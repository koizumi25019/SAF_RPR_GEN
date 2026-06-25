#!/bin/bash
W=verification/xid_fvalue/progress_watch.log
cd /home/koizumi/fdp
: > "$W"
for i in $(seq 1 240); do   # 最大 ~2h (30s間隔)
  ts=$(date '+%T')
  cur=""
  for c in s13207_C s15850_C s35932_C s38417_C s38584_C; do
    p="verification/xid_fvalue/results/${c}_inline.progress"
    [ -f "$p" ] || continue
    last=$(tr '\r' '\n' < "$p" 2>/dev/null | tail -1)
    [ -n "$last" ] && cur="$c: $last"
  done
  echo "[$ts] $cur" >> "$W"
  grep -q ALLDONE verification/xid_fvalue/progress.log 2>/dev/null && { echo "[$ts] ALLDONE" >> "$W"; break; }
  sleep 30
done
