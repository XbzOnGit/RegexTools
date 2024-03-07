#include "nfa.h"
#include "cbitset.h"
#include<assert.h>
#include<stdio.h>
#include<stddef.h>
#include<stdint.h>
#include<limits.h>
#include<stdlib.h>

//About ownership.
//Dynamic memory includes NFA, nodes, edges.
//NFA and nodes inside has different lifetime.
//In NFA_or and NFA_concat, the second NFA is freed, but nodes are not.
//Do not free the edges for the same
//Nodes only increase before you call NFA_free to free them all.


//For the grammar and the correctness of implementation, check python_version.  


#define NFA_HASH_SIZE (129)
//Designed for ASCII, which has only 128 in total.
//% (NFA_HASH_SIZE - 1) is % 128 --> 0 to 127.
//Make a special channel for epsilon.
struct NFA_HASH_TABLE{
    struct NFA_edge* hash_positions[NFA_HASH_SIZE];
};
struct NFA_state{
    struct NFA_HASH_TABLE edge_hashtable;
    //No need to cache epsilon closure for a state.
    //DFS to get it.
    //The problem is how to maintain set of epsilon closure.
    //if a hash table or bitset, can find the closure of any set in O(m+n).
    struct NFA_state* next_in_nfalist;//linked list for all states in one NFA.

    unsigned long long id;//Generted ID from 0 to N-1.
    //This is needed for a bitset.
};

struct NFA_state_stack{
    struct NFA_state** stack;
    unsigned long long ssize;
    unsigned long long spt;
};
struct NFA_edge{
    struct NFA_edge* next;
    int symbol;
    int is_eps;
    struct NFA_state* des;
};
//NFA with one accepting state.
//When converting regex into NFA, I will keep it in this form.
//Note that you do not need a string name for the state.
//This is what will be generated in parse tree.
struct NFA_oneacc{
    struct NFA_state* q_start;
    //To make operations on graph O(1), use a linked list.
    struct NFA_state* states_head;
    struct NFA_state* states_tail;
    //Just a linked list of NFA_states in this NFA.
    //Easy to merge two NFA.
    struct NFA_state* accepted_q;//The NFA constructed here has only one accepting state.
    unsigned long long states_cnt;//Only set before returning to user.
};


static void NFA_state_stack_init(struct NFA_state_stack* sta, unsigned long long ssize){
    assert(sta!=NULL);
    assert(ssize > 0);
    assert(ssize <= (ULLONG_MAX/sizeof(struct NFA_state*)));
    assert(ssize <= (SIZE_MAX/sizeof(struct NFA_state*)));
    sta->stack = (struct NFA_state**)malloc(ssize * sizeof(struct NFA_state*));
    if(sta->stack == NULL){
        fprintf(stderr,"Fail to malloc NFA_state stack\n");
        exit(1);
    }
    sta->ssize = ssize;
    sta->spt = 0;
    return;
}
static void NFA_state_stack_push(struct NFA_state_stack* sta, struct NFA_state* itpt){
    //Assume not overflow, no check here.
    sta->stack[sta->spt] = itpt;
    ++sta->spt;
    return;
}
static void NFA_state_stack_clear(struct NFA_state_stack* sta){
    sta->spt = 0;
    return;
}
static struct NFA_state* NFA_state_stack_pop(struct NFA_state_stack* sta){
    //No check here.
    --sta->spt;
    return sta->stack[sta->spt];
}
static int NFA_state_stack_empty(struct NFA_state_stack* sta){
    return sta->spt == 0;
}
static void NFA_state_stack_free(struct NFA_state_stack* sta){
    //Do not free itself.
    free(sta->stack);
    sta->spt = 0;
    sta->ssize = 0;
    return;
}
static void NFA_state_stack_swap(struct NFA_state_stack* onept, struct NFA_state_stack* twopt){
    assert(onept->ssize == twopt->ssize);//All stacks of the same max size.
    struct NFA_state** temppt = onept->stack;
    onept->stack = twopt->stack;
    twopt->stack = temppt;
    unsigned long long tnum = onept->spt;
    onept->spt = twopt->spt;
    twopt->spt = tnum;
    return;
}
struct Regex2NFA_converter{
    const char* regex_str;
    size_t reglen;
    size_t pospt;
    unsigned long long nfa_id_no;
};
static char R2N_peek(struct Regex2NFA_converter* p){
    assert(p->pospt < p->reglen);
    return p->regex_str[p->pospt];
}
//Only move forward.
static void R2N_move(struct Regex2NFA_converter* p, size_t off){
    size_t nextnum = p->pospt + off;
    if(nextnum < p->pospt){
        fprintf(stderr,"In R2N_move, overflowed size_t after adding off, %zu + %zu\n",p->pospt,off);
        exit(1);
    }
    p->pospt = nextnum;
    return;
}
static int R2N_more(struct Regex2NFA_converter* p){
    return (p->pospt < p->reglen);
}
static void assign_nfa_id(struct Regex2NFA_converter* p, struct NFA_state* spt){
    unsigned long long nxt = p->nfa_id_no + 1;
    if(nxt <= p->nfa_id_no){
        fprintf(stderr,"nfa_id_no overflowed, from %llu to %llu\n",p->nfa_id_no,nxt);
        exit(1);
    }
    spt->id = p->nfa_id_no;
    p->nfa_id_no = nxt;
    return;
}


