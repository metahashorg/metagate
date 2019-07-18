#ifdef _WIN32

#include "machine_uid.h"

#include <vector>
#include <algorithm>
#include <codecvt>
#include <locale>
#include <set>

#include <QString>

#define WIN32_LEAN_AND_MEAN
#include <Wbemidl.h>
#include <windows.h>
#include <intrin.h>
#include <iphlpapi.h>
#include <lm.h>
#include <comdef.h>
#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "wbemuuid.lib")

static bool GetWinMajorMinorVersion(DWORD& major, DWORD& minor) {
    bool bRetCode = false;
    LPBYTE pinfoRawData = 0;
    if (NERR_Success == NetWkstaGetInfo(NULL, 100, &pinfoRawData)) {
        WKSTA_INFO_100* pworkstationInfo = (WKSTA_INFO_100*)pinfoRawData;
        major = pworkstationInfo->wki100_ver_major;
        minor = pworkstationInfo->wki100_ver_minor;
        ::NetApiBufferFree(pinfoRawData);
        bRetCode = true;
    }
    return bRetCode;
}


static std::string osNameImpl() {
    std::string     winver;
    OSVERSIONINFOEX osver;
    SYSTEM_INFO     sysInfo;
    typedef void(__stdcall *GETSYSTEMINFO) (LPSYSTEM_INFO);

    __pragma(warning(push))
    __pragma(warning(disable:4996))
    memset(&osver, 0, sizeof(osver));
    osver.dwOSVersionInfoSize = sizeof(osver);
    GetVersionEx((LPOSVERSIONINFO)&osver);
    __pragma(warning(pop))
    DWORD major = 0;
    DWORD minor = 0;
    if (GetWinMajorMinorVersion(major, minor)) {
        osver.dwMajorVersion = major;
        osver.dwMinorVersion = minor;
    } else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2) {
        OSVERSIONINFOEXW osvi;
        ULONGLONG cm = 0;
        cm = VerSetConditionMask(cm, VER_MINORVERSION, VER_EQUAL);
        ZeroMemory(&osvi, sizeof(osvi));
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        osvi.dwMinorVersion = 3;
        if (VerifyVersionInfoW(&osvi, VER_MINORVERSION, cm)) {
            osver.dwMinorVersion = 3;
        }
    }

    GETSYSTEMINFO getSysInfo = (GETSYSTEMINFO)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetNativeSystemInfo");
    if (getSysInfo == NULL) {
        getSysInfo = ::GetSystemInfo;
    }
    getSysInfo(&sysInfo);

    /*if (osver.dwMajorVersion == 10 && osver.dwMinorVersion >= 0 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows 10 Server";
    if (osver.dwMajorVersion == 10 && osver.dwMinorVersion >= 0 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 10";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 3 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2012 R2";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 3 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 8.1";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2012";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 8";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2008 R2";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 7";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 0 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2008";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 0 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows Vista";
    if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2 && osver.wProductType == VER_NT_WORKSTATION
        &&  sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)  winver = "Windows XP x64";
    if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2)   winver = "Windows Server 2003";
    if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 1)   winver = "Windows XP";
    if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 0)   winver = "Windows 2000";
    if (osver.dwMajorVersion < 5)   winver = "unknown";*/
    winver = "win" + std::to_string(osver.dwMajorVersion) + "." + std::to_string(osver.dwMinorVersion) + "." + std::to_string(osver.wProductType);

    if (osver.wServicePackMajor != 0) {
        std::string sp;
        char buf[128] = { 0 };
        sp = " Service Pack ";
        sprintf_s(buf, sizeof(buf), "%hd", osver.wServicePackMajor);
        sp.append(buf);
        winver += sp;
    }

    return winver;
}

// we just need this for purposes of unique machine id. So any one or two mac's is
// fine.
static uint16_t hashMacAddress(PIP_ADAPTER_INFO info) {
   uint16_t hash = 0;
   for (uint32_t i = 0; i < info->AddressLength; i++) {
      hash += (info->Address[i] << ((i & 1) * 8));
   }
   return hash;
}

static void getMacHash(uint16_t& mac1, uint16_t& mac2) {
   mac1 = 0;
   mac2 = 0;
   IP_ADAPTER_INFO AdapterInfo[32];
   DWORD dwBufLen = sizeof(AdapterInfo);

   DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
   if (dwStatus != ERROR_SUCCESS) {
      return; // no adapters.
   }

   std::vector<uint16_t> addrs;
   PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
   while (pAdapterInfo) {
       addrs.emplace_back(hashMacAddress(pAdapterInfo));
       pAdapterInfo = pAdapterInfo->Next;
   }
   const auto savedPair = findMacAddressFile();
   if (addrs.empty()) {
       // При включении может быть ситуация, когда интерфейсы еще не подгрузились. Берем из файла
       if (!savedPair.first.empty()) {
           mac1 = std::stoul(savedPair.first);
           mac2 = std::stoul(savedPair.second);
       }
       return;
   }
   const auto pair = std::minmax_element(addrs.begin(), addrs.end());
   mac1 = *pair.first;
   mac2 = *pair.second;

   if (!savedPair.first.empty()) {
       const uint16_t savedMac1 = std::stoul(savedPair.first);
       const uint16_t savedMac2 = std::stoul(savedPair.second);
       if (std::find(addrs.begin(), addrs.end(), savedMac1) != addrs.end() || std::find(addrs.begin(), addrs.end(), savedMac2) != addrs.end()) {
           mac1 = savedMac1;
           mac2 = savedMac2;
       }
   } else {
       saveMacAddressesToFile(std::to_string(mac1), std::to_string(mac2));
   }
}

