import json
if __name__ == '__main__':
    # q_start still only one.
    the_dict = {"q_start":"q1",
                "graph":{"q1":{"b":["q2"],"":["q3"]},"q2":{"a":["q2","q3"],"b":["q3"]},"q3":{"a":["q1"]}},
                "accepted_q":["q1"]}
    with open("nfa.json", "w") as f:
        json.dump(the_dict, f)