detect: detect.c
	gcc -Wall -g -o detect detect.c

clean:
	rm -f *.o *.plist detect