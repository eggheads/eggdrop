# Makefile for src/mod/channels.mod/
# $Id: Makefile,v 1.9 2000/03/24 23:00:25 fabian Exp $

srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../channels.o

modules: ../../../channels.so

../channels.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/channels.c
	@rm -f ../channels.o
	mv channels.o ../

../../../channels.so: ../channels.o
	$(LD) -o ../../../channels.so ../channels.o
	$(STRIP) ../../../channels.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $(srcdir)/*.c > .depend

clean:
	@rm -f .depend *.o *.so *~
distclean: clean

#safety hash
channels.o: .././channels.mod/channels.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../src/lang.h ../../../src/eggdrop.h \
 ../../../src/flags.h ../../../src/proto.h ../../../lush.h \
 ../../../src/misc_file.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/memset.h \
 ../../../src/compat/memcpy.h ../../../src/compat/strcasecmp.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h \
 ../channels.mod/channels.h ../channels.mod/cmdschan.c \
 ../channels.mod/tclchan.c ../channels.mod/userchan.c \
 ../channels.mod/udefchan.c
cmdschan.o: .././channels.mod/cmdschan.c
tclchan.o: .././channels.mod/tclchan.c
udefchan.o: .././channels.mod/udefchan.c
userchan.o: .././channels.mod/userchan.c
