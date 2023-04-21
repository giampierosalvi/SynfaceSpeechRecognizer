NICOVER=1.1.2

dependencies: nico

nico:
	wget https://sourceforge.net/projects/nico/files/nico/nico-$(NICOVER)/nico-$(NICOVER).tar.gz
	tar xvfz nico-$(NICOVER).tar.gz
	patch nico-$(NICOVER)/lib/System.c < System1.1.2_c.patch
	patch nico-$(NICOVER)/lib/RTDNN.c < RTDNN1.1.2_c.patch
	patch nico-$(NICOVER)/lib/RTSim.c < RTSim1.1.2_c.patch
	patch nico-$(NICOVER)/lib/Simulation.c < Simulation1.1.2_c.patch
	patch nico-$(NICOVER)/lib/Makefile < Makefile1.1.2.patch
	make -C nico-$(NICOVER)/lib 

clean:
	rm -f nico-$(NICOVER).tar.gz*
	rm -rf nico-$(NICOVER)