static struct NFA_oneacc* R2N_nfa_empty_string(struct Regex2NFA_converter* p){
    struct NFA_oneacc* npt = malloc(sizeof(struct NFA_oneacc));
    if(npt == NULL){
        fprintf(stderr,"Fail to malloc NFA_oneacc in R2N_nfa_empty_string\n");
        exit(1);
    }
    struct NFA_state* spt = calloc(1,sizeof(struct NFA_state));
    if(spt == NULL){
        fprintf(stderr,"Fail to malloc NFA_state in R2N_nfa_empty_string\n");
        exit(1);
    }
    assign_nfa_id(p,spt);
    spt->next_in_nfalist = NULL;
    npt->q_start = spt;
    npt->accepted_q = spt;
    npt->states_head = spt;
    npt->states_tail = spt;
    return npt;
}

static void NFA_add_edge(struct NFA_state* frompt, struct NFA_state* topt, int symbol, int is_eps){
    assert(NFA_HASH_SIZE > 1);
    struct NFA_edge* ept = malloc(sizeof(struct NFA_edge));
    if(ept == NULL){
        fprintf(stderr,"Fail to malloc NFA_edge in NFA_add_edge\n");
        exit(1);
    }
    ept->des = topt;
    ept->is_eps = is_eps;
    ept->symbol = symbol;
    int key = symbol % (NFA_HASH_SIZE - 1);
    if(is_eps){
        key = NFA_HASH_SIZE - 1;
    }
    ept->next = frompt->edge_hashtable.hash_positions[key];
    frompt->edge_hashtable.hash_positions[key] = ept;
    return;
}

static void NFA_add_state(struct NFA_oneacc* nfapt, struct NFA_state* addedpt){
    assert(nfapt->states_head != NULL);
    //Tail is not changed.
    //Only called on nfas with at least one state.
    addedpt->next_in_nfalist = nfapt->states_head;
    nfapt->states_head = addedpt;
    return;
}
//O(1)
static void NFA_merge(struct NFA_oneacc* nonept, struct NFA_oneacc* ntwopt){
    //Merge two into one.
    //Just merge the list of states.
    assert(nonept->states_tail->next_in_nfalist == NULL);
    nonept->states_tail->next_in_nfalist = ntwopt->states_head;
    nonept->states_tail = ntwopt->states_tail;
    return;
}
static struct NFA_oneacc* R2N_nfa_onesymbol(struct Regex2NFA_converter* p, int symbol){
    struct NFA_oneacc* npt = malloc(sizeof(struct NFA_oneacc));
    if(npt == NULL){
        fprintf(stderr,"Fail to malloc NFA_oneacc in R2N_nfa_onesymbol\n");
        exit(1);
    }
    struct NFA_state* sonept = calloc(1,sizeof(struct NFA_state));
    if(sonept == NULL){
        fprintf(stderr,"Fail to malloc the first NFA_state in R2N_nfa_onesymbol\n");
        exit(1);
    }
    struct NFA_state* stwopt = calloc(1,sizeof(struct NFA_state));
    if(stwopt == NULL){
        fprintf(stderr,"Fail to malloc the second NFA_state in R2N_nfa_onesymbol\n");
        exit(1);
    }
    assign_nfa_id(p,sonept);
    assign_nfa_id(p,stwopt);
    sonept->next_in_nfalist = stwopt;
    stwopt->next_in_nfalist = NULL;
    npt->states_head = sonept;
    npt->q_start = sonept;
    npt->accepted_q = stwopt;
    npt->states_tail = stwopt;
    //Add one edge.
    NFA_add_edge(sonept,stwopt,symbol,0);
    return npt;
}

