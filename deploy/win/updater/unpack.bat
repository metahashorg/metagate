@echo off
chcp 65001
timeout 3 /NOBREAK
set T="%~f1\tmp"
cmd /c del %T% /q /s
"%~f1\7z.exe" x -o%T% "%~f2"
rem set p="%%~fT\release3"
for /r %T% %%a in (*) do if %%~nxa==%3 set p=%%~dpa
if defined p (
cmd /c del "%~f4" /q /s
ROBOCOPY "%p:~0,-1%" "%~f4" /E
echo TT "%p:~0,-1%" "%~f4"
cd /D "%~f4"
cmd /C %3
) else (
echo File not found %T% %3
)
