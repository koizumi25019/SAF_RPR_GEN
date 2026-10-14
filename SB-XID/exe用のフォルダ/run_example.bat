@echo off
rem ============================================================
rem  saxid_v1 example: X-identification for s5378 (STTG patterns)
rem  Output: result\s5378_result.txt , result\s5378_xtp.txt
rem ============================================================
cd /d %~dp0
if not exist result mkdir result
saxid_v1.exe circuit\s5378_C.v tp\s5378_C_tp_fault.txt tp\s5378_C_tp.txt result\s5378_result.txt result\s5378_xtp.txt --batch
pause
