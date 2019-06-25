w4118_sh: shell.c shell.h
	gcc -o w4118_sh -Wall shell.c

clean:
	rm -f w4118_sh

.PHONY: clean
