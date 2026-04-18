# Makefile for src/mod/irc.mod/

srcdir = .

SRCS = $(wildcard $(srcdir)/*.c)
OBJS = $(notdir $(SRCS:.c=.o))

doofus:
	@echo "" && \
	echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../irc.o

modules: ../../../irc.$(MOD_EXT)

../irc.o: $(OBJS) irc.h irc_proto.h
	$(CC) -r -nostdlib -o ../irc.o $(OBJS)

%.o: $(srcdir)/%.c irc.h irc_proto.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $<

../../../irc.$(MOD_EXT): ../irc.o
	$(LD) $(CFLAGS) -o ../../../irc.$(MOD_EXT) ../irc.o $(XLIBS) $(MODULE_XLIBS) && $(STRIP) ../../../irc.$(MOD_EXT)

depend:
	echo "../irc.o: $(OBJS)" > .depend; \
	for src in $(SRCS); do \
	  obj=$$(basename $$src .c).o; \
	  $(CC) $(CFLAGS) -MM $$src -MT $$obj >> .depend; \
	done

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../irc.o: chan.o cmdsirc.o irc.o mode.o msgcmds.o tclirc.o
chan.o: .././irc.mod/chan.c ../../../src/mod/module.h ../../../src/main.h \
 ../../../config.h ../../../eggint.h ../../../lush.h ../../../src/lang.h \
 ../../../src/eggdrop.h ../../../src/flags.h ../../../src/cmdt.h \
 ../../../src/tclegg.h ../../../src/tclhash.h ../../../src/chan.h \
 ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/base64.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/explicit_bzero.h \
 ../../../src/compat/strlcpy.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h .././irc.mod/irc.h .././irc.mod/irc_proto.h \
 ../../../src/mod/server.mod/server.h \
 ../../../src/mod/channels.mod/channels.h
cmdsirc.o: .././irc.mod/cmdsirc.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/flags.h \
 ../../../src/cmdt.h ../../../src/tclegg.h ../../../src/tclhash.h \
 ../../../src/chan.h ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/base64.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/explicit_bzero.h \
 ../../../src/compat/strlcpy.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h .././irc.mod/irc.h .././irc.mod/irc_proto.h \
 ../../../src/mod/server.mod/server.h \
 ../../../src/mod/channels.mod/channels.h
irc.o: .././irc.mod/irc.c ../../../src/mod/module.h ../../../src/main.h \
 ../../../config.h ../../../eggint.h ../../../lush.h ../../../src/lang.h \
 ../../../src/eggdrop.h ../../../src/flags.h ../../../src/cmdt.h \
 ../../../src/tclegg.h ../../../src/tclhash.h ../../../src/chan.h \
 ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/base64.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/explicit_bzero.h \
 ../../../src/compat/strlcpy.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h .././irc.mod/irc.h .././irc.mod/irc_proto.h \
 ../../../src/mod/server.mod/server.h \
 ../../../src/mod/channels.mod/channels.h
mode.o: .././irc.mod/mode.c ../../../src/mod/module.h ../../../src/main.h \
 ../../../config.h ../../../eggint.h ../../../lush.h ../../../src/lang.h \
 ../../../src/eggdrop.h ../../../src/flags.h ../../../src/cmdt.h \
 ../../../src/tclegg.h ../../../src/tclhash.h ../../../src/chan.h \
 ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/base64.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/explicit_bzero.h \
 ../../../src/compat/strlcpy.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h .././irc.mod/irc.h .././irc.mod/irc_proto.h \
 ../../../src/mod/server.mod/server.h \
 ../../../src/mod/channels.mod/channels.h
msgcmds.o: .././irc.mod/msgcmds.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/flags.h \
 ../../../src/cmdt.h ../../../src/tclegg.h ../../../src/tclhash.h \
 ../../../src/chan.h ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/base64.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/explicit_bzero.h \
 ../../../src/compat/strlcpy.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h .././irc.mod/irc.h .././irc.mod/irc_proto.h \
 ../../../src/mod/server.mod/server.h \
 ../../../src/mod/channels.mod/channels.h
tclirc.o: .././irc.mod/tclirc.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/flags.h \
 ../../../src/cmdt.h ../../../src/tclegg.h ../../../src/tclhash.h \
 ../../../src/chan.h ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/base64.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/explicit_bzero.h \
 ../../../src/compat/strlcpy.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h .././irc.mod/irc.h .././irc.mod/irc_proto.h \
 ../../../src/mod/server.mod/server.h \
 ../../../src/mod/channels.mod/channels.h
