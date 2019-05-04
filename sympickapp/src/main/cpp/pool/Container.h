//
// Created by yananh on 2019/3/28.
//

#ifndef NATIVEHOOK_CONTAINER_H
#define NATIVEHOOK_CONTAINER_H


class Container {
private:
    unsigned inner1;

    int innerFunc();

    int innerFunc1();

    double innerVar2;
public:
    Container();

    int outVar1;
    short outVar3;
    unsigned outVar;
    int outFunc1();
    int outFunc();

    double outVar2;
};


#endif //NATIVEHOOK_CONTAINER_H
