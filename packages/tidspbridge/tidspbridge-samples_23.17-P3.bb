PRIORITY = "optional"
DESCRIPTION = "Texas Instruments MPU Bridge samples."
LICENSE = "LGPL"
PR = "r0"
RDEPENDS = "tidspbridge-samples-dsp"
DEPENDS = "linux-tiomap tidspbridge-lib tidspbridge-samples-dsp"

FILES_${PN}="/dspbridge"

inherit ccasefetch

CCASE_SPEC = "%\
	element /vobs/wtbu/OMAPSW_MPU/dspbridge/... COMPONENT_ROOT%\
	element /vobs/wtbu/OMAPSW_MPU/dspbridge/... L-MPU-BRIDGE_INT_BL_23-072709%\
	element * /main/LATEST%\
	"

#	element /vobs/wtbu/OMAPSW_MPU/dspbridge/...  L-BRIDGE-MPU_RLS_${PV}%\

CCASE_PATHFETCH = "/vobs/wtbu/OMAPSW_MPU/dspbridge"
CCASE_PATHCOMPONENT = "dspbridge"
CCASE_PATHCOMPONENTS = "3"

do_compile() {

        # FIXME: we should not compile bridge-lib here
        #        it is already done in tidspbridge-lib
        cd ${S}/mpu_api/src
        oe_runmake KRNLSRC=${STAGING_KERNEL_DIR} \
                PREFIX=${S} PROJROOT=${S}/mpu_api \
                ROOTFSDIR=${D}/dspbridge \
                CROSS=${AR%-*}- -f Makefile

	cp bridge/libbridge.so* ${S}/target/lib/
	cp bridge/libbridge.so* ${S}/samples/mpu/lib/
	cp qos/libqos.* ${S}/target/lib/
	cp qos/libqos.* ${S}/samples/mpu/lib/

	cd ${S}/samples/mpu/src
	oe_runmake KRNLSRC=${STAGING_KERNEL_DIR} \
		PREFIX=${S} PROJROOT=${S}/samples/mpu \
		ROOTFSDIR=${S} \
		CROSS=${AR%-*}- -f Makefile
#	BUILD=rel CMDDEFS='GT_TRACE DEBUG' \
#	PROJROOT=${S}/samples
}

do_stage() {
        echo "Nothing to stage - stage done by dspbridge-lib"
#	cd ${S}/mpu_driver
#	install -d ${STAGING_INCDIR}/dspbridge
#        tar -C inc -cf - . | tar -C ${STAGING_INCDIR}/dspbridge -xvf -	
}

do_install() {
	install -d ${STAGING_BINDIR}/dspbridge/samples
        cd ${S}/mpu_api/src
        oe_runmake KRNLSRC=${STAGING_KERNEL_DIR} \
                PREFIX=${S} PROJROOT=${S}/mpu_api \
                ROOTFSDIR=${STAGING_BINDIR}/dspbridge/samples \
                CROSS=${AR%-*}- -f Makefile install


	cd ${S}/samples/mpu/src
        oe_runmake KRNLSRC=${STAGING_KERNEL_DIR} \
                PREFIX=${S} PROJROOT=${S}/samples/mpu \
                ROOTFSDIR=${STAGING_BINDIR}/dspbridge/samples \
                CROSS=${AR%-*}- -f Makefile install
	
	oenote "Installing samples in ${STAGING_BINDIR}/dspbridge/samples " 
        install -d ${D}/dspbridge/samples
	install -m 0755 ${S}/target/dspbridge/*.out ${D}/dspbridge/samples
	install -m 0755 ${S}/samples/utils/install_bridge ${D}/dspbridge/samples
	install -m 0755 ${S}/samples/utils/install_bridge_128 ${D}/dspbridge/samples
        install -m 0755 ${S}/samples/utils/uninstall_bridge ${D}/dspbridge/samples

}
