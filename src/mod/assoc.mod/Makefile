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

../assoc.o: ../module.h ../modvals.h ../../eggdrop.h ../../tandem.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c assoc.c
	mv assoc.o ../

../../../assoc.so: ../assoc.o
	$(LD) -o ../../../assoc.so ../assoc.o
	$(STRIP) ../../../assoc.so

#safety hash
