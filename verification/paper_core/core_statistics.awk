# Aggregate PAPER_CORE stderr from a run with -core_verify off.
# One deletion trial is made for each literal in the initial failed core.
$1 == "[PAPER_CORE]" {
    split($2, a, "="); cubes = a[2] + 0
    split($3, a, "="); solves = a[2] + 0
    split($5, a, "="); inputs = a[2] + 0
    split($6, a, "="); core = a[2] + 0
    split($7, a, "="); prime = a[2] + 0
    found++
}
END {
    if (found != 1 || !cubes || !inputs || prime > core || core > inputs ||
        solves != 2 * cubes + core) {
        print "Invalid CORE statistics or extra verification enabled: " FILENAME > "/dev/stderr"
        exit 1
    }
    printf "%s,%d,%d,%d,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n", \
        circuit, repetition, cubes, inputs, core, prime, solves, core, core / cubes, prime / cubes, \
        100 * (1 - core / inputs), 100 * (1 - prime / inputs), \
        100 * core / inputs, core ? 100 * (core - prime) / core : 0
}
