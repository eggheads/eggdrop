# Makefile for src/mod/wire.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

clean:
	@rm -f *.o *.so *~

static: ../wire.o

modules: ../../../wire.so

../wire.o: wire.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c wire.c
	rm -f ../wire.o
	mv wire.o ../

../../../wire.so: ../wire.o
	$(LD) -o ../../../wire.so ../wire.o
	$(STRIP) ../../../wire.so

wire.c: wire.h ../../chan.h ../../users.h \
../module.h ../modvals.h ../../eggdrop.h

#safety hash
