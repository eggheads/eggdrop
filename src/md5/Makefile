# I took this from src/mod and modified it to compile the md5 stuff <Cybah>

GMAKE=${MAKE} 'LD=${LD}' 'STRIP=${STRIP}' 'CC=${CC}' 'CFLAGS=${CFLAGS}'

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../..;make
	
clean:
	@rm -f *.o *~
	
md5: global.h md5.h
	${CC} ${CFLAGS} md5c.c
