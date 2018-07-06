#include "CheckUpdates.h"
#include "SampleService.h"

#include <tchar.h>
#include <stdio.h> 
#include <Wtsapi32.h>
#include <TlHelp32.h>
#include <Userenv.h>
#include <string>

#define OUTPUTBUFSIZE 4096
#define UNUSED(expr) do { (void)(expr); } while (0)

std::string CCheckUpdates::run_command(LPTSTR cmd, const std::function<void(PWSTR pszFunction, DWORD dwError)> &logger)
{
    STARTUPINFO sinfo;
    PROCESS_INFORMATION pinfo;
    SECURITY_ATTRIBUTES sattr;
    HANDLE readfh;
    register char *cbuff = nullptr;

    // Allocate a buffer to read the app's output
    if (!(cbuff = (char *)GlobalAlloc(GMEM_FIXED, OUTPUTBUFSIZE)))
    {
        logger(L"GlobalAlloc", GetLastError());
        return {};
    }

    // Initialize the STARTUPINFO struct
    ZeroMemory(&sinfo, sizeof(STARTUPINFO));
    sinfo.cb = sizeof(STARTUPINFO);

    sinfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

    // Uncomment this if you want to hide the other app's
    // DOS window while it runs
    //sinfo.wShowWindow = SW_HIDE;

    sinfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    sinfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    // Initialize security attributes to allow the launched app to
    // inherit the caller's STDOUT, STDIN, and STDERR
    sattr.nLength = sizeof(SECURITY_ATTRIBUTES);
    sattr.lpSecurityDescriptor = 0;
    sattr.bInheritHandle = TRUE;

    // Get a pipe from which we read
    // output from the launched app
    if (!CreatePipe(&readfh, &sinfo.hStdOutput, &sattr, 0))
    {
        // Error opening the pipe
        logger(L"CreatePipe", GetLastError());
        GlobalFree(cbuff);
        return nullptr;
    }


    // Launch the app. We should return immediately (while the app is running)
    if (!CreateProcess(0, cmd, 0, 0, TRUE, 0, 0, 0, &sinfo, &pinfo))
    {
        logger(L"CreateProcess", GetLastError());
        CloseHandle(readfh);
        CloseHandle(sinfo.hStdOutput);
        GlobalFree(cbuff);
        return nullptr;
    }

    // Don't need the read access to these pipes
    CloseHandle(sinfo.hStdOutput);

    // We haven't yet read app's output
    sinfo.dwFlags = 0;

    while (readfh)
    {
        // Read in upto OUTPUTBUFSIZE bytes
        if (!ReadFile(readfh, cbuff + sinfo.dwFlags, OUTPUTBUFSIZE - sinfo.dwFlags, &pinfo.dwProcessId, NULL) || !pinfo.dwProcessId)
        {
            // If we aborted for any reason other than that the
            // app has closed that pipe, it's an
            // error. Otherwise, the program has finished its
            // output apparently
            const auto lastError = GetLastError();
            if (lastError != ERROR_BROKEN_PIPE && pinfo.dwProcessId)
            {
                // An error reading the pipe
                logger(L"ReadFile", lastError);
                CloseHandle(readfh);
                CloseHandle(sinfo.hStdOutput);
                return nullptr;
            }

            // Close the pipe
            CloseHandle(readfh);
            readfh = 0;
        }

        sinfo.dwFlags += pinfo.dwProcessId;
    }

    // Close output pipe
    if (readfh) CloseHandle(readfh);

    // Wait for the app to finish
    WaitForSingleObject(pinfo.hProcess, INFINITE);

    // Close process and thread handles
    CloseHandle(pinfo.hProcess);
    CloseHandle(pinfo.hThread);

    // Nul-terminate it
    if (cbuff) *(cbuff + sinfo.dwFlags) = 0;

    std::string res(cbuff);

    // Return the output
    GlobalFree(cbuff);
    return res;
}


CCheckUpdates::CCheckUpdates()
{
}

