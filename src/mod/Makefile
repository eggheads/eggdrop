# Source-make

GMAKE=${MAKE} 'LD=${LD}' 'STRIP=${STRIP}' 'CC=${CC}' 'CFLAGS=${CFLAGS}'

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../..;make
	
clean:
	@for x in *.mod; do cd $$x;${MAKE} clean;cd ..;done
	@rm -f *.o *~ static.h
	
modules:
	for x in *.mod; do cd $$x;${GMAKE} modules;cd ..; done

static:
	@./buildstatic
	@for x in *.mod; do cd $$x;${GMAKE} static;cd ..; done
	
install:
	@for x in *.mod; do cd $$x;${GMAKE} install; cd ..; done
