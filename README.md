# RegexTools
## DFA2Regex
Specify a DFA and it will output regex for it.  
For testing purpose, it will add ^()$ around regex.  
You can modify the code to print(regex_str) instead of full_line_regex to get avoid matching the full line.  
Another issue is that it does not match across lines in grep cos dot does not match newline here.  
## NFA2DFA
Convert NFA into DFA.  
Test case is from Sipser, check README.md in NFA2DFA folder for detail.  
Also add minimization of DFA into NFA2DFA, check mindfa.py for detail.  
## NFASimu  
NFA simulation on the fly.  
## Regex2NFA
Parse regular expression and convert it into NFA.  
## mygrep
Combine Regex2NFA && NFASimu.  
This is a grep that supports all regular languages.  





