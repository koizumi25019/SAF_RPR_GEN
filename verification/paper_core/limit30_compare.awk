BEGIN {
    FS = ","
    print "net_name,f_type,xid_complete,core_complete,xid_fdp,core_fdp" > details
}
FNR == 1 { next }
$4 != "0" && $4 != "1" { next }  # Equivalent-fault echo rows are not representatives.
FILENAME == ARGV[1] {
    key = $1 SUBSEP $2
    xc[key] = $4; xp[key] = $5
    next
}
{
    key = $1 SUBSEP $2
    cc[key] = $4; cp[key] = $5
}
END {
    for (key in xc) {
        if (!(key in cc)) { missing++; continue }
        split(key, name, SUBSEP)
        printf "%s,%s,%s,%s,%s,%s\n", name[1], name[2], xc[key], cc[key], xp[key], cp[key] > details
        if (xc[key] == "1" && cc[key] == "1") {
            both++
            if (xp[key] != cp[key]) mismatches++
        } else {
            if (cc[key] == "1") core_only++
            else if (xc[key] == "1") xid_only++
            else neither++
            if (cp[key] + 0 > xp[key] + 0) core_higher++
            else if (cp[key] + 0 < xp[key] + 0) core_lower++
            else equal++
        }
    }
    for (key in cc) if (!(key in xc)) missing++
    close(details)
    printf "%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", circuit, repetition, \
           both, core_only, xid_only, neither, core_higher, core_lower, equal, mismatches, missing
    if (missing || mismatches) exit 1
}
