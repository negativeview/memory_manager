.PHONY:

mm: build/main.o
	gcc -o mm build/main.o

build/main.o: src/main.c
	gcc -o build/main.o -c src/main.c