# Makefile for src/mod/console.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../..; make

clean:
	@rm -f *.o *.so *~

modules: ../../../console.so

static: ../console.o
	
../../../console.so: console.o
	$(LD) -o ../../../console.so console.o
	$(STRIP) ../../../console.so

console.o: ../module.h ../modvals.h ../../eggdrop.h
	$(CC) $(CFLAGS) -DMAKING_MODS -c console.c

../console.o: console.o
	@rm -f ../console.o
	@ln -s console.mod/console.o ..
	
#safety hash
