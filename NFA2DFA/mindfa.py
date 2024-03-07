from typing import FrozenSet, Dict, Set
# partition is a set of names.
# And another dict to record which name is in which set.
# (two-way mapping)
        
# Need to prove we can partition in any order.
# The binary-relation of undistinguishable is eq relation.
# reflecxivity and symmetry are obvious.
# Transitivity is also true, cos it requires any string.
# So they can be divided into eq-classes. 

# f is a set of functions, fc(S) = T, means S(c) --> T.
# Every symbol has a function fc.
# Any fc is a homomorphism.
        
# When breaking the loop, it can never proceed after that.
# If cannot partition in one round in one step of any symbol, then every partition is an eq class, 
# cos if they are not, there exsists some string forcing some set to be partionable in one step.


# It starts from a situation that it is possible for one partition to be not in the same eq class, 
# but different partitions must be in different eq classes.
# Now prove: 1. correctness: after merging by partition, every partition is undestinguishable.
# If not, there exsists a string, then eat one by one, until two stages differ.
# Then go backward for one, that partition can be partitioned again in one step.
# 1 shows that we get the eq-classes.
# 2. min: Cannot be a smaller DFA.
# If smaller, must have a larger set which is a mixture of not only one eq-class.
def minimize_dfa(dfa_dict:dict,st_name:str,ed_names:list)->dict:
    # First, check if all acc or no acc.
    # Should have no duplication in ed_names.
    F = frozenset(ed_names)
    if len(F) != len(ed_names):
        raise "The accepting states in minimize_dfa have duplications"
    all_symbols = dfa_dict[st_name].keys()
    if len(ed_names) == len(dfa_dict.keys()):
        return {"q_start":"q0",
                "graph":{"q0":{sym : "q0" for sym in all_symbols},
                         "accepted_q":["q0"]}}
    elif len(ed_names) == 0:
        return {"q_start":"q0",
                "graph":{"q0":{sym : "q0" for sym in all_symbols},
                         "accepted_q":[]}}
    else:
        # Have acc && noacc.
        # Top-down. Parition refinement.
        # Note that we do not remove non-distinguishable states here.
        # When generating from NFA, every state is reachable.
        # dead states are not removed, cos I want a complete DFA.
        class Linked_Node:
            # User class hashable by default.
            # Key is derived from id().
            # Only eq to itself.
            def __init__(self,part:FrozenSet[str]):
                self.partition : FrozenSet[str] = part
                self.prev = None
                self.next = None
                self.qname = ""
            
        class Partion_list:
            def __init__(self):
                self.head = None
                self.size = 0
            def insert_front(self, part_node: Linked_Node):
                part_node.next = self.head
                part_node.prev = None
                if self.head is not None:
                    self.head.prev = part_node
                self.head = part_node
                self.size += 1
            def remove(self, part_node:Linked_Node):
                if part_node.prev is not None:
                    part_node.prev.next = part_node.next
                else:
                    # Is head.
                    self.head = part_node.next
                if part_node.next is not None:
                    part_node.next.prev = part_node.prev
                self.size -= 1

        
        
        plist = Partion_list()
        par_dist : Dict[str,Linked_Node] = {}
        Q_minus_F = frozenset(set(dfa_dict.keys()) - F)
        nodeone = Linked_Node(F)
        nodetwo = Linked_Node(Q_minus_F)
        plist.insert_front(nodeone)
        plist.insert_front(nodetwo)
        for name in dfa_dict.keys():
            if name in F:
                par_dist[name] = nodeone
            else:
                par_dist[name] = nodetwo
        while True:
            prevsize = plist.size
            pt = plist.head
            have_parted_more = False
            while pt is not None:
                for c in all_symbols:
                    group_by_despart : Dict[FrozenSet[str], Set[str]] = {}
                    for name in pt.partition:
                        des = dfa_dict[name][c]
                        despart = par_dist[des]
                        if despart in group_by_despart:
                            group_by_despart[despart].add(name)
                        else:
                            group_by_despart[despart] = {name}
                    if len(group_by_despart) > 1:
                        # Partition according to this grouping.
                        # Remove from list first.
                        plist.remove(pt)
                        # Form several more partitions and insert.
                        for newset in group_by_despart.values():
                            fs = frozenset(newset)
                            fn = Linked_Node(fs)
                            plist.insert_front(fn)
                            for changed_name in fs:
                                par_dist[changed_name] = fn
                        # Restart the list loop.
                        have_parted_more = True
                        break
                if have_parted_more:
                    break
                pt = pt.next
            nowsize = plist.size
            if nowsize == prevsize:
                break
        redict = {"q_start":"","graph":{},"accepted_q":[]}
        node_id = 0
        pt = plist.head
        while pt is not None:
            if len(pt.qname) == 0:
                pt.qname = f"q{node_id}"
                node_id += 1
            if st_name in pt.partition:
                redict["q_start"] = pt.qname
            one_name = next(iter(pt.partition))
            # If one in F, then every one must be in F.
            if one_name in F:
                redict["accepted_q"].append(pt.qname)
            if pt.qname not in redict["graph"]:
                    redict["graph"][pt.qname] = {}
            for c in all_symbols:
                des = dfa_dict[one_name][c]
                par_node = par_dist[des]
                if len(par_node.qname) == 0:
                    par_node.qname = f"q{node_id}"
                    node_id += 1
                redict["graph"][pt.qname][c] = par_node.qname
            pt = pt.next
        return redict

       

       
        

if __name__ == '__main__':
    print(minimize_dfa({"q0":{"0":"q3","1":"q1"},"q1":{"0":"q2","1":"q5"},
                        "q2":{"0":"q2","1":"q5"},"q3":{"0":"q0","1":"q4"},
                        "q4":{"0":"q2","1":"q5"},"q5":{"0":"q5","1":"q5"}},"q0",["q1","q2","q4"]))