# Makefile for src/mod/ctcp.mod/

srcdir = .


doofus:
	@echo "" && \
	echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../ctcp.o

modules: ../../../ctcp.$(MOD_EXT)

../ctcp.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/ctcp.c && mv -f ctcp.o ../

../../../ctcp.$(MOD_EXT): ../ctcp.o
	$(LD) $(CFLAGS) -o ../../../ctcp.$(MOD_EXT) ../ctcp.o $(XLIBS) $(MODULE_XLIBS) && $(STRIP) ../../../ctcp.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/ctcp.c -MT ../ctcp.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../ctcp.o: .././ctcp.mod/ctcp.c .././ctcp.mod/ctcp.h \
 ../../../src/mod/module.h ../../../src/main.h ../../../config.h \
 ../../../eggint.h ../../../lush.h ../../../src/lang.h \
 ../../../src/eggdrop.h ../../../src/compat/in6.h ../../../src/flags.h \
 ../../../src/cmdt.h ../../../src/tclegg.h ../../../src/tclhash.h \
 ../../../src/chan.h ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/base64.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/explicit_bzero.h \
 ../../../src/compat/strlcpy.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h ../../../src/mod/server.mod/server.h
