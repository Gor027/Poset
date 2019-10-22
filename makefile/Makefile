poset.o: ../src/poset.cc
	g++ -Wall -Wextra -O2 -std=c++17 -c ../src/poset.cc -o poset.o

poset_example1.o: ../src/poset_example1.c
	gcc -Wall -Wextra -O2 -std=c11 -c ../src/poset_example1.c -o poset_example1.o

poset_example2.o: ../src/poset_example2.cc
	g++ -Wall -Wextra -O2 -std=c++17 -c ../src/poset_example2.cc -o poset_example2.o

poset_example1: poset_example1.o poset.o
	g++ poset_example1.o poset.o -o poset_example1

poset_example2: poset_example2.o poset.o
	g++ poset_example2.o poset.o -o poset_example2

.PHONY: all
all: poset_example1 poset_example2

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf poset_example{1,2}
