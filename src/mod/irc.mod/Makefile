# Makefile for src/mod/irc.mod/

srcdir = .


doofus:
	@echo "" && \
	echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../irc.o

modules: ../../../irc.$(MOD_EXT)

../irc.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/irc.c && mv -f irc.o ../

../../../irc.$(MOD_EXT): ../irc.o
	$(LD) -o ../../../irc.$(MOD_EXT) ../irc.o $(XLIBS) $(MODULE_XLIBS) && $(STRIP) ../../../irc.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/irc.c -MT ../irc.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../irc.o: .././irc.mod/irc.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/compat/in6.h \
 ../../../src/flags.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/memset.h \
 ../../../src/compat/memcpy.h ../../../src/compat/strcasecmp.h \
 ../../../src/compat/strftime.h ../../../src/compat/inet_ntop.h \
 ../../../src/compat/inet_pton.h ../../../src/compat/gethostbyname2.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h .././irc.mod/irc.h \
 ../../../src/mod/server.mod/server.h \
 ../../../src/mod/channels.mod/channels.h .././irc.mod/chan.c \
 .././irc.mod/mode.c .././irc.mod/cmdsirc.c .././irc.mod/msgcmds.c \
 .././irc.mod/tclirc.c
