sccs: main.o hashTable.o
    gcc -o sccs.exe main.o hashTable.o

main.o: main.c
    gcc -c main.c

hashTable.o: hashTable.c hashTable.h
    gcc -c hashTable.c