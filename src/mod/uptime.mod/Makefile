# Makefile for src/mod/uptime.mod/
srcdir = .


doofus:
	@echo "" && \
	echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../uptime.o

modules: ../../../uptime.$(MOD_EXT)

../uptime.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/uptime.c && mv -f uptime.o ../

../../../uptime.$(MOD_EXT): ../uptime.o
	$(LD) $(CFLAGS) -o ../../../uptime.$(MOD_EXT) ../uptime.o $(XLIBS) $(MODULE_XLIBS) && $(STRIP) ../../../uptime.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/uptime.c -MT ../uptime.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../uptime.o: .././uptime.mod/uptime.c .././uptime.mod/uptime.h \
 .././uptime.mod/../module.h ../../../src/main.h ../../../config.h \
 ../../../eggint.h ../../../lush.h ../../../src/lang.h \
 ../../../src/eggdrop.h ../../../src/compat/in6.h ../../../src/flags.h \
 ../../../src/cmdt.h ../../../src/tclegg.h ../../../src/tclhash.h \
 ../../../src/chan.h ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/strcasecmp.h ../../../src/compat/strftime.h \
 ../../../src/compat/inet_ntop.h ../../../src/compat/inet_pton.h \
 ../../../src/compat/gethostbyname2.h ../../../src/compat/strlcpy.h \
 .././uptime.mod/../modvals.h ../../../src/tandem.h \
 .././uptime.mod/../server.mod/server.h
