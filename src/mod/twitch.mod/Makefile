# Makefile for src/mod/twitch.mod/

srcdir = .


doofus:
	@echo "" && \
	echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../twitch.o

modules: ../../../twitch.$(MOD_EXT)

../twitch.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/twitch.c && mv -f twitch.o ../

../../../twitch.$(MOD_EXT): ../twitch.o
	$(LD) $(CFLAGS) -o ../../../twitch.$(MOD_EXT) ../twitch.o $(XLIBS) $(MODULE_XLIBS) && $(STRIP) ../../../twitch.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/twitch.c -MT ../twitch.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../twitch.o: .././twitch.mod/twitch.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/compat/in6.h \
 ../../../src/flags.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/base64.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/explicit_bzero.h ../../../src/compat/strlcpy.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h \
 ../../../src/mod/twitch.mod/twitch.h \
 ../../../src/mod/server.mod/server.h
