# Makefile for src/mod/transfer.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../..; make

clean:
	@rm -f *.o *.so *~

modules: ../../../transfer.so

static: ../transfer.o

transfer.o: ../module.h ../modvals.h ../../eggdrop.h transfer.h \
../../tandem.h ../../users.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c transfer.c

../../../transfer.so: transfer.o
	$(LD) -o ../../../transfer.so transfer.o
	$(STRIP) ../../../transfer.so

../transfer.o: transfer.o
	@rm -f ../transfer.o
	@ln -s transfer.mod/transfer.o ..

#safety hash
