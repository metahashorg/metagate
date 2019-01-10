#ifndef MODULE_H
#define MODULE_H

#include <string>
#include <vector>

enum class StatusModule {
    wait, found, notFound
};

void initModules();

// threadSafe
// Модули должны быть добавлены в основном потоке до запуска QApplication
void addModule(const std::string &moduleName);

// threadSafe
void changeStatus(const std::string &moduleName, StatusModule status);

// threadSafe
std::vector<std::pair<std::string, StatusModule>> getStatusModules();

#endif // MODULES_H
