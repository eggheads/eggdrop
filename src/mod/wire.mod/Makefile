# Makefile for src/mod/wire.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../..; make

clean:
	@rm -f *.o *.so *~

modules: ../../../wire.so

static: ../wire.o

../../../wire.so: wire.o
	$(LD) -o ../../../wire.so wire.o
	$(STRIP) ../../../wire.so

wire.o: ../module.h ../modvals.h ../../eggdrop.h \
	wire.c wire.h ../../chan.h ../../users.h
	$(CC) $(CFLAGS) -DMAKING_MODS -c wire.c

../wire.o: wire.o
	@rm -f ../wire.o
	@ln -s wire.mod/wire.o ..

#safety hash
