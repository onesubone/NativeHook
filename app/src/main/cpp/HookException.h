//
// Created by yananh on 2019/4/8.
//

#ifndef NATIVEHOOK_FALTALEXCEPTION_H
#define NATIVEHOOK_FALTALEXCEPTION_H

#include <string>

#define CODE_OK 0
#define CODE_ERR -1

using namespace std;

class HookException {
private:
    std::string error;
    int code;
public:
    HookException(const char *err) : error(err), code(CODE_ERR) {

    }

    HookException(std::string &err) : error(err), code(CODE_ERR) {

    }

    HookException(const char *err, int id) : error(err), code(id) {

    }

    HookException(std::string &err, int id) : error(err), code(id) {

    }
};


#endif //NATIVEHOOK_FALTALEXCEPTION_H
