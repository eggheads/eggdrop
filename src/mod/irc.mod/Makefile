# Makefile for src/mod/irc.mod/
# $Id: Makefile,v 1.8 2000/03/24 23:00:25 fabian Exp $

srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../irc.o

modules: ../../../irc.so

../irc.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/irc.c
	@rm -f ../irc.o
	mv irc.o ../

../../../irc.so: ../irc.o
	$(LD) -o ../../../irc.so ../irc.o
	$(STRIP) ../../../irc.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $(srcdir)/*.c > .depend

clean:
	@rm -f .depend *.o *.so *~
distclean: clean

#safety hash
chan.o: .././irc.mod/chan.c
cmdsirc.o: .././irc.mod/cmdsirc.c
irc.o: .././irc.mod/irc.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../src/lang.h ../../../src/eggdrop.h \
 ../../../src/flags.h ../../../src/proto.h ../../../lush.h \
 ../../../src/misc_file.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/memset.h \
 ../../../src/compat/memcpy.h ../../../src/compat/strcasecmp.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h ../irc.mod/irc.h \
 ../../../src/mod/server.mod/server.h \
 ../../../src/mod/channels.mod/channels.h ../irc.mod/chan.c \
 ../irc.mod/mode.c ../irc.mod/cmdsirc.c ../irc.mod/msgcmds.c \
 ../irc.mod/tclirc.c
mode.o: .././irc.mod/mode.c
msgcmds.o: .././irc.mod/msgcmds.c
tclirc.o: .././irc.mod/tclirc.c
