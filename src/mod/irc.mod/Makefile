# Makefile for src/mod/irc.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

static: ../irc.o

modules: ../../../irc.so

../irc.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c irc.c
	rm -f ../irc.o
	mv irc.o ../

../../../irc.so: ../irc.o
	$(LD) -o ../../../irc.so ../irc.o
	$(STRIP) ../../../irc.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .depend

clean:
	@rm -f .depend *.o *.so *~

#safety hash
../irc.o: irc.c ../module.h ../../../config.h ../../main.h ../../lang.h \
 ../../eggdrop.h ../../flags.h ../../proto.h ../../../lush.h \
 ../../cmdt.h ../../tclegg.h ../../tclhash.h ../../chan.h \
 ../../users.h ../modvals.h ../../tandem.h irc.h \
 ../server.mod/server.h ../channels.mod/channels.h chan.c mode.c \
 cmdsirc.c msgcmds.c tclirc.c
