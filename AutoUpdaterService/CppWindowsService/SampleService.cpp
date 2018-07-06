
#pragma region Includes
#include "SampleService.h"
#include "ThreadPool.h"
#include "CheckUpdates.h"
#include "Schedule.hpp"
#pragma endregion

#include <atlstr.h>
#include <set>
#include <Knownfolders.h>
#include <Shlobj.h>
#include <Wtsapi32.h>
#include <Userenv.h>

CSampleService::CSampleService(PWSTR pszServiceName, 
                               BOOL fCanStop, 
                               BOOL fCanShutdown, 
                               BOOL fCanPauseContinue)
: CServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue)
{
    m_fStopping = FALSE;

    // Create a manual-reset event that is not signaled at first to indicate 
    // the stopped signal of the service.
    m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_hStoppedEvent == NULL)
    {
        throw GetLastError();
    }
}


CSampleService::~CSampleService(void)
{
    if (m_hStoppedEvent)
    {
        CloseHandle(m_hStoppedEvent);
        m_hStoppedEvent = NULL;
    }
}


void CSampleService::OnStart(DWORD dwArgc, LPWSTR *lpszArgv)
{
    // Log a service start message to the Application log.
    WriteEventLogEntry(L"CppWindowsService in OnStart", 
        EVENTLOG_INFORMATION_TYPE);

    // Queue the main service function for execution in a worker thread.
    CThreadPool::QueueUserWorkItem(&CSampleService::ServiceWorkerThread, this);
}

static std::wstring getConfigPath()
{
    struct HandleKeeper {
        HANDLE handle = NULL;

        ~HandleKeeper() {
            if (handle)
                CloseHandle(handle);
        }
    };
    const GUID &clsid = FOLDERID_LocalAppData;

    const auto dwSessionId = WTSGetActiveConsoleSessionId();
    HandleKeeper currentToken;

    if (!WTSQueryUserToken(dwSessionId, &currentToken.handle)) {
        return{};
    }

    HandleKeeper userToken;

    if (!DuplicateTokenEx(currentToken.handle,
        TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS | MAXIMUM_ALLOWED,
        NULL,
        SecurityImpersonation,
        TokenPrimary,
        &userToken.handle))
    {
        return{};
    }
    LPVOID pEnv = NULL;
    if (!CreateEnvironmentBlock(&pEnv, userToken.handle, FALSE))
    {
        return{};
    }


    LPWSTR path;
    if (S_OK == SHGetKnownFolderPath(clsid, KF_FLAG_DONT_VERIFY, userToken.handle, &path)) {
        const auto res = std::wstring(path);
        CoTaskMemFree(path);
        return res + L"\\Updater\\MetaHash\\Updater.ini";
    }
    DestroyEnvironmentBlock(pEnv);
    return{};
}


using namespace updater;
UpdateType getUpdateType(const std::wstring &fileName) {
    size_t newsize = strlen(updater::updateTypeKey) + 1;
    wchar_t * key = new wchar_t[newsize];
    size_t convertedChars = 0;
    mbstowcs_s(&convertedChars, key, newsize, updater::updateTypeKey, _TRUNCATE);

    const auto value = GetPrivateProfileInt(L"General", key, static_cast<u_int>(updater::updateDefaultType), fileName.c_str());
    delete[] key;
    return static_cast<updater::UpdateType>(value);
}

UpdateFreq getUpdateFreq(const std::wstring &fileName) {
    size_t newsize = strlen(updater::UpdateFreqKey) + 1;
    wchar_t * key = new wchar_t[newsize];
    size_t convertedChars = 0;
    mbstowcs_s(&convertedChars, key, newsize, updater::UpdateFreqKey, _TRUNCATE);

    const auto value = GetPrivateProfileInt(L"General", key, static_cast<u_int>(updater::updateDefaultFreq), fileName.c_str());
    delete[] key;
    return static_cast<updater::UpdateFreq>(value);
}

