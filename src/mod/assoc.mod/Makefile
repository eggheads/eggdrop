# Makefile for src/mod/assoc.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

clean:
	@rm -f *.o *.so *~

static: ../assoc.o

modules: ../../../assoc.so

../assoc.o: assoc.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c assoc.c
	rm -f ../assoc.o
	mv assoc.o ../

../../../assoc.so: ../assoc.o
	$(LD) -o ../../../assoc.so ../assoc.o
	$(STRIP) ../../../assoc.so

assoc.c: ../../tandem.h ../module.h ../modvals.h ../../eggdrop.h

#safety hash
