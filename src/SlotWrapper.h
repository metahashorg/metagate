#ifndef SLOTWRAPPER_H
#define SLOTWRAPPER_H

#include "check.h"
#include "Log.h"
#include "TypedException.h"

template<class Function>
void slotWrapper(const Function &function) {
    try {
        function();
    } catch (const Exception &e) {
        LOG << "Error " << e;
    } catch (const std::exception &e) {
        LOG << "Error " << e.what();
    } catch (const TypedException &e) {
        LOG << "Error typed " << e.description;
    } catch (...) {
        LOG << "Unknown error";
    }
}

#define BEGIN_SLOT_WRAPPER \
    slotWrapper([&]{
#define END_SLOT_WRAPPER \
    });

#endif // SLOTWRAPPER_H
