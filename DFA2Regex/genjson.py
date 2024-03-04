import json
import sys
def sanity_check(dfa_dict:dict):
    if "q_start" not in dfa_dict:
        raise "q_start is not in the dictionary."
    if "graph" not in dfa_dict:
        raise "graph is not in the dictionary."
    if "accepted_q" not in dfa_dict:
        raise "accepted_q is not in the dictionary."
    assert(isinstance(dfa_dict["q_start"],str))
    assert(isinstance(dfa_dict["graph"],dict))
    assert(isinstance(dfa_dict["accepted_q"],list))
    if dfa_dict["q_start"] not in dfa_dict["graph"]:
        raise "q_start is not in the graph."
    for q in dfa_dict["accepted_q"]:
        if q not in dfa_dict["graph"]:
            raise "accepted_q contains a q that is not in the graph."
    acc_set = set(dfa_dict["accepted_q"])
    if len(acc_set) != len(dfa_dict["accepted_q"]):
        raise "accepted_q contains duplicate q."
    full_set = set(dfa_dict["graph"].keys())
    if len(full_set) != len(dfa_dict["graph"]):
        raise "graph contains duplicate q."
    
    
if __name__ == '__main__':
    the_dict = {"q_start":"q0",
                "graph":{"q0":{"a":"q1","b":"q2"},"q1":{"a":"q0","b":"q3"},"q2":{"a":"q3","b":"q0"},"q3":{"a":"q2","b":"q1"}},
                "accepted_q":["q2"]}
    assert(isinstance(the_dict,dict))
    try:
        sanity_check(the_dict)
    except Exception as e:
        print(f"Sanity check fails: {str(e)}",file=sys.stderr)
        sys.exit(1)
    with open("dfa.json","w") as f:
        json.dump(the_dict,f)