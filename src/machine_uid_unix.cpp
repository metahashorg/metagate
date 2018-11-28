#ifndef _WIN32

#include <machine_uid.h>

#include <vector>
#include <algorithm>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <net/if.h>

#ifdef TARGET_OS_MAC
#include <net/if_dl.h>
#include <ifaddrs.h>
#include <net/if_types.h>
#else //!TARGET_OS_MAC
// #include <linux/if.h>
// #include <linux/sockios.h>
#endif //!TARGET_OS_MAC

#include <string>

static const char* getMachineName()
{
    static struct utsname u;

    if ( uname( &u ) < 0 )
    {
        assert(0);
        return "unknown";
    }

    return u.nodename;
}

static std::string osNameImpl() {
    struct utsname uts;
    uname(&uts);
    return std::string(uts.sysname) + std::string(uts.release);
}

//---------------------------------get MAC addresses ------------------------------------unsigned short-unsigned short----------
// we just need this for purposes of unique machine id. So any one or two mac's is fine.
static unsigned short hashMacAddress( unsigned char* mac )
{
    unsigned short hash = 0;

    for ( unsigned int i = 0; i < 6; i++ )
    {
        hash += ( mac[i] << (( i & 1 ) * 8 ));
    }
    return hash;
}

static void getMacHash( unsigned short& mac1, unsigned short& mac2 )
{
    mac1 = 0;
    mac2 = 0;

    std::vector<unsigned short> addrs;

#ifdef TARGET_OS_MAC

    struct ifaddrs* ifaphead;
    if ( getifaddrs( &ifaphead ) != 0 )
        return;

    // iterate over the net interfaces
    struct ifaddrs* ifap;
    for ( ifap = ifaphead; ifap; ifap = ifap->ifa_next )
    {
        struct sockaddr_dl* sdl = (struct sockaddr_dl*)ifap->ifa_addr;
        if ( sdl && ( sdl->sdl_family == AF_LINK ) && ( sdl->sdl_type == IFT_ETHER ))
        {
            addrs.emplace_back(hashMacAddress( (unsigned char*)(LLADDR(sdl))));
        }
    }

    freeifaddrs( ifaphead );

#else // !TARGET_OS_MAC

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP );
    if ( sock < 0 ) return;

    // enumerate all IP addresses of the system
    struct ifconf conf;
    char ifconfbuf[ 128 * sizeof(struct ifreq)  ];
    memset( ifconfbuf, 0, sizeof( ifconfbuf ));
    conf.ifc_buf = ifconfbuf;
    conf.ifc_len = sizeof( ifconfbuf );
    if ( ioctl( sock, SIOCGIFCONF, &conf ))
    {
        assert(0);
        return;
    }

    // get MAC address
    struct ifreq* ifr;
    for ( ifr = conf.ifc_req; (char*)ifr < (char*)conf.ifc_req + conf.ifc_len; ifr++ )
    {
        if ( ifr->ifr_addr.sa_data == (ifr+1)->ifr_addr.sa_data )
            continue;  // duplicate, skip it

        if ( ioctl( sock, SIOCGIFFLAGS, ifr ))
            continue;  // failed to get flags, skip it
        if ( ioctl( sock, SIOCGIFHWADDR, ifr ) == 0 )
        {
            addrs.emplace_back(hashMacAddress( (unsigned char*)&(ifr->ifr_addr.sa_data)));
        }
    }

    close( sock );

#endif // !TARGET_OS_MAC

    if (addrs.empty()) {
        return;
    }
    const auto pair = std::minmax_element(addrs.begin(), addrs.end());
    mac1 = *pair.first;
    mac2 = *pair.second;
}

static unsigned short getVolumeHash()
{
    // we don't have a 'volume serial number' like on windows. Lets hash the system name instead.
    unsigned char* sysname = (unsigned char*)getMachineName();
    unsigned short hash = 0;

    for ( unsigned int i = 0; sysname[i]; i++ )
        hash += ( sysname[i] << (( i & 1 ) * 8 ));

    return hash;
}

#ifdef TARGET_OS_MAC
#include <mach-o/arch.h>
static unsigned short getCpuHash()
{
    const NXArchInfo* info = NXGetLocalArchInfo();
    unsigned short val = 0;
    val += (unsigned short)info->cputype;
    val += (unsigned short)info->cpusubtype;
    return val;
}

#else // !TARGET_OS_MAC

static void getCpuid( unsigned int* p, unsigned int ax )
{
    __asm __volatile
            (   "movl %%ebx, %%esi\n\t"
                "cpuid\n\t"
                "xchgl %%ebx, %%esi"
                : "=a" (p[0]), "=S" (p[1]),
            "=c" (p[2]), "=d" (p[3])
        : "0" (ax)
        );
}

static unsigned short getCpuHash()
{
    unsigned int cpuinfo[4] = { 0, 0, 0, 0 };
    getCpuid( cpuinfo, 0 );
    unsigned short hash = 0;
    unsigned int* ptr = (&cpuinfo[0]);
    for ( unsigned int i = 0; i < 4; i++ )
        hash += (ptr[i] & 0xFFFF) + ( ptr[i] >> 16 );

    return hash;
}
#endif // !TARGET_OS_MAC

std::string getMachineUidInternal() {
    std::string result;
    result += std::to_string(getCpuHash()) + std::string(";");
    result += std::to_string(getVolumeHash()) + std::string(";");
    unsigned short mac1, mac2;
    getMacHash(mac1, mac2);
    result += std::to_string(mac1) + std::to_string(mac2);
    return result;
}

#endif // _WIN32
