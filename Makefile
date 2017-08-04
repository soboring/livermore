INSTALL_TARGET = livermore
all:  livermore.m 
	+$(MAKE) -f livermore.m $@
clean : livermore.m 
	-$(MAKE) -f livermore.m $@

clobber : clean
	-rm -f livermore.m
livermore.m: livermore.pro $(TMAKE)
	$(TMAKE) livermore.pro > $@
install: all
	test -d "$(INSTALL_DIR)"
	cp $(INSTALL_TARGET) $(INSTALL_DIR)
test:
	@echo "no test case"
