#include<stdio.h>
#include<stdlib.h>
#include<limits.h>
#include<stdint.h>
#include<assert.h>
#include"cbitset.h"

//Optimization opportunity: When state stack size << node_number/8,
//we can undo those changes instead of calling clear_bitset.  

#define HASH_TABLE_SIZE (32)

FILE* input_stream = NULL;
FILE* nfa_stream = NULL;
unsigned long long node_number;
unsigned long long q_start;
struct Cedges{
    struct Cedges* next;
    unsigned long long des;
    int symbol;
    int is_eps;
};

struct ChashTable{
    struct Cedges* hash_positions[HASH_TABLE_SIZE];
};
//Every node should have one hash table.
struct ChashTable* node_hash_tables;

void free_chashtable(){
    //Do not free node_hash_tables itself.
    unsigned long long i;
    for(i=0;i<node_number;++i){
        unsigned long long j;
        for(j=0;j<HASH_TABLE_SIZE;++j){
            struct Cedges* pt = node_hash_tables[i].hash_positions[j];
            while(pt != NULL){
                struct Cedges* save = pt;
                pt = pt->next;
                free(save);
            }
        }
    }
}

struct Cbitset final_stages_bitset;

struct Cbitset alReadyOn;//A bitset for Newstats.
unsigned long long* newStates;
unsigned long long newPt = 0;
unsigned long long* oldStates;
unsigned long long oldPt = 0;


void error_exit(void){
    if(nfa_stream != NULL){
        fclose(nfa_stream);
    }
    if(input_stream != NULL){
        fclose(input_stream);
    }
    //Do not need to free memory.
    //exit immediately.
    exit(1);
}
unsigned long long get_non_negative_number_nfa(){
    //Only nfa parsing needs this.
    unsigned long long num = 0;
    int ch;
    do
    {
        ch = fgetc(nfa_stream);
    } while(!(ch >= '0' && ch <= '9'));
    while(ch >= '0' && ch <= '9'){
        int chdiff = ch - '0';
        unsigned long long tenbar = ULLONG_MAX - chdiff;
        //10*num should <= tenbar here.
        //num should be <= tenbar/10.
        //<=eq=> num <= floor(tenbar/10).
        unsigned long long bar = tenbar/10;
        if(num > bar){
            fprintf(stderr,"An unsigned integer exceeds unsigned long long\n");
            error_exit();
        }
        num = num * 10 + chdiff;
        ch = fgetc(nfa_stream);
    }
    return num;
}

void addEdge(unsigned long long src, unsigned long long des, int symbol, int is_eps){
    assert(symbol >= 0);
    assert(HASH_TABLE_SIZE > 1);
    int key = symbol % (HASH_TABLE_SIZE-1);
    if(is_eps){
        key = HASH_TABLE_SIZE - 1;//Make a special channel for epsilon.
    }
    struct Cedges* anedge = calloc(1,sizeof(struct Cedges));
    if(anedge == NULL){
        fprintf(stderr,"Fail to calloc struct Cedges in addEdge\n");
        error_exit();
    }
    anedge->des = des;
    anedge->is_eps = is_eps;
    anedge->symbol = symbol;
    anedge->next = node_hash_tables[src].hash_positions[key];
    node_hash_tables[src].hash_positions[key] = anedge;
    return;
}


void parse_a_graph_tuple(unsigned long long node_idx){
    unsigned long long first_len = get_non_negative_number_nfa();
    //The , should have been removed.
    unsigned long long sec_len = get_non_negative_number_nfa();
    assert(first_len == 1 || first_len == 0);//Currently only support ASCII.
    int is_eps = (first_len == 0);
    int chsymbol = 0;
    if(first_len == 1){
        chsymbol = fgetc(nfa_stream);
    }
    unsigned long long desno = 0;
    unsigned long long i;
    for(i=0;i<sec_len;++i){
        int ch = fgetc(nfa_stream);
        int chdiff = ch - '0';
        unsigned long long tenbar = ULLONG_MAX - chdiff;
        //10*num should <= tenbar here.
        //num should be <= tenbar/10.
        //<=eq=> num <= floor(tenbar/10).
        unsigned long long bar = tenbar/10;
        if(desno > bar){
            fprintf(stderr,"An unsigned integer exceeds unsigned long long\n");
            error_exit();
        }
        desno = desno * 10 + chdiff;
    }
    addEdge(node_idx,desno,chsymbol,is_eps);
    fgetc(nfa_stream);//A space or newline.
    return;
}
void parse_graph_edge_line(unsigned long long node_idx){
    unsigned long long tempecnt = get_non_negative_number_nfa();
    unsigned long long i;
    for(i=0;i<tempecnt;++i){
        parse_a_graph_tuple(node_idx);
    }
}


