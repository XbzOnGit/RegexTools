import json
import sys
# Support |, *, concatenate, ?, ().
# Only supporting these characters and escape(\) can guarantee that it is a real regular language.
# Some extensions will enable non-regular, or even non-context-free language.
# Not supporting a+, use aa* instead.
# Not supporting ^, $. Now is taking the entire input file as a string and newline is one symbol in it.
# Not supporting . for simplicity. You can use (s1|s2|.......|sn).
# Not supporting [] for simplicity, use | instead.
'''
+---+----------------------------------------------------------+
|   |             ERE Precedence (from high to low)            |
+---+----------------------------------------------------------+
| 1 | Collation-related bracket symbols | [==] [::] [..]       |
| 2 | Escaped characters                | \<special character> |
| 3 | Bracket expression                | []                   |
| 4 | Grouping                          | ()                   |
| 5 | Single-character-ERE duplication  | * + ? {m,n}          |
| 6 | Concatenation                     |                      |
| 7 | Anchoring                         | ^ $                  |
| 8 | Alternation                       | |                    |
+---+-----------------------------------+----------------------+
'''
# Now, () > *,? > concatenation > |
'''
Grammar for it:  
regex --> subexpr'|'regex  |  regex'|'subexpr |  subexpr |  epsilon
subexpr --> term 'cont' subexpr | subexpr 'cont' term | term
term --> term* | term? | factor* | factor? | factor
factor --> '('regex')' | char | \char
char --> s1 | s2 | s3 | s4 | ...... | sn
Note that char si includes *,?,(,),\,|
'''
# However, some rules in the above grammar are left-recursive.
# We do not need subexpr concatenate term in subexpr -->, 
# cos subexpr is only used for concatenating more terms, and we can always extract the first term.
# For the same reason, we just need regex --> subexpr'|'regex and '|' regex, 
# cos we just extract subexpr from regex until we cannot, and we can always extract from left side.
# The only left-recursion now in term --> term* | term? can be removed manually.
# The final grammar I implemented is as follows.
'''
regex --> subexpr'|'regex  | '|' regex | subexpr | epsilon
subexpr --> term 'cont' subexpr | term
term --> factor<concatenate>termright
termright --> *termright|?termright|epsilon
factor --> '('regex')' | char | \char
char --> s1 | s2 | s3 | s4 | ...... | sn
'''
# Note that the above grammar does not have left-recursion.  
# Add epsilon to regex to make empty regex legal, and add |regex to make epsilon before | legal.
# So subexpr must not be epsilon before parsing, though it can return None in the end.
# termrigh is extracting ? or * if possible, so it will use epsilon only if no * or ? at all.



        
# The converter combines parser and converter.
# It is one-pass. It reads every symbol of regular expression just once.
# Don't use it twice. Construct another converter instead.
class Regex2NFAConverter:
    def __init__(self,regex_str:str):
        self.regex_string = regex_str
        self.pospt = 0
        self.nfa_state_no = 0
        self.left_parentheses_cnt = 0
    def more(self):
        return self.pospt < len(self.regex_string)
    def peek(self):
        assert(self.more())
        return self.regex_string[self.pospt]
    def move(self, off:int):
        self.pospt += off
    

    def nfa_empty_string(self)->dict:
        q_name = f"q{self.nfa_state_no}"
        self.nfa_state_no += 1
        return {"q_start":q_name,"graph":{q_name:{}},"accepted_q":[q_name]}
    
    def nfa_onesymbol(self, symbol:str)->dict:
        qone = f"q{self.nfa_state_no}"
        self.nfa_state_no += 1
        qtwo = f"q{self.nfa_state_no}"
        self.nfa_state_no += 1
        return {"q_start":qone,"graph":{qone:{symbol:[qtwo]},qtwo:{}},"accepted_q":[qtwo]}


    def parse(self)->dict:
        tempre = self.regex()
        if tempre is None:
            return self.nfa_empty_string()
        else:
            return tempre
        
    
    # Every nfa initially comes from nfa_empty_string or nfa_onesymbol.
    # And later added states also increase state_no.
    # So they all have unique names.
    def nfa_or(self, fir_nfa:dict,sec_nfa:dict)->dict:
        # Add one node, and two edges.
        q_name = f"q{self.nfa_state_no}"
        self.nfa_state_no += 1
        return {"q_start":q_name,"graph":{**fir_nfa['graph'],**sec_nfa['graph'],q_name:{"":[fir_nfa['q_start'],sec_nfa['q_start']]}}
                    ,"accepted_q":fir_nfa['accepted_q'] + sec_nfa['accepted_q']}
        
    def nfa_star(self, nfa_dict:dict)->dict:
        # Add one node and |acceped_q| + 1 edges.
        q_name = f"q{self.nfa_state_no}"
        self.nfa_state_no += 1
        oriqstart = nfa_dict['q_start']
        next_graph = nfa_dict['graph'].copy()
        for eq in nfa_dict['accepted_q']:
            if "" in next_graph[eq]:
                next_graph[eq][""].append(oriqstart)
            else:
                next_graph[eq][""] = [oriqstart]
        next_graph[q_name] = {"":[oriqstart]}
        return {"q_start":q_name,"graph":next_graph,"accepted_q":nfa_dict['accepted_q'] + [q_name]}
    
    def nfa_concat(self,fir_nfa:dict,sec_nfa:dict)->dict:
        # No added node.  
        # Add k1 edges.
        new_graph_of_fir = fir_nfa['graph'].copy()
        secst = sec_nfa['q_start']
        for eq in fir_nfa['accepted_q']:
            if "" in new_graph_of_fir[eq]:
                new_graph_of_fir[eq][""].append(secst)
            else:
                new_graph_of_fir[eq][""] = [secst]
        return {"q_start":fir_nfa['q_start'],'graph':{**new_graph_of_fir,**sec_nfa['graph']},'accepted_q':sec_nfa['accepted_q'].copy()}
        
    
    # None is a NFA only accepting empty string.
    # Other dict is the NFA for this part.
    # Specially, termright cannot construct a dict on its own, 
    # the only function calling it is term, and it just returns some information to term.
    # Note, ) is handled by factor to keep it the same level with (. But it can be peeked by other levels.
    def regex(self):
        # Note that the problem of (regex) should be handled outside regex function.
        # factor function should handle this.
        if not self.more():
            return None
        peekc = self.peek()
        if peekc == ')':
            if self.left_parentheses_cnt <= 0:
                raise f"See ) in regex but counter is {self.left_parentheses_cnt} <= 0"
            else:
                # But do not eat the char, and do not -= this counter.
                # In three situations do we call regex().
                # 1. initially.
                # 2. inside this function, after unescaped |
                # 3. inside factor, after unescaped (
                # With left_cnt > 0, it cannot be the first call.
                # In either case in 2 or 3, returning None is correct.
                return None
        elif peekc == '?' or peekc == '*':
            raise "Calling regex starting with ? or *"
        elif peekc == '|':
            self.move(1)
            # epsilon | right_node
            right_node = self.regex()
            if right_node is None:
                return None
            else:
                # epsilon | right_node
                # Add empty into it.
                return self.nfa_or(self.nfa_empty_string(),right_node)
        else:
            # others || \ || (
            # expands into subexpr || subexpr | regex
            left_node = self.subexpr()
            if not self.more():
                return left_node
            peekc = self.peek()
            if peekc == '|':
                self.move(1)
                right_node = self.regex()
                if left_node is None:
                    if right_node is None:
                        return None
                    else:
                        return self.nfa_or(self.nfa_empty_string(),right_node)
                else:
                    if right_node is None:
                        return self.nfa_or(left_node,self.nfa_empty_string())
                    else:
                        return self.nfa_or(left_node,right_node)

            else:
                # Should be a ) here.
                # Check the comment above subexpr.
                # Here we have more input and not peeking |, must be ).
                assert(peekc == ')')
                return left_node
            
            

        

    # subexpr returns to regex only if no any other term following.
    # It means no more input, or peeking | or peeking ).
    # When calling subexpr, must have more input(verified).
    # subexpr can return None, if term returns None in a case like ().
    def subexpr(self):
        assert(self.more())
        left_node = self.term()
        # term will go through a single factor and all *? after it.
        # Then check if it is possible to get another term.
        # If possible, call subexpr again.
        if not self.more():
            return left_node
        else:
            peekc = self.peek()
            # If more, but is | --> return.
            # If more, but is *? --> error.
            # If more, but is (,\,symbol --> another term.
            if peekc == '|':
                # Cannot get another term from here.
                return left_node
            elif peekc == '*' or peekc == '?':
                # term should not leave these here.
                raise "peekc is * or ? after calling term in subexpr"
            elif peekc == ')':
                # Cannot get another term.
                return left_node
            else:
                # others or \ or (, just go on for another term.
                right_node = self.subexpr()
                return self.nfa_concat(left_node,right_node)
    
    # When calling term, must have more input(verified).
    # term can return None in a case like ().
    def term(self):
        if not self.more():
            raise "No more char when calling term"
        fac_node = self.factor()
        right_node = self.termright()
        if fac_node is None:
            return None
        else:
            if right_node is None:
                return fac_node
            else:
                if right_node == '*':
                    return self.nfa_star(fac_node)
                elif right_node == '?':
                    #epsilon | fac(not empty)
                    return self.nfa_or(self.nfa_empty_string(),fac_node)
                else:
                    raise f"Unexpected return from termright {right_node}"

    def termright(self):
        # termright is just empty or a sequence of mixture of * and ?
        # Use a while loop to get them all.
        if not self.more():
            return None
        peekc = self.peek()
        if peekc == '*':
            # Eat them all.
            # The effect is the same with a single *.
            self.move(1)
            if not self.more():
                return '*'
            peekc = self.peek()
            while peekc == '*' or peekc == '?':
                self.move(1)
                if not self.more():
                    break
                peekc = self.peek()
            return '*'
        elif peekc == '?':
            eff = '?'
            self.move(1)
            if not self.more():
                return '?'
            peekc = self.peek()
            while True:
                if peekc == '*':
                    # Turn into *.
                    eff = '*'
                elif peekc == '?':
                    pass #  Nothing to do.
                else:
                    break
                self.move(1)
                if not self.more():
                    break
                peekc = self.peek()
            return eff
        else:
            # Empty.
            return None

    def factor(self):
        # In factor, should always have more chars available.
        peekc = self.peek() # Do not move here.
        if peekc == '(':
            self.left_parentheses_cnt += 1
            self.move(1)
            # Fine to call regex with no more char.
            redict = self.regex()
            if not self.more():
                raise "No char is left in factor after calling regex, but () unparied"
            peekc = self.peek()
            if peekc != ')':
                raise "Not ) in factor after calling regex"
            self.left_parentheses_cnt -= 1
            self.move(1)
            # Fine to leave no char after calling factor.
            return redict
        elif peekc == '\\':
            # Escape.
            self.move(1)
            if not self.more():
                raise "Trailing backslash"
            return self.char()
        else:
            # just char.
            return self.char()

    def char(self):
        # In char, guaranteed to be able to peek.
        symbol = self.peek()
        self.move(1)
        return self.nfa_onesymbol(symbol)
    


def Regex2NFA(regex_string:str)->dict:
    parser = Regex2NFAConverter(regex_string)
    return parser.parse()

    
if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python3 Regex2NFA.py regex.txt nfa.json",file=sys.stderr)
        sys.exit(1)
    with open(sys.argv[1],"r") as f:
        regex_string = f.read()
    NFA_dict = Regex2NFA(regex_string)
    with open(sys.argv[2],"w") as f:
        json.dump(NFA_dict,f)