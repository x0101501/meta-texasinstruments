PRIORITY = "optional"
DESCRIPTION = "Texas Instruments MPU Bridge Samples."
LICENSE = "GPL"
PR = "r0"
DEPENDS = " \
	tidspbridge-lib \
        "

PACKAGES = "${PN}-dev ${PN}-dbg ${PN}"
FILES_${PN}-dbg = "/dspbridge/.debug"
FILES_${PN} = "/dspbridge"

S = "${WORKDIR}/git"

PV = "23.0+git+${SRCREV}"

SRC_URI = "git://dev.omapzoom.org/pub/scm/tidspbridge/userspace-dspbridge.git;protocol=git;branch=bridge-2.6.31"

do_compile() {

        # FIXME: we should not compile bridge-lib here
        #        it is already done in tidspbridge-lib
        cd ${S}/source/mpu_api/src
        oe_runmake KRNLSRC=${STAGING_KERNEL_DIR} \
                PREFIX=${S}/source PROJROOT=${S}/source/mpu_api \
                ROOTFSDIR=${D}/dspbridge \
                CROSS=${AR%-*}- -f Makefile

	cp bridge/libbridge.so* ${S}/source/target/lib/
	cp bridge/libbridge.so* ${S}/source/samples/mpu/lib/
	cp qos/libqos.* ${S}/source/target/lib/
	cp qos/libqos.* ${S}/source/samples/mpu/lib/

	cd ${S}/source/samples/mpu/src
	oe_runmake KRNLSRC=${STAGING_KERNEL_DIR} \
		PREFIX=${S}/source PROJROOT=${S}/source/samples/mpu \
		ROOTFSDIR=${S}/source \
		CROSS=${AR%-*}- -f Makefile
#	BUILD=rel CMDDEFS='GT_TRACE DEBUG' \
#	PROJROOT=${S}/samples
}

do_stage() {
        echo "Nothing to stage - stage done by dspbridge-lib"
}

do_install() {
	install -d ${STAGING_BINDIR}/dspbridge/samples
        cd ${S}/source/mpu_api/src
        oe_runmake KRNLSRC=${STAGING_KERNEL_DIR} \
                PREFIX=${S}/source PROJROOT=${S}/source/mpu_api \
                ROOTFSDIR=${STAGING_BINDIR}/dspbridge/samples \
                CROSS=${AR%-*}- -f Makefile install


	cd ${S}/source/samples/mpu/src
        oe_runmake KRNLSRC=${STAGING_KERNEL_DIR} \
                PREFIX=${S}/source PROJROOT=${S}/source/samples/mpu \
                ROOTFSDIR=${STAGING_BINDIR}/dspbridge/samples \
                CROSS=${AR%-*}- -f Makefile install
	
	oenote "Installing samples in ${STAGING_BINDIR}/dspbridge/samples " 
        install -d ${D}/dspbridge/samples
	install -m 0755 ${S}/source/target/dspbridge/*.out ${D}/dspbridge/samples
	install -m 0755 ${S}/source/samples/utils/install_bridge ${D}/dspbridge/samples
	install -m 0755 ${S}/source/samples/utils/install_bridge_128 ${D}/dspbridge/samples
        install -m 0755 ${S}/source/samples/utils/uninstall_bridge ${D}/dspbridge/samples

}
