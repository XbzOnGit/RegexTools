# Regex2NFA
I implemented a regular expression parser and converter.  
It will parse a regular expression and convert it into NFA.  
## Functionality
Support (), \, *, ?, |, concatenate.  
This is enough for any regular language.  
And this cannot express any non-regular language(like CFG).  
## Format
Input format is a regular expression. Use printf '(a|b)*abb' > regex.txt to prevent appending newline in vim.  
Output format is json, the same format with other nfa jsons in this project.  
This format requires any state to have an entry in graph. If no edge is from this state, write q:{}.  
## Test case 
I used (a|b)*abb  
I converted it into NFA, and use NFA2DFA to convert it into DFA.  
Then I draw the graph for DFA, and prove it equivalent with (a|b)*abb mathematically.  

## Time complexity
The python version includes some copying in the construction of graph, and thus cannot guarantee O(r).  
Perhaps I will implement a C version later, combining the C_version of NFASimu to get the a guaranteed O(r) time on the construction of NFA, and then O(x*(m+n)), in total O(|r| * |x|).  
About O(m+n) = O(r).  
When limiting accepting state to be always only one, every or, char, star, concat is O(1), adding at most two states and at most four edges.  
So the final m <= 2 x r && n <= 4 x r.  
