all:
	g++ -o parsercli parsercli.cpp scanner.cpp scanner.h parser.cpp parser.h nodes.h myparser.cpp myparser.h -lfl