static uint16_t getVolumeHash() {
   DWORD serialNum = 0;

   // Determine if this volume uses an NTFS file system.
   const bool res = GetVolumeInformationA( LPCSTR("C:\\"), NULL, 0, &serialNum, NULL, NULL, NULL, 0);
   if (!res) {
       serialNum = 0;
   }
   uint16_t hash = (uint16_t)((serialNum + (serialNum >> 16)) & 0xFFFF);

   return hash;
}

static uint16_t getCpuHash() {
   int cpuinfo[4] = {0, 0, 0, 0};
   __cpuid(cpuinfo, 0);
   uint16_t hash = 0;
   uint16_t* ptr = (uint16_t*)(&cpuinfo[0]);
   for ( uint32_t i = 0; i < 8; i++ ) {
      hash += ptr[i];
   }

   return hash;
}

static std::string getMachineName() {
   static char computerName[1024];
   DWORD size = 1024;
   GetComputerName(LPWSTR(computerName), &size);
   return std::string(computerName);
}

std::string getMachineUidInternal() {
    std::string result;
    result += std::to_string(getCpuHash()) + std::string(";");
    result += std::to_string(getVolumeHash()) + std::string(";");
    unsigned short mac1, mac2;
    getMacHash(mac1, mac2);
    result += std::to_string(mac1) + std::to_string(mac2);
    return result;
}

static std::pair<std::string,std::string> getComputerManufacturerAndModel() {
    // Obtain the initial locator to Windows Management on a particular host computer.
    IWbemLocator *locator = nullptr;
    IWbemServices *services = nullptr;
    auto hResult = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&locator);

    auto hasFailed = [&hResult]() {
        if (FAILED(hResult)) {
            auto error = _com_error(hResult);
            return true;
        }
        return false;
    };

    auto getValue = [&hResult, &hasFailed](IWbemClassObject *classObject, LPCWSTR property) {
        std::string propertyValueText = "Not set";
        VARIANT propertyValue;
        hResult = classObject->Get(property, 0, &propertyValue, 0, 0);
        if (!hasFailed()) {
            if ((propertyValue.vt == VT_NULL) || (propertyValue.vt == VT_EMPTY)) {
            } else if (propertyValue.vt & VT_ARRAY) {
                propertyValueText = "Unknown"; //Array types not supported
            } else {
                const auto wstr = std::wstring(propertyValue.bstrVal, SysStringLen(propertyValue.bstrVal));
                using convert_type = std::codecvt_utf8<wchar_t>;
                std::wstring_convert<convert_type, wchar_t> converter;

                //use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
                propertyValueText = converter.to_bytes(wstr);
            }
        }
        VariantClear(&propertyValue);
        return propertyValueText;
    };

    std::string manufacturer = "Not set";
    std::string model = "Not set";
    if (!hasFailed()) {
        // Connect to the root\cimv2 namespace with the current user and obtain pointer pSvc to make IWbemServices calls.
        hResult = locator->ConnectServer(L"ROOT\\CIMV2", nullptr, nullptr, 0, NULL, 0, 0, &services);

        if (!hasFailed()) {
            // Set the IWbemServices proxy so that impersonation of the user (client) occurs.
            hResult = CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
                RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

            if (!hasFailed()) {
                IEnumWbemClassObject* classObjectEnumerator = nullptr;
                hResult = services->ExecQuery(L"WQL", L"SELECT * FROM Win32_ComputerSystem", WBEM_FLAG_FORWARD_ONLY |
                    WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &classObjectEnumerator);
                if (!hasFailed()) {
                    IWbemClassObject *classObject;
                    ULONG uReturn = 0;
                    hResult = classObjectEnumerator->Next(WBEM_INFINITE, 1, &classObject, &uReturn);
                    if (uReturn != 0) {
                        manufacturer = getValue(classObject, (LPCWSTR)L"Manufacturer");
                        model = getValue(classObject, (LPCWSTR)L"Model");
                    }
                    classObject->Release();
                }
                classObjectEnumerator->Release();
            }
        }
    }

    if (locator) {
        locator->Release();
    }
    if (services) {
        services->Release();
    }
    CoUninitialize();
    return { manufacturer, model };
}

bool isVirtualInternal() {
    const auto result = getComputerManufacturerAndModel();
    QString str = QString::fromStdString(result.second);
    str = str.toLower();

    const std::set<QString> virtualNames = {"VirtualBox", "Oracle", "VMware", "Parallels", "Virtual"};
    for (const QString &name: virtualNames) {
        if (str.contains(name.toLower())) {
            return true;
        }
    }
    return false;
}

#endif // _WIN32