void parse_nfa(void){
    node_number = get_non_negative_number_nfa();
    if(node_number == 0){
        fprintf(stderr,"Node number is 0\n");
        error_exit();
    }
    q_start = get_non_negative_number_nfa();
    if(!(q_start>=0 && q_start<node_number)){
        fprintf(stderr,\
        "q_start is %llu, while node_number is %llu, which does not meet q_start >= 0 && q_start < node_number\n"\
        ,q_start,node_number);
        error_exit();
    }
    if(node_number > SIZE_MAX){
        fprintf(stderr,"node_number is %llu, which is > SIZE_MAX\n",node_number);
        error_exit();
    }
    node_hash_tables = calloc(node_number,sizeof(struct ChashTable));
    if(node_hash_tables == NULL){
        fprintf(stderr,"Fail to calloc %llu struct ChashTable\n",node_number);
        error_exit();
    }
    unsigned long long n_acc = get_non_negative_number_nfa();
    if(n_acc > SIZE_MAX){
        fprintf(stderr,"n_acc is %llu, which is > SIZE_MAX\n",n_acc);
        error_exit();
    }
    if(init_cbitset(&final_stages_bitset,node_number)!=0){
        fprintf(stderr,"Fail to init cbitset of final_stages_bitset\n");
        error_exit();
    }
    unsigned long long i;
    for(i=0;i<n_acc;++i){
        unsigned long long accno = get_non_negative_number_nfa();
        if(!(accno >= 0 && accno < node_number)){
            fprintf(stderr,\
            "accno is %llu, while node_number is %llu, which does not meet accno >= 0 && accno < node_number\n"\
            ,accno,node_number);
            error_exit();
        }
        put_cbitset(&final_stages_bitset,accno);
    }
    //Now add edges.
    for(i=0;i<node_number;++i){
        parse_graph_edge_line(i);
    }
}
void addState(unsigned long long node_idx, struct Cbitset* bpt, unsigned long long* thestack, unsigned long long* spt){
    thestack[(*spt)] = node_idx;
    ++(*spt);
    put_cbitset(bpt,node_idx);
    struct Cedges* pt = node_hash_tables[node_idx].hash_positions[HASH_TABLE_SIZE-1];
    while(pt != NULL){
        if(!test_cbitset(bpt,pt->des)){
            addState(pt->des,bpt,thestack,spt);
        }
        pt = pt->next;
    }
    return;
}
int run_NFA(void){
    assert(HASH_TABLE_SIZE > 1);
    if(init_cbitset(&alReadyOn,node_number)!=0){
        fprintf(stderr,"Fail to init alReadyOn\n");
        error_exit();
    }
    assert(node_number <= (ULLONG_MAX/sizeof(unsigned long long)));
    assert(node_number <= (SIZE_MAX/sizeof(unsigned long long)));
    oldStates = (unsigned long long*)malloc(sizeof(unsigned long long)*node_number);
    if(oldStates == NULL){
        fprintf(stderr,"Fail to malloc oldStates\n");
        error_exit();
    }
    newStates = (unsigned long long*)malloc(sizeof(unsigned long long)*node_number);
    if(newStates == NULL){
        fprintf(stderr,"Fail to malloc newStates\n");
        error_exit();
    }
    addState(q_start,&alReadyOn,oldStates,&oldPt);
    clear_bitset(&alReadyOn);
    int ch = fgetc(input_stream);
    while(ch != EOF){
        //alReadyOn is empty, newStates && newPt empty.
        //OldStates and OldPt are the closure now.
        unsigned long long j;
        int key = ch % (HASH_TABLE_SIZE - 1);//Not eps.
        for(j=0;j<oldPt;++j){
            unsigned long long nidx = oldStates[j];
            struct Cedges* pt = node_hash_tables[nidx].hash_positions[key];
            while(pt != NULL){
                if(pt->symbol == ch){
                    if(!test_cbitset(&alReadyOn,pt->des)){
                        addState(pt->des,&alReadyOn,newStates,&newPt);
                    }
                }
                pt = pt->next;
            }
        }
        clear_bitset(&alReadyOn);
        unsigned long long* templpt = oldStates;
        oldStates = newStates;
        newStates = templpt;
        oldPt = newPt;
        newPt = 0;
        ch = fgetc(input_stream);
    }
    int retans = 0;
    unsigned long long i;
    for(i=0;i<oldPt;++i){
        if(test_cbitset(&final_stages_bitset,oldStates[i])){
            retans = 1;
            goto clean_and_return;
        }
    }
clean_and_return:
    free_bitset(&alReadyOn);
    free(oldStates);
    free(newStates);
    return retans;
}
int main(int argc, char** argv){
    if(argc != 3){
        fprintf(stderr,"Usage: ./NFASimu nfa.txt xxx.txt");
        return 1;
    }
    nfa_stream = fopen(argv[1],"r");
    if(nfa_stream == NULL){
        fprintf(stderr,"Fail to open %s\n",argv[1]);
        return 1;
    }
    input_stream = fopen(argv[2],"r");
    if(input_stream == NULL){
        fprintf(stderr,"Fail to open %s\n",argv[2]);
        fclose(nfa_stream);
        return 1;
    }
    parse_nfa();
    fclose(nfa_stream);
    nfa_stream = NULL;
    if(run_NFA()){
        printf("Accepted\n");
    }else{
        printf("Rejected\n");
    }
    fclose(input_stream);
    free_chashtable();
    free(node_hash_tables);
    free_bitset(&final_stages_bitset);
    return 0;
}