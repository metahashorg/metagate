#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#include <io.h>
#include <vector>
#else
#ifndef S_SPLINT_S /* Including this here triggers a known bug in splint */
#include <unistd.h>
#endif
#endif

#ifdef _WIN32
int libscrypt_salt_gen(uint8_t *salt, size_t len)
{
        std::vector<unsigned char> buf(len);
        static HCRYPTPROV provider;
        if (!CryptAcquireContext(&provider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        {
                return -1;
        }

        if (!CryptGenRandom(provider, len, buf.data()))
        {
                return -1;
        }
        memcpy(salt, buf.data(), len);
        return 0;
}
#else
extern "C" {
int libscrypt_salt_gen(uint8_t *salt, size_t len)
{
	unsigned char buf[len];
	size_t data_read = 0;
    int urandom = open("/dev/urandom", O_RDONLY);

	if (urandom < 0)
	{
		return -1;
	}

	while (data_read < len) {
		ssize_t result = read(urandom, buf + data_read, len - data_read);

		if (result < 0)
		{
			if (errno == EINTR || errno == EAGAIN) {
				continue;
			}

			else {
				(void)close(urandom);
				return -1;
			}
		}

		data_read += result;
	}

	/* Failures on close() shouldn't occur with O_RDONLY */
	(void)close(urandom);

	memcpy(salt, buf, len);

	return 0;
}
}
#endif
