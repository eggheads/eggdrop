# Makefile for src/

OBJS = botcmd.o botmsg.o botnet.o chanprog.o cmds.o dcc.o \
	dccutil.o flags.o language.o main.o mem.o misc.o \
	modules.o net.o rfc1459.o tcl.o tcldcc.o tclhash.o \
	tclmisc.o tcluser.o userent.o userrec.o users.o md5/md5c.o

MOD_OBJS = mod/assoc.o mod/blowfish.o mod/channels.o mod/console.o \
	mod/ctcp.o mod/filesys.o mod/irc.o mod/notes.o mod/seen.o \
	mod/server.o mod/share.o mod/transfer.o mod/wire.o mod/woobie.o

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../; make

modegg: $(OBJS) $(XREQ)
	@echo ""
	@echo "Linking eggdrop..."
	@echo ""
	$(LD) -o ../$(EGGEXEC) $(OBJS) $(XLIBS)
	$(STRIP) ../$(EGGEXEC)
	@echo "Successful compile: $(EGGEXEC)"
	@echo ""
	
static: $(OBJS) $(XREQ)
	@echo ""
	@echo "Linking eggdrop... (static version)"
	@echo ""
	$(LD) -o ../$(EGGEXEC) $(OBJS) $(MOD_OBJS) $(XLIBS)
	$(STRIP) ../$(EGGEXEC)
	@echo "Successful compile: $(EGGEXEC)"
	@echo ""

memegg: $(OBJS) $(XREQ)
	@echo ""
	@echo "Linking eggdrop... (debug versrion)"
	@echo ""
	$(LD) -g -o ../$(EGGEXEC) $(OBJS) $(XLIBS)
	@echo "Successful compile: $(EGGEXEC)"
	@echo ""
	
sdebug: $(OBJS) $(XREQ)
	@echo ""
	@echo "Linking eggdrop... (static and debug version)"
	@echo ""
	$(LD) -g -o ../$(EGGEXEC) $(OBJS) $(MOD_OBJS) $(XLIBS)
	@echo "Successful compile: $(EGGEXEC)"
	@echo ""

clean:
	@rm -f *.o *.a *~

botcmd.o:
	@echo "(This will take a while. Go make a pizza or something.)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c botcmd.c

main.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) \
	'-DCCFLAGS="$(CC) $(CFLAGS) $(CPPFLAGS)"' \
	'-DLDFLAGS="$(LD)"' \
	'-DSTRIPFLAGS="$(STRIP)"' -c main.c

md5/md5c.o:
	@cd md5; $(MAKE) 'MAKE=$(MAKE)' 'CC=$(CC)' 'LD=$(LD)' \
	'STRIP=$(STRIP)' 'CFLAGS=$(CFLAGS)' md5
	@echo "---------- Yeah! that's the compiling, now the linking! ----------"

libtcle.a: $(TCLLIB)/lib$(TCLLIBFN)
	@echo "[ Fixing lib$(TCLLIBFN) -> libtcle.a ]"
	cp $(TCLLIB)/lib$(TCLLIBFN) ./libtcle.a
	chmod u+rw libtcle.a
	ar d ./libtcle.a tclMain.o
	$(RANLIB) ./libtcle.a

eggdrop.h:
	@echo "You do not have the eggdrop source!"
	@exit 1

.SUFFIXES:
.SUFFIXES: .c .h .o .a

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

botcmd.o: chan.h main.h modules.h tandem.h users.h
botmsg.o: main.h tandem.h
botnet.o: main.h tandem.h
chanprog.o: main.h modules.h
cmds.o: main.h modules.h tandem.h
dcc.o: main.h md5/global.h md5/md5.h modules.h tandem.h
dccutil.o: chan.h main.h modules.h tandem.h
flags.o: main.h
language.o: main.h
main.o: chan.h main.h modules.h patch.h tandem.h
mem.o: mod/modvals.h
misc.o: chan.h main.h
modules.o: main.h modules.h tandem.h users.h
net.o: main.h
rfc1459.o: main.h
tcl.o: main.h
tcldcc.o: main.h modules.h tandem.h
tclhash.o: chan.h main.h match.c users.h
tclmisc.o: main.h modules.h tandem.h
tcluser.o: chan.h main.h tandem.h users.h
userent.o: main.h users.h
userrec.o: chan.h main.h modules.h tandem.h users.h
users.o: chan.h main.h modules.h tandem.h users.h
main.h: ../config.h chan.h cmdt.h eggdrop.h flags.h lang.h \
	 proto.h tclegg.h tclhash.h users.h
modules.h: mod/modvals.h
proto.h: ../lush.h
tclegg.h: ../lush.h proto.h

#safety hash
