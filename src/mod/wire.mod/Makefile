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

../wire.o: ../module.h ../modvals.h ../../eggdrop.h wire.c wire.h \
../../chan.h ../../users.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c wire.c
	mv wire.o ../

../../../wire.so: ../wire.o
	$(LD) -o ../../../wire.so ../wire.o
	$(STRIP) ../../../wire.so

#safety hash
