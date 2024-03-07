# mygrep
Implement a grep.  
Support all regular language: *,?,|  
And support parentheses: (,)  
And support escape for them: \\    
Parse regular expression and convert it into NFA, then simulate NFA on the fly.  
## Usage
make  
./mygrep pattern input_file_name  
If no input_file_name, read from stdin.  
Examples:  
./mygrep '(a|b)*abb' yes.txt  
./mygrep '(a|b)*abb' no.txt  

## Complexity  
r is the length of regular expression.  
m is the number of edges.  
n is the number of nodes.  
### Time complexity  
Parse and convert into NFA: O(r).  
Simulate NFA on the fly: O(|x| * (m+n)) = O(|x| * |r|)  
### Space complexity  
Parse and convert into NFA: O(r).  
Simulate NFA on the fly: O(n).  