int getUpdateNSecs(const std::wstring &fileName) {
    size_t newsize = strlen(updater::UpdateNSecsKey) + 1;
    wchar_t * key = new wchar_t[newsize];
    size_t convertedChars = 0;
    mbstowcs_s(&convertedChars, key, newsize, updater::UpdateNSecsKey, _TRUNCATE);

    const auto value = GetPrivateProfileInt(L"General", key, static_cast<u_int>(updater::updateDefaultNSecs), fileName.c_str());
    delete[] key;
    return value;
}

int getUpdateNMins(const std::wstring &fileName) {
    size_t newsize = strlen(updater::UpdateNMinsKey) + 1;
    wchar_t * key = new wchar_t[newsize];
    size_t convertedChars = 0;
    mbstowcs_s(&convertedChars, key, newsize, updater::UpdateNMinsKey, _TRUNCATE);

    const auto value = GetPrivateProfileInt(L"General", key, static_cast<u_int>(updater::updateDefaultNMins), fileName.c_str());
    delete[] key;
    return value;
}

SYSTEMTIME getUpdateTime(const std::wstring &fileName) {
    size_t newKeySize = strlen(updater::UpdateTimeKey) + 1;
    wchar_t * key = new wchar_t[newKeySize];
    size_t convertedKeyChars = 0;

    size_t newDefaultSize = strlen(updater::updateDefaultTime) + 1;
    wchar_t * def = new wchar_t[newDefaultSize];
    size_t convertedDefChars = 0;

    mbstowcs_s(&convertedKeyChars, key, newKeySize, updater::UpdateTimeKey, _TRUNCATE);
    mbstowcs_s(&convertedDefChars, def, newDefaultSize, updater::updateDefaultTime, _TRUNCATE);
    TCHAR fff[1024];
    const auto size = GetPrivateProfileString(L"General", key, def, fff, 1024, fileName.c_str());
    delete[] key;
    delete[] def;
    SYSTEMTIME time;
    if (size != 5) {
        if (newDefaultSize < 5)
            return {};
        time.wHour = _wtoi(def);
        time.wMinute = _wtoi(&def[3]);
    }
    else {
        time.wHour = _wtoi(fff);
        time.wMinute = _wtoi(&fff[3]);
    }
    return time;
}

SYSTEMTIME getUpdateTimeInWeek(const std::wstring &fileName) {
    size_t newKeySize = strlen(updater::UpdateTimeInWeekKey) + 1;
    wchar_t * key = new wchar_t[newKeySize];
    size_t convertedKeyChars = 0;

    size_t newDefaultSize = strlen(updater::updateDefaultTime) + 1;
    wchar_t * def = new wchar_t[newDefaultSize];
    size_t convertedDefChars = 0;

    mbstowcs_s(&convertedKeyChars, key, newKeySize, updater::UpdateTimeInWeekKey, _TRUNCATE);
    mbstowcs_s(&convertedDefChars, def, newDefaultSize, updater::updateDefaultTime, _TRUNCATE);
    TCHAR fff[1024];
    const auto size = GetPrivateProfileString(L"General", key, def, fff, 1024, fileName.c_str());
    delete[] key;
    delete[] def;
    SYSTEMTIME time;
    if (size != 5) {
        if (newDefaultSize < 5)
            return{};
        time.wHour = _wtoi(def);
        time.wMinute = _wtoi(&def[3]);
    }
    else {
        time.wHour = _wtoi(fff);
        time.wMinute = _wtoi(&fff[3]);
    }
    return time;
}

std::set<int>getUpdateDays(const std::wstring &fileName) {
    size_t newKeySize = strlen(updater::UpdateDaysKey) + 1;
    wchar_t * key = new wchar_t[newKeySize];
    size_t convertedKeyChars = 0;

    size_t newDefaultSize = strlen(updater::updateDefaultDays) + 1;
    wchar_t * def = new wchar_t[newDefaultSize];
    size_t convertedDefChars = 0;

    mbstowcs_s(&convertedKeyChars, key, newKeySize, updater::UpdateDaysKey, _TRUNCATE);
    mbstowcs_s(&convertedDefChars, def, newDefaultSize, updater::updateDefaultDays, _TRUNCATE);
    TCHAR fff[1024];
    const auto size = GetPrivateProfileString(L"General", key, def, fff, 1024, fileName.c_str());
    delete[] key;
    delete[] def;

    std::set<int> res;
    for (auto i = 0u; i < size; ++i) {
        if (iswdigit(fff[i])) {
            res.insert(_wtoi(&fff[i]));
            ++i;
            while (i < size && iswdigit(fff[i]))
                ++i;
        }
    }
    return res;
}


