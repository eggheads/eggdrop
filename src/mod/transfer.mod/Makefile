# Makefile for src/mod/transfer.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

clean:
	@rm -f *.o *.so *~

static: ../transfer.o

modules: ../../../transfer.so

../transfer.o: transfer.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c transfer.c
	rm -f ../transfer.o
	mv transfer.o ../

../../../transfer.so: ../transfer.o
	$(LD) -o ../../../transfer.so ../transfer.o
	$(STRIP) ../../../transfer.so

transfer.c: transfer.h ../../tandem.h ../../users.h \
../module.h ../modvals.h ../../eggdrop.h

#safety hash
