# Makefile for src/mod/irc.mod/
# $Id: Makefile,v 1.11 2000/09/12 15:26:53 fabian Exp $

srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../irc.o

modules: ../../../irc.$(MOD_EXT)

../irc.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/irc.c
	@rm -f ../irc.o
	mv irc.o ../

../../../irc.$(MOD_EXT): ../irc.o
	$(LD) -o ../../../irc.$(MOD_EXT) ../irc.o $(XLIBS)
	$(STRIP) ../../../irc.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $(srcdir)/irc.c > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~
distclean: clean

#safety hash
../irc.o: .././irc.mod/irc.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../src/lang.h \
 ../../../src/eggdrop.h ../../../src/flags.h ../../../src/proto.h \
 ../../../lush.h ../../../src/misc_file.h ../../../src/cmdt.h \
 ../../../src/tclegg.h ../../../src/tclhash.h ../../../src/chan.h \
 ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/memset.h ../../../src/compat/memcpy.h \
 ../../../src/compat/strcasecmp.h ../../../src/compat/strftime.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h ../irc.mod/irc.h \
 ../../../src/mod/server.mod/server.h \
 ../../../src/mod/channels.mod/channels.h ../irc.mod/chan.c \
 ../irc.mod/mode.c ../irc.mod/cmdsirc.c ../irc.mod/msgcmds.c \
 ../irc.mod/tclirc.c
