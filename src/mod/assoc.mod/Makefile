# Makefile for src/mod/assoc.mod/

srcdir = .


doofus:
	@echo "" && \
	echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../assoc.o

modules: ../../../assoc.$(MOD_EXT)

../assoc.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/assoc.c && mv -f assoc.o ../

../../../assoc.$(MOD_EXT): ../assoc.o
	$(LD) -o ../../../assoc.$(MOD_EXT) ../assoc.o $(XLIBS) $(MODULE_XLIBS) && $(STRIP) ../../../assoc.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/assoc.c -MT ../assoc.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../assoc.o: .././assoc.mod/assoc.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/compat/in6.h \
 ../../../src/flags.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/memset.h \
 ../../../src/compat/memcpy.h ../../../src/compat/strcasecmp.h \
 ../../../src/compat/strftime.h ../../../src/compat/inet_ntop.h \
 ../../../src/compat/inet_pton.h ../../../src/compat/gethostbyname2.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h .././assoc.mod/assoc.h
