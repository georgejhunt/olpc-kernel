OLPCBUILDBASE	= $(srctree)/olpc

# Directories
SOURCESDIR	= $(OLPCBUILDBASE)/SOURCES
RPMSDIR		= $(OLPCBUILDBASE)/RPMS

# Kernel tarball for RPM
TARBALL		= $(SOURCESDIR)/olpc-kernel.tar.bz2

# What to clean out
clean-dirs := $(RPMSDIR)
CLEAN_FILES += $(TARBALL) $(SOURCESDIR)/olpc-kernel

#
# Create a tarball out of the gittree
#
olpc-tarball $(TARBALL):
	@echo "Making tarball..."
	mkdir -p $(SOURCESDIR) && \
	cd $(SOURCESDIR) && \
	rm -f olpc-kernel && \
	ln -s $(srctree) olpc-kernel && \
	tar -X $(OLPCBUILDBASE)/.tarignore -X $(srctree)/.gitignore \
		--exclude=olpc-kernel/include/asm \
		--exclude=olpc/SOURCES/olpc-kernel -hcjf $(TARBALL) olpc-kernel/

xo_1-kernel-rpm:
	@echo "Please run the following command to build an XO-1 kernel rpm:"
	@echo "  ./olpc/buildrpm 1.0"

xo_1_5-kernel-rpm:
	@echo "Please run the following command to build an XO-1.5 kernel rpm:"
	@echo "  ./olpc/buildrpm 1.0"

xo_1_75-kernel-rpm:
	@echo "Please run the following command to build an XO-1.75 kernel rpm:"
	@echo "  ./olpc/buildrpm 1.75"

