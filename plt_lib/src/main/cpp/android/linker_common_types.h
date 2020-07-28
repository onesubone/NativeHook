//
// Created by yananh on 2019/4/1.
//
#include <android/dlext.h>
#include "linked_list.h"

struct soinfo;

class SoinfoListAllocator {
public:
    static void* alloc();
    static void free(void* entry);

private:
    // unconstructable
    DISALLOW_IMPLICIT_CONSTRUCTORS(SoinfoListAllocator);
};

class NamespaceListAllocator {
public:
    static void* alloc();
    static void free(void* entry);

private:
    // unconstructable
    DISALLOW_IMPLICIT_CONSTRUCTORS(NamespaceListAllocator);
};

typedef LinkedList<soinfo, SoinfoListAllocator> soinfo_list_t;
typedef LinkedList<android_namespace_t, NamespaceListAllocator> android_namespace_list_t;