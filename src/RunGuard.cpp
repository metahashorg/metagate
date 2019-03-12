#include "RunGuard.h"

#include <QCryptographicHash>


namespace
{

QString generateKeyHash( const QString& key, const QString& salt )
{
    QByteArray data;

    data.append( key.toUtf8() );
    data.append( salt.toUtf8() );
    data = QCryptographicHash::hash( data, QCryptographicHash::Sha1 ).toHex();

    return data;
}

}


RunGuard::RunGuard( const QString& key )
    : key( key )
    , memLockKey( generateKeyHash( key, "_memLockKey" ) )
    , sharedmemKey( generateKeyHash( key, "_sharedmemKey" ) )
    , sharedMem( sharedmemKey )
    , memLock( memLockKey, 1 )
{
    memLock.acquire();
    {
        QSharedMemory fix( sharedmemKey );    // Fix for *nix: http://habrahabr.ru/post/173281/
        fix.attach();
    }
    memLock.release();
}

RunGuard::~RunGuard()
{
    release();
}

bool RunGuard::isAnotherRunning()
{
    if ( sharedMem.isAttached() )
        return false;

    memLock.acquire();
    const bool isRunning = sharedMem.attach();
    if ( isRunning )
        sharedMem.detach();
    memLock.release();

    return isRunning;
}

bool RunGuard::tryToRun()
{
    if ( isAnotherRunning() )   // Extra check
        return false;

    memLock.acquire();
    const bool result = sharedMem.create(4096);
    char *eraseData = static_cast<char*>(sharedMem.data());
    std::fill(eraseData, eraseData + sharedMem.size(), 0);
    memLock.release();
    if ( !result )
    {
        release();
        return false;
    }

    return true;
}

void RunGuard::storeValue(const std::string &value) {
    if (isAnotherRunning()) {
        memLock.acquire();
        const bool success = sharedMem.attach();
        if (success && value.size() <= sharedMem.size() + 10) {
            char *data = static_cast<char*>(sharedMem.data());
            if (data != nullptr) {
                const std::string strInt = std::to_string(value.size());
                const std::string delimiter = ";";
                const std::string header = strInt + delimiter;
                std::copy(header.data(), header.data() + header.size(), data);
                std::copy(value.data(), value.data() + value.size(), data + header.size());
            }
        }
        sharedMem.detach();
        memLock.release();
    }
}

std::string RunGuard::getValueAndReset() {
    std::string result;
    memLock.acquire();
    if (sharedMem.isAttached()) {
        if (sharedMem.size() >= 10) {
            const char *data = static_cast<const char*>(sharedMem.data());
            const char *endInt = data + 10;
            const char *found = std::find(data, endInt, ';');
            if (found != endInt) {
                const std::string sizeStr(data, found);
                const size_t size = std::stoull(sizeStr);
                if (size <= sharedMem.size() + 10) {
                    result = std::string(found + 1, found + size);
                }
            }
        }

        char *eraseData = static_cast<char*>(sharedMem.data());
        std::fill(eraseData, eraseData + sharedMem.size(), 0);
    }
    memLock.release();
    return result;
}

void RunGuard::release()
{
    memLock.acquire();
    if ( sharedMem.isAttached() )
        sharedMem.detach();
    memLock.release();
} 
