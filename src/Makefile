# Source-make

GMAKE = ${MAKE} 'CC=${CC}' 'CFLAGS=${CFLAGS}' 

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ..;make

../eggdrop:  ${OBJS} ${XREQ} eggdrop.h
	@echo ""
	@echo "Linking eggdrop..."
	@echo ""
	${LD} -o ../eggdrop ${OBJS} ${XLIBS}
	${STRIP} ../eggdrop
	@echo "Successful compile: eggdrop"
	@echo ""
	
eggdrop: ../eggdrop

eggdrop.h: 
	@touch eggdrop.h

static: ${OBJS} ${XREQ}
	@echo ""
	@echo "Linking eggdrop...(statically)"
	@echo ""
	${LD} -o ../eggdrop ${OBJS} ${XLIBS} mod/*.o
	${STRIP} ../eggdrop
	@echo "Successful compile: eggdrop"
	@echo ""

debug: ${OBJS} ${XREQ}
	@echo ""
	@echo "Linking eggdrop (debug version)..."
	@echo ""
	${LD} -g -o ../eggdrop ${OBJS} ${XLIBS}
	@echo "Successful compile: eggdrop (debug version)"
	@echo ""

botcmd.o: botcmd.c
	@echo "(This will take a while. Go make a pizza or something.)"
	${CC} ${CFLAGS} botcmd.c

botmsg.o: botmsg.c
	${CC} ${CFLAGS} botmsg.c

botnet.o: botnet.c
	${CC} ${CFLAGS} botnet.c
	
chanprog.o: chanprog.c
	${CC} ${CFLAGS} chanprog.c
	
cmds.o: cmds.c
	${CC} ${CFLAGS} cmds.c

dcc.o: dcc.c
	${CC} ${CFLAGS} dcc.c
	
dccutil.o: dccutil.c
	${CC} ${CFLAGS} dccutil.c

flags.o: flags.c
	${CC} ${CFLAGS} flags.c

language.o: language.c 
	${CC} ${CFLAGS} language.c

main.o: main.c patch.h 
	${CC} ${CFLAGS} \
	'-DCCFLAGS="${CC}"' \
	'-DLDFLAGS="${LD}"' \
	'-DSTRIPFLAGS="${STRIP}"' main.c

mem.o: mem.c
	${CC} ${CFLAGS} mem.c
	
misc.o: misc.c
	${CC} ${CFLAGS} misc.c

modules.o: modules.c
	${CC} ${CFLAGS} modules.c

net.o: net.c
	${CC} ${CFLAGS} net.c

rfc1459.o: rfc1459.c
	${CC} ${CFLAGS} rfc1459.c

tcl.o: tcl.c
	${CC} ${CFLAGS} tcl.c
	
tcldcc.o: tcldcc.c
	${CC} ${CFLAGS} tcldcc.c

tclhash.o: tclhash.c
	${CC} ${CFLAGS} tclhash.c
	
tclmisc.o: tclmisc.c
	${CC} ${CFLAGS} tclmisc.c

tcluser.o: tcluser.c
	${CC} ${CFLAGS} tcluser.c

userent.o: userent.c
	${CC} ${CFLAGS} userent.c

userrec.o: userrec.c
	${CC} ${CFLAGS} userrec.c

users.o: users.c
	${CC} ${CFLAGS} users.c
	@echo "---------- Yeah! that's the compiling, now the linking! ----------"

# I orginially had this in another file, but some lame make's can't cope :/ 
botcmd.o: \
   main.h \
   tandem.h \
   users.h \
   chan.h \
   modules.h
botmsg.o: \
   main.h \
   tandem.h
botnet.o: \
   main.h \
   tandem.h
chanprog.o: \
   main.h \
   modules.h
cmds.o: \
   main.h \
   tandem.h \
   modules.h
dcc.o: \
   main.h \
   modules.h \
   tandem.h
dccutil.o: \
   main.h \
   chan.h \
   modules.h \
   tandem.h
flags.o: \
   main.h
language.o: \
   main.h
main.o: \
   main.h \
   chan.h \
   modules.h \
   patch.h
mem.o: \
   mod/modvals.h
misc.o: \
   main.h \
   chan.h
modules.o: \
   main.h \
   modules.h \
   users.h
net.o: \
   main.h
tcl.o: \
   main.h
tcldcc.o: \
   main.h \
   tandem.h
tclhash.o: \
   main.h \
   chan.h \
   users.h \
   match.c
tclmisc.o: \
   main.h \
   modules.h
tcluser.o: \
   main.h \
   users.h \
   chan.h \
   tandem.h
userent.o: \
   main.h \
   users.h
userrec.o: \
   main.h \
   users.h \
   chan.h \
   modules.h \
   tandem.h
users.o: \
   main.h \
   users.h \
   chan.h \
   modules.h \
   tandem.h
main.h: \
   lang.h \
   eggdrop.h \
   flags.h \
   proto.h \
   cmdt.h \
   tclegg.h \
   tclhash.h \
   chan.h \
   users.h
	@touch main.h
modules.h: \
   mod/modvals.h
	@touch modules.h
tclegg.h: \
   proto.h
	@touch tclegg.h
#safety hash
