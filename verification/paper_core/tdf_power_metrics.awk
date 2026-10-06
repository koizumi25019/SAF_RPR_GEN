# Parse completed low-power CORE runs, including runs with zero cubes.
BEGIN {
    FS = ","
    while ((getline line < logfile) > 0) {
        split(line, fields, ":"); value = fields[2] + 0
        if (line ~ /^\/\/  CPU Time *:/) cpu = value
        if (line ~ /^\/\/  Time *:/) wall = value
        if (line ~ /CPU Time \(Don.t care\)/) dc = value
        if (line ~ /CPU Time \(CaDiCaL\)/) sat = value
        if (line ~ /CPU Time \(BDD\)/) bdd = value
        if (line ~ /Number of Target Faults/) expected_faults = value
        if (line ~ /Don't-care Method/ && line ~ /CORE/) core_method = 1
        if (line ~ /Dominance Cube Reuse/ && line ~ /off/) nodom = 1
        if (line ~ /CORE Extra Verification/ && line ~ /off/) noverify = 1
        if (line ~ /Low Power/ && line ~ /on/) power = 1
        if (line ~ /denominator=2\^/) { split(line, bits, "denominator=2\\^"); inputs = bits[2] + 0 }
    }
    close(logfile)
    while ((getline line < errfile) > 0) {
        if (line ~ /^\[POWER\] signals=/) {
            split(line, words, " ")
            split(words[2], a, "="); signals = a[2]+0
            split(words[3], a, "="); threshold = a[2]+0
            split(words[4], a, "="); budget = a[2]+0
            found_power++
        }
        if (line ~ /^\[PAPER_CORE\] cubes=/) {
            split(line, words, " ")
            split(words[2], a, "="); core_cubes = a[2]+0
            split(words[3], a, "="); solves = a[2]+0
            split(words[5], a, "="); input_bits = a[2]+0
            split(words[6], a, "="); care_core = a[2]+0
            split(words[7], a, "="); care_min = a[2]+0
            found_core++
        }
        if (line ~ /^\[GT\]/) diagnostic = 1
    }
    close(errfile)
    limit = (circuit == "s5378" || circuit == "s9234") ? 30 : 0
}
FNR > 1 && ($4 == "0" || $4 == "1") {
    faults++; cubes += $3
    if ($4 == "1") complete++; else incomplete++
    if ($5 + 0 > 0) positive++; else zero++
    if ($6 + 0 != 0 || (limit && $3 + 0 > limit)) invalid = 1
}
END {
    if (!core_method || !nodom || !noverify || !power || diagnostic || invalid ||
        found_core != 1 || found_power != 1 || faults != expected_faults ||
        threshold != 20 || !inputs || cubes != core_cubes || input_bits != inputs*cubes ||
        care_min > care_core || care_core > input_bits || solves != 2*cubes + care_core) {
        print "Invalid low-power benchmark run: " FILENAME > "/dev/stderr"; exit 1
    }
    printf "%s,%d,%d,%d,%d,%d,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,", \
        circuit,repetition,limit,threshold,signals,budget,inputs,cpu,wall,dc,sat,bdd, \
        faults,complete,incomplete,zero,positive,cubes,input_bits,care_core,care_min,solves
    if (input_bits) printf "%.6f,%.6f,%.6f,%.6f\n", \
        100*(1-care_core/input_bits),100*(1-care_min/input_bits), \
        100*care_core/input_bits,care_core ? 100*(care_core-care_min)/care_core : 0
    else printf ",,,\n"
}
