all:
	gcc -ansi -pedantic -Wall external.c -o external
	gcc -ansi -pedantic -Wall central.c -o central
	gcc -ansi -pedantic -Wall external8.c -o external8
	gcc -ansi -pedantic -Wall central8.c -o central8
	gcc -ansi -pedantic -Wall external44.c -o external44
	gcc -ansi -pedantic -Wall central44.c -o central44
	gcc -ansi -pedantic -Wall test1.c -o test1
	gcc -ansi -pedantic -Wall test2.c -o test2
clean:
	rm -f central external central8 external8 central44 external44 test1 test2
