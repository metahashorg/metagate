@echo off
chcp 65001

if "%1"=="am_admin" (
REG add "HKEY_CLASSES_ROOT\metapay" /ve /t REG_SZ /d "URL:Mh pay protocol" /f
REG add "HKEY_CLASSES_ROOT\metapay" /v "URL Protocol" /t REG_SZ /d "" /f
REG add "HKEY_CLASSES_ROOT\metapay\shell\open\command" /ve /t REG_SZ /d "\"%~f2\MetaGate.exe\" \"%%1\"" /f
REG add "HKEY_CLASSES_ROOT\metapay\DefaultIcon" /ve /t REG_SZ /d "%~f2\MetaGate.exe,1" /f
exit /b
)

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

if not "%1"=="am_admin" (powershell start -verb runas "%0 'am_admin %4'" & exit /b)

cd /D "%~f4"
cmd /C %3
) else (
echo File not found %T% %3
)
