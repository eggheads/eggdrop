# Makefile for src/mod/assoc.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../..; make

clean:
	@rm -f *.o *.so *~

modules: ../../../assoc.so

static: ../assoc.o
	
../../../assoc.so: assoc.o
	$(LD) -o ../../../assoc.so assoc.o
	$(STRIP) ../../../assoc.so

assoc.o: ../module.h ../modvals.h ../../eggdrop.h \
	../../tandem.h
	$(CC) $(CFLAGS) -DMAKING_MODS -c assoc.c

../assoc.o: assoc.o
	@rm -f ../assoc.o
	@ln -s assoc.mod/assoc.o ..
	
#safety hash
