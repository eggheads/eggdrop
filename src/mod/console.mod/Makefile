# Makefile for src/mod/console.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

clean:
	@rm -f *.o *.so *~

static: ../console.o

modules: ../../../console.so

../console.o: console.c ../module.h ../modvals.h ../../eggdrop.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c console.c
	rm -f ../console.o
	mv console.o ../

../../../console.so: ../console.o
	$(LD) -o ../../../console.so ../console.o
	$(STRIP) ../../../console.so

#safety hash
