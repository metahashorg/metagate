#ifndef GIT_SHA1_H_
#define GIT_SHA1_H_

#include <string>

extern const char g_GIT_SHA1[]; 
extern const char g_GIT_REFSPEC[];
extern const char g_GIT_IS_LOCAL_CHANGES[];

std::string g_git_sha1();

#endif //
