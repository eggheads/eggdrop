# Makefile for src/mod/irc.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../..; make

clean:
	@rm -f *.o *.so *~

modules: ../../../irc.so

static: ../irc.o

irc.o: ../module.h ../modvals.h ../../eggdrop.h chan.c cmdsirc.c mode.c \
msgcmds.c tclirc.c irc.h ../channels.mod/channels.h ../server.mod/server.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c irc.c

../../../irc.so: irc.o
	$(LD) -o ../../../irc.so irc.o
	$(STRIP) ../../../irc.so

../irc.o: irc.o
	@rm -f ../irc.o
	@ln -s irc.mod/irc.o ..

#safety hash
