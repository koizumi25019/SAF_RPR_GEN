@echo off
rem ============================================================
rem  saxid_v1 : run all 7 circuits with the STTG patterns (tp\)
rem
rem  Usage : double-click            -> all 7 circuits
rem          run_all.bat s5378 s9234 -> only the given circuits
rem  Input : tp\<circuit>_C_tp.txt , tp\<circuit>_C_tp_fault.txt
rem  Output: result\<circuit>_result.txt , result\<circuit>_xtp.txt ,
rem          result\log_<circuit>.txt
rem ============================================================
setlocal enabledelayedexpansion
cd /d %~dp0
if not exist result mkdir result

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
rem set OPT=--batch --keep-opb
rem set OPT=--batch --keep-opb --varmap
rem set OPT=--batch --max-faults=128
rem set OPT=--batch --threads=4 --max-faults=128 --no-debug

rem ------------------------------------------------------------
rem  Target circuits (can also be given as arguments)
rem ------------------------------------------------------------
set LIST=s5378 s9234 s13207 s15850 s35932 s38417 s38584
if not "%~1"=="" set LIST=%*

echo options : %OPT%
for %%c in (%LIST%) do (
    echo [!date! !time!] start %%c
    saxid_v1.exe circuit\%%c_C.v tp\%%c_C_tp_fault.txt tp\%%c_C_tp.txt result\%%c_result.txt result\%%c_xtp.txt %OPT% > result\log_%%c.txt 2>&1
    echo [!date! !time!] end   %%c  exit=!errorlevel!
)
echo all done.
pause
