# Makefile for src/mod/irc.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

clean:
	@rm -f *.o *.so *~

static: ../irc.o

modules: ../../../irc.so

../irc.o: irc.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c irc.c
	rm -f ../irc.o
	mv irc.o ../

../../../irc.so: ../irc.o
	$(LD) -o ../../../irc.so ../irc.o
	$(STRIP) ../../../irc.so

irc.c: chan.c mode.c cmdsirc.c msgcmds.c tclirc.c irc.h \
../channels.mod/channels.h ../server.mod/server.h \
../module.h ../modvals.h ../../eggdrop.h

#safety hash
