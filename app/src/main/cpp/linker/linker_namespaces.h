/*
 * Copyright (C) 2016 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include "linker_common_types.h"

struct soinfo;

struct android_namespace_link_t {
public:
    android_namespace_link_t(android_namespace_t* linked_namespace,
                             const std::unordered_set<std::string>& shared_lib_sonames,
                             bool allow_all_shared_libs)
            : linked_namespace_(linked_namespace), shared_lib_sonames_(shared_lib_sonames),
              allow_all_shared_libs_(allow_all_shared_libs)
    {}

    android_namespace_t* linked_namespace() const {
        return linked_namespace_;
    }

    const std::unordered_set<std::string>& shared_lib_sonames() const {
        return shared_lib_sonames_;
    }

    bool is_accessible(const char* soname) const {
        if (soname == nullptr) {
            return false;
        }
        return allow_all_shared_libs_ || shared_lib_sonames_.find(soname) != shared_lib_sonames_.end();
    }

    bool allow_all_shared_libs() const {
        return allow_all_shared_libs_;
    }

private:
    android_namespace_t* const linked_namespace_;
    const std::unordered_set<std::string> shared_lib_sonames_;
    bool allow_all_shared_libs_;
};

struct android_namespace_t {
public:
    android_namespace_t() {}

    soinfo_list_t get_global_group();
    soinfo_list_t get_shared_group();

private:
    const char* name_;
    bool is_isolated_;
    bool is_greylist_enabled_;
    std::vector<std::string> ld_library_paths_;
    std::vector<std::string> default_library_paths_;
    std::vector<std::string> permitted_paths_;
    // Loader looks into linked namespace if it was not able
    // to find a library in this namespace. Note that library
    // lookup in linked namespaces are limited by the list of
    // shared sonames.
    std::vector<android_namespace_link_t> linked_namespaces_;
    soinfo_list_t soinfo_list_;

    DISALLOW_COPY_AND_ASSIGN(android_namespace_t);
};
