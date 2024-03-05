# NFA simulation on the fly  
C and python version uses the same NFA, but in different formats.  
## python version  
Test this with:  
python3 NFASimu.py nfa.json  xxx.txt  
### Format  
NFA is in json.  
## C version  
Input is a graph with attribute on it.  
Note that currently assuming that every symbol here is encoded as ASCII.   
### Format  
The first line:  
n q_start n_acc acc_1 ......  
n is node_number.  
Then n lines, every line starts with a number and EXACTLY one space.  
The number shows how many out edges are there from this node(note that a,b to the same node is taken as 2, and epsilon is also one option  
a to node1 && node2 is also two, epsilon to n nodes taken as n).   
Then every part is a,b,xxxxxyyyyyy  
(there are a bytes for the symbol and b bytes for destination, exactly. If a is zero, then it is epsilon).  
Then there should be EXACTLY one newline to get to the next line.  

An example is given in nfa.txt, which is the same nfa with nfa.json.  

Compile first and then run with ./NFASimu nfa.txt xxx.txt  
