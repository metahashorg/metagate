@echo off
timeout 3 /NOBREAK
set T="%~f1\tmp"
cmd /c del %T% /q /s
"%~f1\7z.exe" x -o%T% "%~f2"
rem set p="%%~fT\release3"
for /r %T% %%a in (*) do if %%~nxa==%3 set p=%%~dpa
if defined p (
cmd /c del "%~f4" /q /s
xcopy /E /Y "%p:~0,-1%" "%~f4"
echo TT "%p:~0,-1%" "%~f4"
cd /D "%~f4"
cmd /C %3
) else (
echo File not found %T% %3
)


REM @echo off
REM timeout 3 /NOBREAK
REM cmd /c del "%5" /q /s
REM %1\7z.exe x -o%5 %2
REM rem echo "First" %3 %5 >> C:\Users\svr\AppData\Local\Temp\{A}\log.log
REM rem cmd /c for /r %5 %%a in (*) do if "%%~nxa"==%3 set p=%%~dpa
REM set p=%5/release3/
REM rem echo p %p%
REM rem echo "Second" %3 %5 >> C:\Users\svr\AppData\Local\Temp\{A}\log.log
REM if defined p (
REM cmd /c del "%4" /q /s
REM rem echo "%p:~0,-1%" %4 %5 %a% >> C:\Users\svr\AppData\Local\Temp\{A}\log.log
REM xcopy /E /Y "%p:~0,-1%" "%4"
REM rem echo "%p:~0,-1%" %4 %5 %a% >> C:\Users\svr\AppData\Local\Temp\{A}\log.log
REM cd /D %4
REM cmd /K %3
REM ) else (
REM echo File not found %5 %3 >> C:\Users\svr\AppData\Local\Temp\{A}\log.log
REM )