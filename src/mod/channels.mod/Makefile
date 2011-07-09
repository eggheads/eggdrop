# Makefile for src/mod/channels.mod/
# $Id: Makefile,v 1.18 2011/07/09 15:07:48 thommey Exp $

srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../channels.o

modules: ../../../channels.$(MOD_EXT)

../channels.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/channels.c
	@rm -f ../channels.o
	mv channels.o ../

../../../channels.$(MOD_EXT): ../channels.o
	$(LD) -o ../../../channels.$(MOD_EXT) ../channels.o $(XLIBS) $(MODULE_XLIBS)
	$(STRIP) ../../../channels.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/channels.c -MT ../channels.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../channels.o: .././channels.mod/channels.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/flags.h \
 ../../../src/cmdt.h ../../../src/tclegg.h ../../../src/tclhash.h \
 ../../../src/chan.h ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/memset.h ../../../src/compat/memcpy.h \
 ../../../src/compat/strcasecmp.h ../../../src/compat/strdup.h \
 ../../../src/compat/strftime.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h .././channels.mod/channels.h \
 .././channels.mod/cmdschan.c .././channels.mod/tclchan.c \
 .././channels.mod/userchan.c .././channels.mod/udefchan.c
