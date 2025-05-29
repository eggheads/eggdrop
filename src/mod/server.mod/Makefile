# Makefile for src/mod/server.mod/

srcdir = .


doofus:
	@echo "" && \
	echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../server.o

modules: ../../../server.$(MOD_EXT)

../server.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/server.c && mv -f server.o ../

../../../server.$(MOD_EXT): ../server.o
	$(LD) $(CFLAGS) -o ../../../server.$(MOD_EXT) ../server.o $(XLIBS) $(MODULE_XLIBS) && $(STRIP) ../../../server.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/server.c -MT ../server.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../server.o: .././server.mod/server.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/compat/in6.h \
 ../../../src/flags.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/base64.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/explicit_bzero.h ../../../src/compat/strlcpy.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h \
 .././server.mod/server.h .././server.mod/isupport.c \
 .././server.mod/tclisupport.c .././server.mod/servmsg.c \
 .././server.mod/../irc.mod/irc.h \
 .././server.mod/../channels.mod/channels.h .././server.mod/sasl.c \
 .././server.mod/cmdsserv.c .././server.mod/tclserv.c
