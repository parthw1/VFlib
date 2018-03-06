all:  ekdump ekreenc
clean:
	-rm -f  ekdump ekreenc core *.core *.o a.out *~ 
ekdump: ekdump.c
	$(CC) -o ekdump ekdump.c
ekreenc: ekreenc.c
	$(CC) -o ekreenc ekreenc.c