static void R2N_convert_init(struct Regex2NFA_converter* p, const char* regex_str){
    assert(p!=NULL);
    assert(regex_str!=NULL);
    //regex_str will not exceed size_t
    //Cos size_t should include the max possible char array size.
    p->pospt = 0;
    size_t slen = 0;
    while(regex_str[slen] != '\0'){
        ++slen;
    }
    p->reglen = slen;
    p->regex_str = regex_str;
    p->nfa_id_no = 0;
    return;
}
//O(1).
static struct NFA_oneacc* R2N_nfa_or(struct Regex2NFA_converter* p, struct NFA_oneacc* firpt, struct NFA_oneacc* secpt){
    //Ored into the firstpt and free the second one.
    //Do not free the states inside it.
    //Both firpt and secpt should not be used in previous way.

    //Merge && add two nodes && add four edges.
    struct NFA_state* sonept = calloc(1,sizeof(struct NFA_state));
    if(sonept == NULL){
        fprintf(stderr,"Fail to malloc the first NFA_state in R2N_nfa_or\n");
        exit(1);
    }
    assign_nfa_id(p,sonept);
    struct NFA_state* stwopt = calloc(1,sizeof(struct NFA_state));
    if(stwopt == NULL){
        fprintf(stderr,"Fail to malloc the second NFA_state in R2N_nfa_or\n");
        exit(1);
    }
    assign_nfa_id(p,stwopt);
    NFA_add_state(firpt,sonept);
    NFA_add_state(firpt,stwopt);
    //Merge lists.
    NFA_merge(firpt,secpt);
    //Add four edges.
    NFA_add_edge(sonept,firpt->q_start,0,1);
    NFA_add_edge(sonept,secpt->q_start,0,1);
    NFA_add_edge(firpt->accepted_q,stwopt,0,1);
    NFA_add_edge(secpt->accepted_q,stwopt,0,1);
    firpt->q_start = sonept;
    firpt->accepted_q = stwopt;
    free(secpt);//Do not free the nodes.
    return firpt;
}
//O(1)
static struct NFA_oneacc* R2N_nfa_star(struct Regex2NFA_converter* p, struct NFA_oneacc* npt){
    //Add two nodes, four edges.
    struct NFA_state* sonept = calloc(1,sizeof(struct NFA_state));
    if(sonept == NULL){
        fprintf(stderr,"Fail to malloc the first NFA_state in R2N_nfa_star\n");
        exit(1);
    }
    struct NFA_state* stwopt = calloc(1,sizeof(struct NFA_state));
    if(stwopt == NULL){
        fprintf(stderr,"Fail to malloc the second NFA_state in R2N_nfa_star\n");
        exit(1);
    }
    assign_nfa_id(p,sonept);
    assign_nfa_id(p,stwopt);
    NFA_add_state(npt,sonept);
    NFA_add_state(npt,stwopt);
    NFA_add_edge(sonept,npt->q_start,0,1);
    NFA_add_edge(sonept,stwopt,0,1);
    NFA_add_edge(npt->accepted_q,stwopt,0,1);
    NFA_add_edge(npt->accepted_q,npt->q_start,0,1);
    npt->q_start = sonept;
    npt->accepted_q = stwopt;
    return npt;
}
//O(1)
static struct NFA_oneacc* R2N_nfa_concat(struct Regex2NFA_converter* p, struct NFA_oneacc* firpt, struct NFA_oneacc* secpt){
    //Add no node.
    //Add one edge.
    //Merge two lists.
    NFA_merge(firpt,secpt);
    NFA_add_edge(firpt->accepted_q,secpt->q_start,0,1);
    firpt->accepted_q = secpt->accepted_q;
    free(secpt);//Do not free the nodes.
    return firpt;
}

static struct NFA_oneacc* R2N_regex(struct Regex2NFA_converter* p);
static struct NFA_oneacc* R2N_onesymchar(struct Regex2NFA_converter* p);
static struct NFA_oneacc* R2N_subexpr(struct Regex2NFA_converter* p);
static struct NFA_oneacc* R2N_factor(struct Regex2NFA_converter* p);
static int R2N_termright(struct Regex2NFA_converter* p);
static struct NFA_oneacc* R2N_term(struct Regex2NFA_converter* p);




