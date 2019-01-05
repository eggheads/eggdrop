# Makefile for src/mod/share.mod/

srcdir = .


doofus:
	@echo "" && \
	echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../share.o

modules: ../../../share.$(MOD_EXT)

../share.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/share.c && mv -f share.o ../

../../../share.$(MOD_EXT): ../share.o
	$(LD) $(CFLAGS) -o ../../../share.$(MOD_EXT) ../share.o $(XLIBS) $(MODULE_XLIBS) && $(STRIP) ../../../share.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/share.c -MT ../share.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../share.o: .././share.mod/share.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/compat/in6.h \
 ../../../src/flags.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/strcasecmp.h \
 ../../../src/compat/strftime.h ../../../src/compat/inet_ntop.h \
 ../../../src/compat/inet_pton.h ../../../src/compat/gethostbyname2.h \
 ../../../src/compat/strlcpy.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/mod/transfer.mod/transfer.h \
 ../../../src/mod/channels.mod/channels.h .././share.mod/share.h \
 .././share.mod/uf_features.c
