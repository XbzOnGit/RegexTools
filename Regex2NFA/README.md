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
