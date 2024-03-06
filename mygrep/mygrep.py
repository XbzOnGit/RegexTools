import sys
import Regex2NFA
import NFASimu

if __name__ == '__main__':
    if len(sys.argv) == 1 or len(sys.argv) > 3:
        print("Usage: python3 mygrep.py pattern <file>",file=sys.stderr)
        sys.exit(1)
    regex_str = sys.argv[1]
    if len(sys.argv)== 2:
        input_str = input()
    else:
        with open(sys.argv[2],'r') as f:
            input_str = f.read()
    NFA_dict = Regex2NFA.Regex2NFA(regex_str)
    NFASimu.register_input_string(input_str)
    if NFASimu.NFASimu(NFA_dict['graph'],NFA_dict['q_start'],NFA_dict['accepted_q']):
        print("yes")
    else:
        print("no")
