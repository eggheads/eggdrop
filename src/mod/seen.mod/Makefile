# Makefile for src/mod/seen.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

static: ../seen.o

modules: ../../../seen.so

../seen.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c seen.c
	rm -f ../seen.o
	mv seen.o ../

../../../seen.so: ../seen.o
	$(LD) -o ../../../seen.so ../seen.o
	$(STRIP) ../../../seen.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .depend

clean:
	@rm -f .depend *.o *.so *~

#safety hash
../seen.o: seen.c ../module.h ../../../config.h ../../main.h ../../lang.h \
 ../../eggdrop.h ../../flags.h ../../proto.h ../../../lush.h \
 ../../cmdt.h ../../tclegg.h ../../tclhash.h ../../chan.h \
 ../../users.h ../modvals.h ../../tandem.h ../channels.mod/channels.h
