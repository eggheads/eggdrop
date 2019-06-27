# Makefile for src/mod/transfer.mod/

srcdir = .


doofus:
	@echo "" && \
	echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../transfer.o

modules: ../../../transfer.$(MOD_EXT)

../transfer.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/transfer.c && mv -f transfer.o ../

../../../transfer.$(MOD_EXT): ../transfer.o
	$(LD) $(CFLAGS) -o ../../../transfer.$(MOD_EXT) ../transfer.o $(XLIBS) $(MODULE_XLIBS) && $(STRIP) ../../../transfer.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/transfer.c -MT ../transfer.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../transfer.o: .././transfer.mod/transfer.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/compat/in6.h \
 ../../../src/flags.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/strcasecmp.h \
 ../../../src/compat/strftime.h ../../../src/compat/inet_ntop.h \
 ../../../src/compat/inet_pton.h ../../../src/compat/gethostbyname2.h \
 ../../../src/compat/strlcpy.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h ../../../src/users.h .././transfer.mod/transfer.h \
 .././transfer.mod/transferfstat.c .././transfer.mod/transferqueue.c \
 .././transfer.mod/tcltransfer.c
