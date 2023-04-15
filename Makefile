# build the executable from raycaster.c
all: raycaster.c
	gcc -O3 -Wall -Wextra -lm -lSDL2 -o raycaster.out raycaster.c

clean: 
	$(RM) raycaster.out