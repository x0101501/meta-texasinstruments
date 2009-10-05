SECTION = "libs"
PRIORITY = "optional"
DESCRIPTION = "Texas Instruments MPU/DSP Bridge libraries."
LICENSE = "GPL"
PR = "r0"
DEPENDS = " \
	linux-tiomap \
        "

PACKAGES = "${PN} ${PN}-dbg ${PN}-dev"
FILES_${PN} = "${libdir}/libbridge.so ${libdir}/libbridge.so.2 ${libdir}/libqos.a ${libdir}/libqos.so.2"
FILES_${PN}-dev = "${includedir}/dspbridge"


inherit pkgconfig

S = "${WORKDIR}/git"

PV = "23.0+git+${SRCREV}"

SRC_URI = "git://dev.omapzoom.org/pub/scm/tidspbridge/userspace-dspbridge.git;protocol=git;branch=bridge-2.6.31"

SRC_URI += " \
	file://23.12-mkcross-api.patch;patch=1 \
	"

do_compile() {
	#mkdir ${S}/target
	cd ${S}/source/mpu_api/src
	oe_runmake PREFIX=${S}/source TGTROOT=${S}/source KRNLSRC=${STAGING_KERNEL_DIR} \
		BUILD=rel CMDDEFS='GT_TRACE DEBUG'

	cd ${S}/source/mpu_api/src/bridge
	mv libbridge.so libbridge.so.2
	ln -s libbridge.so.2 libbridge.so

	# don't blame me -- i voted for kodos!
	cd ${S}/source/mpu_api/src/qos
	mv libqos.a libqos.so.2
	ln -s libqos.so.2 libqos.a
}

do_stage() {
	oe_libinstall -so -C ${S}/source/mpu_api/src/bridge libbridge ${STAGING_LIBDIR}
	oe_libinstall -so -C ${S}/source/mpu_api/src/qos libqos ${STAGING_LIBDIR}
	install -d ${STAGING_INCDIR}/dspbridge
	install -m 0644 ${S}/source/mpu_api/inc/*.h ${STAGING_INCDIR}/dspbridge/
}

do_install() {
	oe_libinstall -so -C ${S}/source/mpu_api/src/bridge libbridge ${D}/${libdir}
	oe_libinstall -so -C ${S}/source/mpu_api/src/qos libqos ${D}/${libdir}
	install -d ${D}${includedir}/dspbridge
	install -m 0644 ${S}/source/mpu_api/inc/*.h ${D}${includedir}/dspbridge/
}
