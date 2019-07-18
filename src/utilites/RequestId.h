#ifndef REQUESTID_H
#define REQUESTID_H

class RequestId {
public:

    size_t get() {
        return id++;
    }

private:

    size_t id = 0;

};

#endif // REQUESTID_H
