#pragma once

#include <functional>
#include <windows.h> 

class CCheckUpdates
{
public:
    CCheckUpdates();
    ~CCheckUpdates();

    static void check(const std::function<void(PWSTR, DWORD)> &logger, bool silency);
protected:
    static std::string run_command(LPTSTR cmd, const std::function<void(PWSTR pszFunction, DWORD dwError)> &logger);
};

