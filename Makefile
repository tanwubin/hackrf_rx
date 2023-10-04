
all:
	gcc -o rx rx.c -lm -lhackrf -Wno-strict-aliasing

clean:
	rm rx

