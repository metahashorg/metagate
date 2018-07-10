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

static BOOL LaunchAppIntoDifferentSession(std::wstring &lpCommandLine) {
	PROCESS_INFORMATION pi;
	STARTUPINFOEX si;
	BOOL bResult = FALSE;
	DWORD dwSessionId, winlogonPid;
	HANDLE hUserToken, hUserTokenDup, hPToken, hProcess;
	DWORD dwCreationFlags;

	dwSessionId = WTSGetActiveConsoleSessionId();
	if (!dwSessionId) {
		goto Cleanup;
	}

	//////////////////////////////////////////
	// Find the winlogon process
	////////////////////////////////////////
	PROCESSENTRY32 procEntry;

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE) {
		goto Cleanup;
	}

	procEntry.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hSnap, &procEntry)) {
		goto Cleanup;
	}

	do {
		if (_stricmp((const char*)procEntry.szExeFile, (const char*)(L"winlogon.exe")) == 0) {
			// We found a winlogon process...
			// make sure it's running in the console session
			DWORD winlogonSessId = 0;
			if (ProcessIdToSessionId(procEntry.th32ProcessID, &winlogonSessId)
				&& winlogonSessId == dwSessionId) {
				winlogonPid = procEntry.th32ProcessID;
				break;
			}
		}
	} while (Process32Next(hSnap, &procEntry));
	////////////////////////////////////////////////////////////////////////

	printf("Active winlogon session PID: %d\n", winlogonPid);

	hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, winlogonPid);
	if (hProcess == INVALID_HANDLE_VALUE) {
		goto Cleanup;
	}

	if (!::OpenProcessToken(hProcess,/*TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY
									 |*/TOKEN_DUPLICATE/*|TOKEN_ASSIGN_PRIMARY|TOKEN_ADJUST_SESSIONID
									 |TOKEN_READ|TOKEN_WRITE*/, &hPToken)) {
		goto Cleanup;
	}

	WTSQueryUserToken(dwSessionId, &hUserToken);
	if (hUserToken == INVALID_HANDLE_VALUE) {
		goto Cleanup;
	}

	dwCreationFlags = NORMAL_PRIORITY_CLASS;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.StartupInfo.cb = sizeof(STARTUPINFO);
	si.StartupInfo.lpDesktop = L"winsta0\\default";
	ZeroMemory(&pi, sizeof(pi));
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
		goto Cleanup;
	}
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!DuplicateTokenEx(hPToken, MAXIMUM_ALLOWED, NULL,
		SecurityIdentification, TokenPrimary, &hUserTokenDup)) {
		goto Cleanup;
	}

	//Adjust Token privilege
	SetTokenInformation(hUserTokenDup,
		TokenSessionId, (void*)dwSessionId, sizeof(DWORD));

	if (!AdjustTokenPrivileges(hUserTokenDup, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),
		(PTOKEN_PRIVILEGES)NULL, NULL)) {
		goto Cleanup;
	}

	LPVOID pEnv = NULL;

	/*SIZE_T AttributeListSize;
	InitializeProcThreadAttributeList(NULL, 1, 0, &AttributeListSize);


	si.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(
		GetProcessHeap(),
		0,
		AttributeListSize
	);


	if (InitializeProcThreadAttributeList(si.lpAttributeList,
		1,
		0,
		&AttributeListSize) == FALSE) {
		goto Cleanup;
	}


	if (UpdateProcThreadAttribute(si.lpAttributeList,
		0,
		PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
		&hProcess,
		sizeof(HANDLE),
		NULL,
		NULL) == FALSE) {
		goto Cleanup;
	}*/

	if (CreateEnvironmentBlock(&pEnv, hUserTokenDup, TRUE)) {
		dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
	} else
		pEnv = NULL;

	// Launch the process in the client's logon session.
	bResult = CreateProcessAsUser(
		hUserTokenDup,           // client's access token
		NULL,  // file to execute
		&lpCommandLine[0],         // command line
		NULL,      // pointer to process SECURITY_ATTRIBUTES
		NULL,        // pointer to thread SECURITY_ATTRIBUTES
		FALSE,       // handles are not inheritable
		dwCreationFlags,   // creation flags
		pEnv,        // pointer to new environment block
		NULL,        // name of current directory
		(LPSTARTUPINFOW)&si,        // pointer to STARTUPINFO structure
		&pi        // receives information about new process
	);
	// End impersonation of client.

	WaitForSingleObject(pi.hProcess, INFINITE);

Cleanup:

	DestroyEnvironmentBlock(pEnv);
	CloseHandle(hProcess);
	CloseHandle(hUserToken);
	CloseHandle(hUserTokenDup);
	CloseHandle(hPToken);

	return bResult;

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

		TCHAR szDir[MAX_PATH] = { 0 };
		GetModuleFileName(NULL, szDir, MAX_PATH);
		szDir[std::wstring(szDir).find_last_of(L"\\/")] = 0;
		const std::wstring fileName = std::wstring(szDir) + L"\\maintenancetool.exe";
		std::wstring args2 = fileName + std::wstring(L" --script=\"") + std::wstring(szDir) + (silency ? std::wstring(L"\\silency.qs\"") : std::wstring(L"\\script.qs\""));
		LaunchAppIntoDifferentSession(args2);

        /*LPVOID pEnv = NULL;
        if (!CreateEnvironmentBlock(&pEnv, userToken.handle, FALSE))
        {
            logger(L"CreateEnvironmentBlock", GetLastError());
            return;
        }

        TCHAR szDir[MAX_PATH] = { 0 };

		SIZE_T AttributeListSize;
		InitializeProcThreadAttributeList(NULL, 1, 0, &AttributeListSize);

		sinfo.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(
			GetProcessHeap(),
			0,
			AttributeListSize
		);

		if (InitializeProcThreadAttributeList(sinfo.lpAttributeList,
			1,
			0,
			&AttributeListSize) == FALSE) {
			logger(L"InitializeProcThreadAttributeList", GetLastError());
			return;
		}

		PVOID t = 0;
		if (UpdateProcThreadAttribute(sinfo.lpAttributeList,
			0,
			PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
			t,
			sizeof(t),
			NULL,
			NULL) == FALSE) {
			logger(L"UpdateProcThreadAttribute", GetLastError());
			return;
		}

        GetModuleFileName(NULL, szDir, MAX_PATH);
        szDir[std::wstring(szDir).find_last_of(L"\\/")] = 0;
        const std::wstring fileName = std::wstring(szDir) + L"\\maintenancetool.exe";
        std::wstring args2 = fileName + std::wstring(L" --script=\"") + std::wstring(szDir) + (silency ? std::wstring(L"\\silency.qs\"") : std::wstring(L"\\script.qs\""));
        if (CreateProcessAsUser(userToken.handle, NULL, &args2[0], NULL, NULL, false, 0, NULL, NULL, (LPSTARTUPINFOW)&sinfo, &pinfo))
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
        RevertToSelf();*/
    }
}


CCheckUpdates::~CCheckUpdates()
{
}
