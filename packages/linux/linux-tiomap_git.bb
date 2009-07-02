require linux-omap.inc

PR = "r1"

#SRC_URI = "git://lna0919116.emea.dhcp.ti.com/kernel.org/linux-omap-2.6.git;protocol=git"
SRC_URI = "git://git.kernel.org/pub/scm/linux/kernel/git/tmlind/linux-omap-2.6.git;protocol=git"

PV = "2.6+git${SRCREV}"

S = "${WORKDIR}/git"


COMPATIBLE_MACHINE = "omap-3430ldp|omap-3430sdp"
DEFAULT_PREFERENCE = "1"


# You can supply your own defconfig if you like.  See
# http://bec-systems.com/oe/html/recipes_sources.html for a full explanation
#SRC_URI_omap-3430ldp += "file://defconfig-omap-3430ldp"
SRC_URI_omap-3430sdp += "file://defconfig-omap-3430sdp"

# work-around for touchscreen problem (remove this when proper soln is found):
#ADD_DISTRO_FEATURES += "sed -i 's/# CONFIG_INTERCONNECT_IO_POSTING is not set/CONFIG_INTERCONNECT_IO_POSTING=y/' ${S}/.config"

do_stage_append() {
	install -d ${STAGING_KERNEL_DIR}/drivers/media/video/isp
	install -m 0644 ${S}/drivers/media/video/isp/*.h ${STAGING_KERNEL_DIR}/drivers/media/video/isp
	install -d ${STAGING_KERNEL_DIR}/arch/arm/mach-omap2
	install -m 0644 ${S}/arch/arm/mach-omap2/*.h ${STAGING_KERNEL_DIR}/arch/arm/mach-omap2
}
