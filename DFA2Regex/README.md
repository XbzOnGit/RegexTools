# Convert DFA into Regex
1. For testing purpose, I force the regex to match a full line by adding ^ and $ around it.  
If you do not want this constrain, just print regex_str instread of full_line_regex  
2. Specify the DFA in genjson.py as a dict and generate the json, then python3 DFA2Regex.py xxx.json to get regex_str or full_line_regex  
3. A quick demo:  
grep -E "$(python3 DFA2Regex.py dfa.json)" test.txt