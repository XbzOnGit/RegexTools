#ifndef __NFA_H__
#define __NFA_H__
#include<stddef.h>
struct NFA_oneacc;
struct NFA_oneacc* Regex2NFA(const char* regex_str);
int NFASimu(struct NFA_oneacc* nfa, const char* input_str, size_t lenofstr);
void free_NFA(struct NFA_oneacc* nfa);
//About memory management.
//It does not take ownership for strings.
#endif