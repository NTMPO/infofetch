all:
	gcc -O3 infofetch.c -o infofetch -lm

install:
	install -Dm755 infofetch $(DESTDIR)/usr/bin/infofetch
