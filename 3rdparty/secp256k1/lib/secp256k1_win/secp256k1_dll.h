#ifndef SECP256K1_DLL_H
#define SECP256K1_DLL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BUILDING_DLL
#define THE_DLL __declspec(dllexport)
#else
#define THE_DLL __declspec(dllimport)
#endif

int __stdcall THE_DLL EC_Verify(unsigned char *msg, int msglen,
	unsigned char *sig, int siglen,
	unsigned char *pubkey, int pubkeylen);

//int __stdcall THE_DLL EC_Init();

#ifdef __cplusplus
}
#endif

#endif  // SECP256K1_DLL_H
