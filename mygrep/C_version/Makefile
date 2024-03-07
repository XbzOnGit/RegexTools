mygrep: cbitset.c cbitset.h mygrep.c nfa.c nfa.h
	gcc -O2 -o mygrep mygrep.c cbitset.c nfa.c

.PHONY: test
test: cbitset.c cbitset.h mygrep.c nfa.c nfa.h
	gcc -g -o mygrep mygrep.c cbitset.c nfa.c

.PHONY: clean
clean:
	rm mygrep
