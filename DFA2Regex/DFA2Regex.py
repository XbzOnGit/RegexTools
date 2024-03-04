# Given a DFA, 
# formatted as a json of {q_start: string, "gragh": {qname: {symbol: destination}]}, "accepted_q": [a list of string]}
# Must be well-formed, which is to say every symbol has a destination.

# Output a regex.
# Note that I only use *,|, ? ,concatenate as reserved. So I need to escape only * | ( ) \, ?

# Input is by default dfa.json
import json
import sys    
# Assumption: the parameters are healthy(be a correct DFA).
# 1. In dfa_dict, every qname has a dict which lists every symbol and its destination.
# 2. original_start in dfa_dict.
# 3. original_accs in dfa_dict, and no duplication in itself.
def dfa_to_gnfa(dfa_dict:dict, original_start: str, original_accs: list)->list:
    # Organize the edges as {stage_name: string|None}.
    # A string for an existsing regex('' with length 0 for epsilon), None for empty set.

    # dfa is {qname:{symbol: stage_name}}, opposite direction.
    # Find the longest stage_name in dfa, then generate two more longs names.
    # In this way, I can guarantee that the two new stages are not in the original dfa.
    longest_length = max([len(x) for x in dfa_dict.keys()])
    q_start = "__q_start__" + "a" * longest_length
    q_end = "__q_end__" + "b" * longest_length
    gnfa_dict = {q_start:{},q_end:{}}
    gnfa_dict[q_start][q_end] = None
    # Only record the outgoing edges.
    original_acc_set = set(original_accs)
    escape_set = set(['\\','|','*','(',')','?'])
    # An edge is a {"qname","string|None"}.
    for qname in dfa_dict.keys():
        gnfa_dict[q_start][qname] = '' if qname == original_start else None
        # Then construct the other stages, 
        # should have one edge to itself && one edge to every other stage except q_start.
        # Now only construct the ones q_start to it && it to q_end && it to itself.
        gnfa_dict[qname] = {}
        gnfa_dict[qname][q_end] = '' if qname in original_acc_set else None
        # To others None now, then can be overwritten.
        for qe in dfa_dict.keys():
            gnfa_dict[qname][qe] = None
    
    # Now construct the orignial stages' edges to each other.
    # Can overwrite the None.
    for qname, edges_dict in dfa_dict.items():
        for symbol, dest in edges_dict.items():
            if symbol in escape_set:
                symbol = '\\' + symbol
            if dest not in gnfa_dict[qname]:
                gnfa_dict[qname][dest] = symbol
            else:
                if gnfa_dict[qname][dest] is None:
                    gnfa_dict[qname][dest] = symbol
                else:
                    gnfa_dict[qname][dest] += "|" + symbol
    return [q_start,q_end,gnfa_dict]

def convert(gnfa_dict: dict, q_start: str, q_end: str)->dict:
    # q_start and q_end will not change.
    assert(len(gnfa_dict) > 2)
    qrip = None
    next_qlist = []
    for qname in gnfa_dict.keys():
        if qname != q_start and qname != q_end:
            qrip = qname
            break
    # Now qrip is the one to be ripped.
    next_gnfa_dict = {}
    for qname in gnfa_dict.keys():
        if qname != qrip:
            next_qlist.append(qname)
            next_gnfa_dict[qname] = {}
    
    for qa in next_qlist:
        if qa == q_end:
            # q_end does not have out edges.
            continue
        for qb in next_qlist:
            if qb == q_start:
                # q_start does not have incoming edges.
                continue
            # Construct the edge from qa to qb.
            # (R1)(R2)*(R3)|(R4)
            # R1 is qa to qrip, R2 is qrip to qrip, R3 is qrip to qb, R4 is qa to qb.
            R1 = gnfa_dict[qa][qrip]
            R3 = gnfa_dict[qrip][qb]
            R2 = gnfa_dict[qrip][qrip]
            First_part = None
            if R1 is not None and R3 is not None:
                eff_R1 = '(' + R1 + ')' if R1 != '' else ''
                eff_R3 = '(' + R3 + ')' if R3 != '' else ''
                if R2 is None:
                    First_part = eff_R1 + eff_R3
                else:
                    eff_R2 = '(' + R2 + ')*' if R2 != '' else ''
                    First_part = eff_R1 + eff_R2 + eff_R3
            # Else just None.
            R4 = gnfa_dict[qa][qb]
            Second_part = None
            if R4 is not None:
                if R4 != '':
                    Second_part = '(' + R4 + ')'
                else:
                    Second_part = ''
            # Else just None.
            if First_part is not None:
                if Second_part is not None:
                    if First_part == '':
                        if Second_part == '':
                            next_gnfa_dict[qa][qb] = ''
                        else:
                            next_gnfa_dict[qa][qb] = Second_part + '?'
                    else:
                        if Second_part == '':
                            next_gnfa_dict[qa][qb] = First_part + '?'
                        else:
                            next_gnfa_dict[qa][qb] = First_part + '|' + Second_part
                else:
                    next_gnfa_dict[qa][qb] = First_part
            else:
                if Second_part is not None:
                    next_gnfa_dict[qa][qb] = Second_part
                else:
                    next_gnfa_dict[qa][qb] = None

    return next_gnfa_dict
    
def dfa2regex(dfa_dict:dict, orignial_start: str, original_accs: list)->str:
    q_start, q_end, gnfa_dict = dfa_to_gnfa(dfa_dict, orignial_start, original_accs)
    # Then it is GNFA.
    # Loop until only two stages, and read the string in between.
    while len(gnfa_dict) > 2:
        gnfa_dict = convert(gnfa_dict, q_start, q_end)
    # New read the only edge.
    if gnfa_dict[q_start][q_end] is None:
        raise "This DFA does not match anything."
    return gnfa_dict[q_start][q_end]
def match_full_line(regex_string:str):
    return "^(" + regex_string + ")$"
if __name__ == '__main__':
    file_name = None
    if(len(sys.argv) == 1):
        file_name = "dfa.json"
    elif(len(sys.argv) == 2):
        file_name = sys.argv[1]
    else:
        print("Usage: python3 DFA2Regex.py [dfa.json]",file=sys.stderr)
        sys.exit(1)
    with open(file_name,"r") as f:
        dfa_dict = json.load(f)
    graph_dfa = dfa_dict['graph']
    st_qname = dfa_dict['q_start']
    ed_qnames = dfa_dict['accepted_q']
    regex_str = dfa2regex(graph_dfa, st_qname, ed_qnames)
    full_line_regex = match_full_line(regex_str)
    print(full_line_regex)