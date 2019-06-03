//
// Created by yananh on 2019/5/26.
//

#ifndef RUBICK_ELF_HOOK_H
#define RUBICK_ELF_HOOK_H

struct hook_symbol {
    const char *so_name;
    const char *symbol_name;
    void *new_value;
};

void hook(std::vector<hook_symbol*> sym_list);
#endif //RUBICK_ELF_HOOK_H
