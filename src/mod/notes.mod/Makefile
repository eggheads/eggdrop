# Makefile for src/mod/notes.mod

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

clean:
	@rm -f *.o *.so *~

static: ../notes.o

modules: ../../../notes.so

../notes.o: ../module.h ../modvals.h ../../eggdrop.h cmdsnote.c notes.c \
notes.h ../../tandem.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c notes.c
	rm -f ../notes.o
	mv notes.o ../

../../../notes.so: ../notes.o
	$(LD) -o ../../../notes.so ../notes.o
	$(STRIP) ../../../notes.so

#safety hash
