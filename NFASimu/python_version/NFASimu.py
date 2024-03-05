import json
import sys


# Now I just read all into memory.
Input_string = ""
pospt = 0
def getchar()->str:
    global pospt
    if pospt == len(Input_string):
        return ''
    else:
        savept = pospt
        pospt += 1
        return Input_string[savept]

cached_closure_fromq = {}
def epsilon_closure_q(nfa_dict:dict, qname:str)->set:
    check_cache = cached_closure_fromq.get(qname,None)
    if check_cache is not None:
        return check_cache
    eps_closure = {qname}
    stack = [qname]
    while len(stack) > 0:
        current = stack.pop()
        if '' in nfa_dict[current]:
            for q in nfa_dict[current]['']:
                if q not in eps_closure:
                    eps_closure.add(q)
                    stack.append(q)
    cached_closure_fromq[qname] = eps_closure
    return eps_closure

def epsilon_closure_set(nfa_dict:dict, qs)->set:
    ansset = set()
    for q in qs:
        ansset |= epsilon_closure_q(nfa_dict,q)
    return ansset

def move_q(nfa_dict:dict, qname:str, symbol:str)->set:
    return set(nfa_dict[qname].get(symbol,[]))

def move_set(nfa_dict:dict, qs, symbol:str)->set:
    ansset = set()
    for q in qs:
        ansset |= move_q(nfa_dict,q, symbol)
    return ansset



def NFASimu(nfa_dict:dict, st_qname:str, ed_qnames:list)->bool:
    S = epsilon_closure_q(nfa_dict,st_qname)
    c = getchar()
    while len(c) > 0:
        S = epsilon_closure_set(nfa_dict,move_set(nfa_dict,S,c))
        c = getchar()
    F = set(ed_qnames)
    fiset = F.intersection(S)
    return len(fiset) > 0


if __name__ == '__main__':
    file_name = None
    input_file = None
    if len(sys.argv) == 3:
        file_name = sys.argv[1]
        input_file = sys.argv[2]
    else:
        print("Usage: python3 NFASimu.py [nfa.json] [input_string.txt]",file=sys.stderr)
        sys.exit(1)
    with open(file_name,"r") as f:
        nfa_dict = json.load(f)
    with open(input_file,'r') as f:
        Input_string = f.read() # Read the entire file into memory.
    graph_nfa = nfa_dict['graph']
    st_qname = nfa_dict['q_start']
    ed_qnames = nfa_dict['accepted_q']
    ans = NFASimu(graph_nfa, st_qname, ed_qnames)
    ansstr = "Accepted" if ans else "Rejected"
    print(ansstr)
    