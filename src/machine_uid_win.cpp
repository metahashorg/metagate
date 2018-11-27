#ifdef _WIN32

#include "machine_uid.h"

#include <vector>
#include <algorithm>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <iphlpapi.h>
#include <lm.h>
#pragma comment(lib, "netapi32.lib")

bool GetWinMajorMinorVersion(DWORD& major, DWORD& minor)
{
    bool bRetCode = false;
    LPBYTE pinfoRawData = 0;
    if (NERR_Success == NetWkstaGetInfo(NULL, 100, &pinfoRawData))
    {
        WKSTA_INFO_100* pworkstationInfo = (WKSTA_INFO_100*)pinfoRawData;
        major = pworkstationInfo->wki100_ver_major;
        minor = pworkstationInfo->wki100_ver_minor;
        ::NetApiBufferFree(pinfoRawData);
        bRetCode = true;
    }
    return bRetCode;
}


std::string osNameImpl(){
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
    if (GetWinMajorMinorVersion(major, minor))
    {
        osver.dwMajorVersion = major;
        osver.dwMinorVersion = minor;
    }
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2)
    {
        OSVERSIONINFOEXW osvi;
        ULONGLONG cm = 0;
        cm = VerSetConditionMask(cm, VER_MINORVERSION, VER_EQUAL);
        ZeroMemory(&osvi, sizeof(osvi));
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        osvi.dwMinorVersion = 3;
        if (VerifyVersionInfoW(&osvi, VER_MINORVERSION, cm))
        {
            osver.dwMinorVersion = 3;
        }
    }

    GETSYSTEMINFO getSysInfo = (GETSYSTEMINFO)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetNativeSystemInfo");
    if (getSysInfo == NULL)  getSysInfo = ::GetSystemInfo;
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

    if (osver.wServicePackMajor != 0)
    {
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
static uint16_t hashMacAddress( PIP_ADAPTER_INFO info )
{
   uint16_t hash = 0;
   for ( uint32_t i = 0; i < info->AddressLength; i++ )
   {
      hash += ( info->Address[i] << (( i & 1 ) * 8 ));
   }
   return hash;
}

static void getMacHash( uint16_t& mac1, uint16_t& mac2 )
{
   mac1 = 0;
   mac2 = 0;
   IP_ADAPTER_INFO AdapterInfo[32];
   DWORD dwBufLen = sizeof( AdapterInfo );

   DWORD dwStatus = GetAdaptersInfo( AdapterInfo, &dwBufLen );
   if ( dwStatus != ERROR_SUCCESS )
      return; // no adapters.

   std::vector<uint16_t> addrs;
   PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
   while (pAdapterInfo) {
       addrs.emplace_back(hashMacAddress(pAdapterInfo));
       pAdapterInfo = pAdapterInfo->Next;
   }
   if (addrs.empty()) {
       return;
   }
   const auto pair = std::minmax_element(addrs.begin(), addrs.end());
   mac1 = *pair.first;
   mac2 = *pair.second;
}

static uint16_t getVolumeHash()
{
   DWORD serialNum = 0;

   // Determine if this volume uses an NTFS file system.
   const bool res = GetVolumeInformationA( LPCSTR("C:\\"), NULL, 0, &serialNum, NULL, NULL, NULL, 0);
   if (!res) {
       serialNum = 0;
   }
   uint16_t hash = (uint16_t)(( serialNum + ( serialNum >> 16 )) & 0xFFFF );

   return hash;
}

static uint16_t getCpuHash()
{
   int cpuinfo[4] = { 0, 0, 0, 0 };
   __cpuid( cpuinfo, 0 );
   uint16_t hash = 0;
   uint16_t* ptr = (uint16_t*)(&cpuinfo[0]);
   for ( uint32_t i = 0; i < 8; i++ )
      hash += ptr[i];

   return hash;
}

static std::string getMachineName()
{
   static char computerName[1024];
   DWORD size = 1024;
   GetComputerName( LPWSTR(computerName), &size );
   return std::string(computerName);
}

std::string getMachineUid() {
    std::string result;
    result += std::to_string(getCpuHash()) + std::string(";");
    result += std::to_string(getVolumeHash()) + std::string(";");
    unsigned short mac1, mac2;
    getMacHash(mac1, mac2);
    result += std::to_string(mac1) + std::to_string(mac2);
    return result;
}

#endif // _WIN32
