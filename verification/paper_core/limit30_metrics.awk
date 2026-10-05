BEGIN {
    FS = ","
    while ((getline line < logfile) > 0) {
        split(line, parts, ":")
        value = parts[2] + 0
        if (line ~ /^\/\/  CPU Time *:/) cpu = value
        if (line ~ /^\/\/  Time *:/) wall = value
        if (line ~ /CPU Time \(Don.t care\)/) dc = value
        if (line ~ /CPU Time \(CaDiCaL\)/) sat = value
        if (line ~ /CPU Time \(BDD\)/) bdd = value
        if (line ~ /Don't-care Method/) {
            expected = method == "core" ? "CORE" : "XID"
            if (index(line, expected)) found_method = 1
        }
        if (line ~ /Dominance Cube Reuse/ && line ~ /off/) found_nodom = 1
        if (line ~ /CORE Extra Verification/ && line ~ /off/) found_noverify = 1
    }
    close(logfile)
}
FNR > 1 && ($4 == "0" || $4 == "1") {
    faults++
    cubes += $3
    if ($4 == "1") complete++
    else incomplete++
}
END {
    if (!found_method || !found_nodom || !found_noverify || !faults) exit 1
    printf "%s,%s,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%d,%d\n", \
           circuit, method, repetition, cpu, wall, dc, sat, bdd, faults, complete, incomplete, cubes
}
