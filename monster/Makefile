.PHONY: all
all: main.o ../build/libmujs.a
	g++ main.o -L ../build/ -lmujs -o dump

../build/libmujs.a:
	cd ..; make 

main.o: main.cc
	g++ -c main.cc -I .. -o main.o

.PHONY: clean
clean:
	cd ..; make clean
	rm main.o
	m dump
