NICOVER=1.1.2

dependencies: nico

nico:
	wget https://sourceforge.net/projects/nico/files/nico/nico-$(NICOVER)/nico-$(NICOVER).tar.gz
	tar xvfz nico-$(NICOVER).tar.gz
	patch nico-$(NICOVER)/lib/System.c < System1.1.2_c.patch
	make -C nico-$(NICOVER)/lib 