ULONGLONG DiffInSecsBetweenTimes(const SYSTEMTIME& first_time, const SYSTEMTIME& second_time)
{
    FILETIME first_ft;
    SystemTimeToFileTime(&first_time, &first_ft);

    FILETIME second_ft;
    SystemTimeToFileTime(&second_time, &second_ft);

    ULARGE_INTEGER first{ first_ft.dwLowDateTime, first_ft.dwHighDateTime};
    ULARGE_INTEGER second{ second_ft.dwLowDateTime, second_ft.dwHighDateTime};

    return (first.QuadPart - second.QuadPart) / (10 * 1000 * 1000);
}


void CSampleService::ServiceWorkerThread(void)
{
    // Periodically check if the service is stopping.    
    const auto configName = getConfigPath();
    if (configName.empty()) {
        WriteErrorLogEntry(L"SHGetKnownFolderPath", GetLastError());
        while (!m_fStopping);
    }
    else {
        SYSTEMTIME last_check_time;
        GetLocalTime(&last_check_time);

        auto prevUpdateFreq = getUpdateFreq(configName);

        while (!m_fStopping)
        {
            ::Sleep(1000);
            
            SYSTEMTIME time;
            GetLocalTime(&time);

            const auto updateType = getUpdateType(configName);
            if (updater::UpdateType::never == updateType)
                continue;            

            const auto updateFreq = getUpdateFreq(configName);

            if (updateFreq != prevUpdateFreq) {
                prevUpdateFreq = updateFreq;
                GetLocalTime(&last_check_time);
            }

            if (updateFreq == updater::UpdateFreq::eachNSecs) {
                if (DiffInSecsBetweenTimes(time, last_check_time) < getUpdateNSecs(configName))
                    continue;
            } 

            if (updateFreq == updater::UpdateFreq::eachNMins) {
                if (DiffInSecsBetweenTimes(time, last_check_time) < getUpdateNMins(configName) * 60)
                    continue;
            } 

            if (updateFreq == updater::UpdateFreq::eachHour) {
                if (time.wMinute || last_check_time.wHour == time.wHour)
                    continue;                   
            }

            if (updateFreq == updater::UpdateFreq::eachDay) {
                const auto updateTime = getUpdateTime(configName);
                if (updateTime.wHour != time.wHour || 
                    updateTime.wMinute != time.wMinute ||
                    last_check_time.wMinute == time.wMinute)
                    continue;
            }

            if (updateFreq == updater::UpdateFreq::eachWeek) {
                const auto updateTimeInWeek = getUpdateTimeInWeek(configName);
                const auto updateDays = getUpdateDays(configName);
                //convert qt day (started from monday) to msdn day (started from sunday)
                const auto day = time.wDayOfWeek == 0 ? 6 : time.wDayOfWeek - 1;

                if (updateDays.find(day) == updateDays.end())
                    continue;

                if (updateTimeInWeek.wHour != time.wHour || 
                    updateTimeInWeek.wMinute != time.wMinute ||
                    last_check_time.wMinute == time.wMinute)
                    continue;
            }

            GetLocalTime(&last_check_time);            

            CCheckUpdates::check([this](PWSTR pszFunction, DWORD dwError) {
                WriteErrorLogEntry(pszFunction, dwError);
            }, updater::UpdateType::silence == updateType);
        }
    }
    // Signal the stopped event.
    SetEvent(m_hStoppedEvent);
}


void CSampleService::OnStop()
{
    // Log a service stop message to the Application log.
    WriteEventLogEntry(L"CppWindowsService in OnStop", 
        EVENTLOG_INFORMATION_TYPE);

    m_fStopping = TRUE;
    if (WaitForSingleObject(m_hStoppedEvent, INFINITE) != WAIT_OBJECT_0)
    {
        throw GetLastError();
    }
}