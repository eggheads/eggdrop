# Makefile for src/mod/wire.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

static: ../wire.o

modules: ../../../wire.so

../wire.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c wire.c
	rm -f ../wire.o
	mv wire.o ../

../../../wire.so: ../wire.o
	$(LD) -o ../../../wire.so ../wire.o
	$(STRIP) ../../../wire.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .depend

clean:
	@rm -f .depend *.o *.so *~

#safety hash
../wire.o: wire.c ../module.h ../../../config.h ../../main.h ../../lang.h \
 ../../eggdrop.h ../../flags.h ../../proto.h ../../../lush.h \
 ../../cmdt.h ../../tclegg.h ../../tclhash.h ../../chan.h \
 ../../users.h ../modvals.h ../../tandem.h wire.h
