# Makefile for src/mod/notes.mod

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../..; make

clean:
	@rm -f *.o *.so *~

modules: ../../../notes.so

static: ../notes.o
	
../../../notes.so: notes.o
	$(LD) -o ../../../notes.so notes.o
	$(STRIP) ../../../notes.so

notes.o: ../module.h ../modvals.h ../../eggdrop.h \
	cmdsnote.c notes.c notes.h ../../tandem.h
	$(CC) $(CFLAGS) -DMAKING_MODS -c notes.c

../notes.o: notes.o
	@rm -f ../notes.o
	@ln -s notes.mod/notes.o ..
	
#safety hash
