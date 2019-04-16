::REG ADD "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Run" /v "MetaGate" /t REG_SZ /d "\"C:\Users\alave\MetaGate2\MetaGate.exe\" \"-t\"" /f
::REG ADD "HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run" /v "MetaGate" /t REG_SZ /d "\"C:\Users\alave\MetaGate2\MetaGate.exe\" \"-t\"" /f
REG DELETE "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Run" /v "MetaGate" /f
REG DELETE "HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run" /v "MetaGate" /f
