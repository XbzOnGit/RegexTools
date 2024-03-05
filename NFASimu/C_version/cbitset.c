#include"cbitset.h"
#include<stdio.h>
#include<assert.h>
#include<stdlib.h>
#include<string.h>
int init_cbitset(struct Cbitset* cpt, unsigned long long csize){
    assert(cpt != NULL);
    assert(csize > 0);
    if((csize + 7) < csize){
        fprintf(stderr,"csize + 7 exceeds unsiged long long\n");
        return -1;
    }
    //Assume (at least)8 bits in a byte.
    unsigned long long byte_needed = (csize + 7)/8;
    cpt->pt = calloc(byte_needed,1);
    if(cpt->pt == NULL){
        fprintf(stderr,"Fail to calloc %llu bytes for a bitset\n",byte_needed);
        return -1;
    }
    cpt->size = csize;
    return 0;
}
void put_cbitset(struct Cbitset* cpt, unsigned long long pos){
    assert(cpt != NULL);
    assert(pos < cpt->size);
    unsigned long long byte_pos = pos / 8;
    unsigned long long left_with = pos % 8;
    (*(cpt->pt + byte_pos)) |= (1<<left_with);
    return;
}
void clear_cbitset(struct Cbitset* cpt, unsigned long long pos){
    assert(cpt != NULL);
    assert(pos < cpt->size);
    unsigned long long byte_pos = pos / 8;
    unsigned long long left_with = pos % 8;
    (*(cpt->pt + byte_pos)) &= (~(1<<left_with));
    return;
}
int test_cbitset(struct Cbitset* cpt, unsigned long long pos){
    assert(cpt != NULL);
    assert(pos < cpt->size);
    unsigned long long byte_pos = pos / 8;
    unsigned long long left_with = pos % 8;
    return ((((*(cpt->pt + byte_pos)) >> left_with) & 0x1) != 0);
}
void free_bitset(struct Cbitset* cpt){
    if(cpt != NULL){
        free(cpt->pt);
    }
    return;
}
void clear_bitset(struct Cbitset* cpt){
    assert(cpt != NULL);
    unsigned long long byteall = (cpt->size + 7)/8;
    memset(cpt->pt,0,byteall);
    return;
}