void CCheckUpdates::check(const std::function<void(PWSTR pszFunction, DWORD dwError)> &logger, bool silency)
{
    struct HandleKeeper {
        HANDLE handle = NULL;

        ~HandleKeeper() {
            if (handle)
                CloseHandle(handle);
        }
    };

    LPTSTR command = _tcsdup(TEXT("maintenancetool.exe --checkupdates"));
    if (!run_command(command, logger).empty())
    {
        /*
        size_t newsize = strlen(output) + 1;
        wchar_t * wcstring = new wchar_t[newsize];
        size_t convertedChars = 0;
        mbstowcs_s(&convertedChars, wcstring, newsize, output, _TRUNCATE);
        */

        
        TOKEN_PRIVILEGES oldTokenPrivileges = { 0 };
        HandleKeeper processToken;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &processToken.handle)) {
            logger(L"OpenProcessToken", GetLastError());
            return;
        }

        // This step might not be necessary because SeTcbPrivilege is enabled by default for Local System
        LUID luid;
        if (!LookupPrivilegeValue(NULL, SE_TCB_NAME, &luid)) {
            logger(L"LookupPrivilegeValue", GetLastError());
            return;
        }

        TOKEN_PRIVILEGES adjTokenPrivileges = { 0 };
        adjTokenPrivileges.PrivilegeCount = 1;
        adjTokenPrivileges.Privileges[0].Luid = luid;
        adjTokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        
        DWORD dwOldTPLen;
        DWORD dwSessionId;
        const auto e = GetLastError();
        if (AdjustTokenPrivileges(processToken.handle, FALSE, &adjTokenPrivileges, sizeof(TOKEN_PRIVILEGES), &oldTokenPrivileges, &dwOldTPLen)) {
            const auto err = GetLastError();
            if (err == ERROR_NOT_ALL_ASSIGNED) {
                logger(L"AdjustTokenPrivileges", err);
            }
        }
        else {
            logger(L"AdjustTokenPrivileges", GetLastError());
            return;
        }

        struct RevertPrivelegue {
            ~RevertPrivelegue() {
                RevertToSelf();
            }
        };
        RevertPrivelegue revertKeper;
        UNUSED(revertKeper);

        STARTUPINFO sinfo;
        PROCESS_INFORMATION pinfo;
        SecureZeroMemory(&pinfo, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&sinfo, sizeof(STARTUPINFO));
        sinfo.cb = sizeof(STARTUPINFO);
        sinfo.dwFlags = STARTF_USESHOWWINDOW;

        sinfo.wShowWindow = SW_SHOW;
        sinfo.lpDesktop = _tcsdup(TEXT("Winsta0\\default"));
        
        dwSessionId = WTSGetActiveConsoleSessionId();
        HandleKeeper currentToken;

        if (!WTSQueryUserToken(dwSessionId, &currentToken.handle)){
            logger(L"WTSQueryUserToken", GetLastError());
            return;
        }



        HandleKeeper userToken;

        if (!DuplicateTokenEx(currentToken.handle,
            TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS | MAXIMUM_ALLOWED,
            NULL,
            SecurityImpersonation,
            TokenPrimary,
            &userToken.handle))
        {
            logger(L"DuplicateTokenEx", GetLastError());
            return;
        }
        LPVOID pEnv = NULL;
        if (!CreateEnvironmentBlock(&pEnv, userToken.handle, FALSE))
        {
            logger(L"CreateEnvironmentBlock", GetLastError());
            return;
        }

        TCHAR szDir[MAX_PATH] = { 0 };

        GetModuleFileName(NULL, szDir, MAX_PATH);
        szDir[std::wstring(szDir).find_last_of(L"\\/")] = 0;
        const std::wstring fileName = std::wstring(szDir) + L"\\maintenancetool.exe";
        std::wstring args2 = fileName + std::wstring(L" --script=\"") + std::wstring(szDir) + (silency ? std::wstring(L"\\silency.qs\"") : std::wstring(L"\\script.qs\""));
        if (CreateProcessAsUser(userToken.handle, NULL, &args2[0], NULL, NULL, false, 0, NULL, NULL, &sinfo, &pinfo))
        {
            WaitForSingleObject(pinfo.hProcess, INFINITE);
            if (pinfo.hProcess != NULL)
            {
                CloseHandle(pinfo.hProcess);
            }
            if (pinfo.hThread != NULL)
            {
                CloseHandle(pinfo.hThread);
            }
        }
        else {
            logger(L"CreateProcessAsUser", GetLastError());
        }
        DestroyEnvironmentBlock(pEnv);
        RevertToSelf();
    }
}


CCheckUpdates::~CCheckUpdates()
{
}
