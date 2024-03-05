#ifndef __C_BITSET__
#define __C_BITSET__
struct Cbitset{
    char* pt;
    unsigned long long size;
};
int init_cbitset(struct Cbitset* cpt, unsigned long long csize);
void put_cbitset(struct Cbitset* cpt, unsigned long long pos);
void clear_cbitset(struct Cbitset* cpt, unsigned long long pos);
int test_cbitset(struct Cbitset* cpt, unsigned long long pos);
void free_bitset(struct Cbitset* cpt);
void clear_bitset(struct Cbitset* cpt);
#endif
