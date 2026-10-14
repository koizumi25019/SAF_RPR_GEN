@echo off
rem ============================================================
rem  saxid_v1 : run all 7 circuits with the 1-fault-1-pattern STTG
rem             patterns (tp_1f1p\). Each line = 1 pattern + 1 fault.
rem
rem  Usage : double-click            -> all 7 circuits
rem          run_1f1p.bat s5378 s9234 -> only the given circuits
rem  Input : tp_1f1p\<circuit>_C_1f1p_tp.txt , tp_1f1p\<circuit>_C_1f1p_fault.txt
rem          (made by MTTG "bat\1f1p\run_1f1p.bat", which copies them here)
rem          Circuits whose input files are missing are skipped.
rem  Output: result_1f1p\<circuit>_result.txt , result_1f1p\<circuit>_xtp.txt ,
rem          result_1f1p\log_<circuit>.txt
rem  Note  : the number of cases is large (s5378: 4511 ... s38417: 30579).
rem          Every case has only 1 fault, so --max-faults has no effect here.
rem ============================================================
setlocal enabledelayedexpansion
cd /d %~dp0
if not exist result_1f1p mkdir result_1f1p

rem ------------------------------------------------------------
rem  Options (OPT)
rem   Only the LAST "set OPT=" line that is not commented out is used.
rem   To switch, put "rem " in front of the current line and remove
rem   "rem " from the line you want.
rem   Always keep --batch (otherwise the program waits for a keyboard input).
rem
rem   --batch          run in batch mode (no mode prompt at startup)
rem   --threads=N      number of clasp threads, 1..64 (default 1).
rem                    N>=2 uses parallel search: faster, but among equally
rem                    good solutions the chosen one may differ between runs
rem                    (same Xcount, different care-bit positions).
rem                    N=1 always gives the same result for the same input.
rem   --keep-opb       keep the OPB file of each case in OPB_Files\
rem                    (default: deleted after solving)
rem   --varmap         append a variable-number / net-name table to each
rem                    kept OPB file (use with --keep-opb)
rem   --max-faults=N   max number of faults per line (default 64).
rem                    Lines with more faults are skipped and recorded as
rem                    ERROR(TooManyFaults:count).
rem   --no-debug       do not print per-case progress to the log
rem   --no-result      do not write the result file (only the X pattern file).
rem                    The result file name in this bat is then ignored.
rem ------------------------------------------------------------
set OPT=--batch --threads=16
rem set OPT=--batch --threads=1
rem set OPT=--batch --threads=4
rem set OPT=--batch --threads=8 --no-debug
rem set OPT=--batch --threads=16 --no-result
rem set OPT=--batch --threads=16 --no-debug
rem set OPT=--batch --keep-opb
rem set OPT=--batch --keep-opb --varmap

rem ------------------------------------------------------------
rem  Target circuits (can also be given as arguments)
rem ------------------------------------------------------------
rem set LIST=s5378 s9234 s13207 s15850 s35932 s38417 s38584
set LIST=s35932 s38584

if not "%~1"=="" set LIST=%*

echo options : %OPT%
for %%c in (%LIST%) do (
    if exist tp_1f1p\%%c_C_1f1p_tp.txt (
        echo [!date! !time!] start %%c
        saxid_v1.exe circuit\%%c_C.v tp_1f1p\%%c_C_1f1p_fault.txt tp_1f1p\%%c_C_1f1p_tp.txt result_1f1p\%%c_result.txt result_1f1p\%%c_xtp.txt %OPT% > result_1f1p\log_%%c.txt 2>&1
        echo [!date! !time!] end   %%c  exit=!errorlevel!
    ) else (
        echo [!date! !time!] skip  %%c  ^(tp_1f1p\%%c_C_1f1p_tp.txt not found^)
    )
)
echo all done.
pause
