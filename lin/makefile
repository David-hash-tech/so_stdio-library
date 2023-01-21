build:main

main:main.o libso_stdio.so 
	gcc main.o -lso_stdio -o main

main.o:main.c
	gcc -c main.c -o main.o

so_stdio.o:so_stdio.c
	gcc -fPIC -c so_stdio.c -o so_stdio.o

libso_stdio.so:so_stdio.o
	sudo rm /lib/libso_stdio.so
	gcc -shared so_stdio.o -o libso_stdio.so
	sudo cp libso_stdio.so /lib

clean:
	rm *.o 
	rm *.so
	rm main