static struct NFA_oneacc* R2N_onesymchar(struct Regex2NFA_converter* p){
    char ch = R2N_peek(p);
    R2N_move(p,1);
    return R2N_nfa_onesymbol(p,((int)ch));
}
static struct NFA_oneacc* R2N_regex(struct Regex2NFA_converter* p){
    if(!R2N_more(p)){
        return NULL;
    }
    char peekc = R2N_peek(p);
    if(peekc == ')'){
        return NULL;
    }
    else if(peekc == '|'){
        R2N_move(p,1);
        struct NFA_oneacc* right_node = R2N_regex(p);
        if(right_node == NULL){
            return NULL;
        }else{
            return R2N_nfa_or(p,R2N_nfa_empty_string(p),right_node);
        }
    }
    else{
        struct NFA_oneacc* left_node = R2N_subexpr(p);
        if(!R2N_more(p)){
            return left_node;
        }
        peekc = R2N_peek(p);
        if(peekc == '|'){
            R2N_move(p,1);
            struct NFA_oneacc* right_node = R2N_regex(p);
            if(left_node == NULL){
                if(right_node == NULL){
                    return NULL;
                }else{
                    return R2N_nfa_or(p,R2N_nfa_empty_string(p),right_node);
                }
            }else{
                if(right_node == NULL){
                    return R2N_nfa_or(p,left_node,R2N_nfa_empty_string(p));
                }else{
                    return R2N_nfa_or(p,left_node,right_node);
                }
            }
        }else{
            assert(peekc == ')');
            return left_node;
        }
    }
}
static struct NFA_oneacc* R2N_subexpr(struct Regex2NFA_converter* p){
    assert(R2N_more(p));
    struct NFA_oneacc* left_node = R2N_term(p);
    if(!R2N_more(p)){
        return left_node;
    }else{
        char peekc = R2N_peek(p);
        if(peekc == '|'){
            return left_node;
        }else if(peekc == ')'){
            return left_node;
        }else{
            struct NFA_oneacc* right_node = R2N_subexpr(p);
            return R2N_nfa_concat(p,left_node,right_node);
        }
    }
}
//0 for NULL, 1 for '*', 2 for '?'
static int R2N_termright(struct Regex2NFA_converter* p){
    if(!R2N_more(p)){
        return 0;
    }
    char peekc = R2N_peek(p);
    if(peekc == '*'){
        R2N_move(p,1);
        if(!R2N_more(p)){
            return 1;
        }
        peekc = R2N_peek(p);
        while(peekc == '*' || peekc == '?'){
            R2N_move(p,1);
            if(!R2N_more(p)){
                break;
            }
            peekc = R2N_peek(p);
        }
        return 1;
    }else if(peekc == '?'){
        int eff = 2;
        R2N_move(p,1);
        if(!R2N_more(p)){
            return 2;
        }
        peekc = R2N_peek(p);
        while(1){
            if(peekc == '*'){
                eff = 1;
            }else if(peekc == '?'){

            }else{
                break;
            }
            R2N_move(p,1);
            if(!R2N_more(p)){
                break;
            }
            peekc = R2N_peek(p);
        }
        return eff;
    }else{
        return 0;
    }
}
static struct NFA_oneacc* R2N_term(struct Regex2NFA_converter* p){
    if(!R2N_more(p)){
        fprintf(stderr,"No more char when calling term\n");
        exit(1);
    }
    struct NFA_oneacc* fac_node = R2N_factor(p);
    int right_no = R2N_termright(p);
    if(fac_node == NULL){
        return NULL;
    }else{
        if(right_no == 0){
            return fac_node;
        }else{
            if(right_no == 1){
                return R2N_nfa_star(p,fac_node);
            }else if(right_no == 2){
                return R2N_nfa_or(p,R2N_nfa_empty_string(p),fac_node);
            }else{
                fprintf(stderr,"Unexpected return from termright %d\n",right_no);
                exit(1);
            }
        }
    }
}
static struct NFA_oneacc* R2N_factor(struct Regex2NFA_converter* p){
    char peekc = R2N_peek(p);
    if(peekc == '('){
        R2N_move(p,1);
        struct NFA_oneacc* re_node = R2N_regex(p);
        if(!R2N_more(p)){
            fprintf(stderr,"No char is left in factor after calling regex, but () unparied\n");
            exit(1);
        }
        peekc = R2N_peek(p);
        if(peekc != ')'){
            fprintf(stderr,"Not ) in factor after calling regex\n");
            exit(1);
        }
        R2N_move(p,1);
        return re_node;
    }else if(peekc == '\\'){
        R2N_move(p,1);
        if(!R2N_more(p)){
            fprintf(stderr,"Trailing backslash\n");
            exit(1);
        }
        return R2N_onesymchar(p);
    }else{
        return R2N_onesymchar(p);
    }
}


