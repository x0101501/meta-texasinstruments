SECTION = "toolchains"
PRIORITY = "optional"
DESCRIPTION = "Texas Instruments DSP Toolchain"
LICENSE = "Texas Instruments"
PR = "r1"

inherit dfetch native

DIRAC_PATHFETCH = "/data/omapts/linux/dsp-tc/BIOS-${PV}"
DIRAC_PATHCOMPONENT = "BIOS-${PV}"
DIRAC_PATHCOMPONENTS = 6

do_stage() {
	chmod -R +w ${S}/*
	install -d ${STAGING_BINDIR}/dspbridge/tools
	cp -a ${S}/* ${STAGING_BINDIR}/dspbridge/tools
	chmod -R +wx ${STAGING_BINDIR}/dspbridge/tools	
	ln -s ${STAGING_BINDIR}/dspbridge/tools ${STAGING_BINDIR}/dspbridge/tools/bios_5_33_04
}
