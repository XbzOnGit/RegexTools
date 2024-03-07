import json
import sys



untaged_dstates = [] # A stack is enough.
# Every stage will enter only once, and order does not matter.
closure_to_id = {}
next_dfa_stage_no = 0

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

def get_non_epsilon_input_symbols(nfa_dict:dict)->set:
    allsymset = set()
    for edge_dict in nfa_dict.values():
        for symbol in edge_dict.keys():
            if len(symbol) > 0:
                allsymset.add(symbol)
    return allsymset

def register_a_new_closure(clo:set)->int:
    global next_dfa_stage_no
    fs = frozenset(clo)
    assert(fs not in closure_to_id)
    closure_to_id[fs] = next_dfa_stage_no
    untaged_dstates.append(fs)
    save_for_return = next_dfa_stage_no
    next_dfa_stage_no += 1
    return save_for_return

def nfa2dfa(nfa_dict:dict, st_qname:str, ed_qnames:list)->dict:
    dfa_dict = {"q_start":"","graph":{},"accepted_q":[]}
    non_epsilon_input_symbols = get_non_epsilon_input_symbols(nfa_dict)
    # There is still a problem for allocating names for dfa stages.
    # Every closure should have a unique name.
    # Get q_start.
    closure_for_s0 = epsilon_closure_q(nfa_dict,st_qname)
    first_no = register_a_new_closure(closure_for_s0)
    dfa_dict["q_start"] = f"q{first_no}"

    # Now construct graph.
    while len(untaged_dstates) > 0:
        T_clo: frozenset = untaged_dstates.pop() # A stack.
        for symbol in non_epsilon_input_symbols:
            U = epsilon_closure_set(nfa_dict,move_set(nfa_dict,T_clo,symbol))
            fu = frozenset(U)
            if fu not in closure_to_id:
                # A new closure here.
                reno = register_a_new_closure(U)
                dfa_dict["graph"][f"q{reno}"] = {}
            the_Tclo_name = f"q{closure_to_id[T_clo]}"
            if the_Tclo_name not in dfa_dict["graph"]:
                dfa_dict["graph"][the_Tclo_name] = {}
            dfa_dict["graph"][the_Tclo_name][symbol] = f"q{closure_to_id[fu]}"

    # Now get accepted_q.
    # Loop over every exsisting closure, and find those with accepted in NFA.
    acceped_in_nfa = set(ed_qnames)
    for clo in closure_to_id.keys():
        for stage in clo:
            if stage in acceped_in_nfa:
                theno = closure_to_id[clo]
                dfa_dict["accepted_q"].append(f"q{theno}")
    # Duplication.
    dfa_dict['accepted_q'] = list(set(dfa_dict['accepted_q']))
    return dfa_dict


if __name__ == '__main__':
    file_name = None
    if(len(sys.argv) == 1):
        file_name = "nfa.json"
    elif(len(sys.argv) == 2):
        file_name = sys.argv[1]
    else:
        print("Usage: python3 NFA2DFA.py [nfa.json]",file=sys.stderr)
        sys.exit(1)
    with open(file_name,"r") as f:
        nfa_dict = json.load(f)
    graph_nfa = nfa_dict['graph']
    st_qname = nfa_dict['q_start']
    ed_qnames = nfa_dict['accepted_q']
    dfa_dict = nfa2dfa(graph_nfa, st_qname, ed_qnames)
    print(dfa_dict)