static struct NFA_oneacc* R2N_convert_parse(struct Regex2NFA_converter* p){
    struct NFA_oneacc* anspt = R2N_regex(p);
    if(anspt == NULL){
        anspt = R2N_nfa_empty_string(p);
        anspt->states_cnt = 1;
        return anspt;
    }else{
        anspt->states_cnt = p->nfa_id_no;
        return anspt;
    }
}
struct NFA_oneacc* Regex2NFA(const char* regex_str){
    struct Regex2NFA_converter converter;
    R2N_convert_init(&converter,regex_str);
    return R2N_convert_parse(&converter);
}
static void addState(struct NFA_state* statept, struct Cbitset* bpt, struct NFA_state_stack* thestack){
    NFA_state_stack_push(thestack,statept);
    put_cbitset(bpt,statept->id);
    struct NFA_edge* pt = statept->edge_hashtable.hash_positions[NFA_HASH_SIZE - 1];
    while(pt != NULL){
        if(!test_cbitset(bpt,pt->des->id)){
            addState(pt->des,bpt,thestack);
        }
        pt = pt->next;
    }
    return;
}

int NFASimu(struct NFA_oneacc* nfa, const char* input_str, size_t lenofstr){
    assert(nfa->states_cnt > 0);
    //Only one final stage here.
    struct NFA_state* final_state = nfa->accepted_q;
    struct Cbitset alreadyOn;
    if(init_cbitset(&alreadyOn,nfa->states_cnt)!=0){
        fprintf(stderr,"Fail to init bitset alReadyOn\n");
        exit(1);
    }
    struct NFA_state_stack oldStates;
    NFA_state_stack_init(&oldStates,nfa->states_cnt);
    struct NFA_state_stack newStates;
    NFA_state_stack_init(&newStates,nfa->states_cnt);
    addState(nfa->q_start,&alreadyOn,&oldStates);
    clear_bitset(&alreadyOn);
    int retans = 0;
    size_t curpt_in_ch = 0;
    while(curpt_in_ch < lenofstr){
        int ch = (int)(input_str[curpt_in_ch]);
        int key = ch % (NFA_HASH_SIZE - 1);//Not epsilon.
        while(!NFA_state_stack_empty(&oldStates)){
            struct NFA_state* temppt = NFA_state_stack_pop(&oldStates);
            struct NFA_edge* ept = temppt->edge_hashtable.hash_positions[key];
            while(ept != NULL){
                if(ept->symbol == ch){
                    //If NFA_HASH_SIZE big enough, like 129 for ASCII.
                    //Do not need this check at all.
                    if(!test_cbitset(&alreadyOn,ept->des->id)){
                        addState(ept->des,&alreadyOn,&newStates);
                    }
                }
                ept = ept->next;
            }
        }
        NFA_state_stack_swap(&oldStates,&newStates);
        clear_bitset(&alreadyOn);
        ++curpt_in_ch;
    }
    size_t nowsize = oldStates.spt;
    while(!NFA_state_stack_empty(&oldStates)){
        struct NFA_state* spt = NFA_state_stack_pop(&oldStates);
        if(spt == nfa->accepted_q){
            retans = 1;
            goto clean_and_return;
        }
    }
clean_and_return:
    free_bitset(&alreadyOn);
    NFA_state_stack_free(&oldStates);
    NFA_state_stack_free(&newStates);
    return retans;
}
void free_NFA(struct NFA_oneacc* nfapt){
    //Converter has nothing to free.
    //Free all the nodes && edges && NFA itself.
    //time-complexity: O(m+n) = O(r)
    struct NFA_state* spt = nfapt->states_head;
    while(spt != NULL){
        struct NFA_state* nxtpt = spt->next_in_nfalist;
        unsigned long long i;
        for(i=0;i<NFA_HASH_SIZE;++i){
            struct NFA_edge* ept = spt->edge_hashtable.hash_positions[i];
            while(ept != NULL){
                struct NFA_edge* enxt = ept->next;
                free(ept);
                ept = enxt;
            }
        }
        free(spt);
        spt = nxtpt;
    }
    free(nfapt);
}