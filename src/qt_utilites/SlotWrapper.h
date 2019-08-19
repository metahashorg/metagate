#ifndef SLOTWRAPPER_H
#define SLOTWRAPPER_H

#include "check.h"
#include "Log.h"
#include "TypedException.h"

template<class Function>
void slotWrapper(const std::string &file, const Function &function) {
    try {
        function();
    } catch (const Exception &e) {
        LOG2(file) << "Error " << e;
    } catch (const std::exception &e) {
        LOG2(file) << "Error " << e.what();
    } catch (const TypedException &e) {
        LOG2(file) << "Error typed " << e.description;
    } catch (...) {
        LOG2(file) << "Unknown error";
    }
}

#define BEGIN_SLOT_WRAPPER \
    slotWrapper(__FILE__, [&]{
#define END_SLOT_WRAPPER \
    });

#endif // SLOTWRAPPER_H
