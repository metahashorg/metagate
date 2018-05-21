#ifndef PLATFORM_H
#define PLATFORM_H

#include <QString>

#if defined(Q_OS_WIN32)
const QString osName = "win";
#elif defined(Q_OS_LINUX)
const QString osName = "linux";
#elif defined(Q_OS_MACX)
const QString osName = "macx";
#else
const QString osName = "unknown";
#endif

#if defined(PRODUCTION)
const bool isProductionSetup = true;
#elif defined(DEVELOPMENT)
const bool isProductionSetup = false;
#else
#error Define PRODUCTION or DEVELOPMENT macros!
#endif

#endif // PLATFORM_H
