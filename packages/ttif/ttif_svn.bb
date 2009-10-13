SECTION = "libs"
DESCRIPTION = "Test and Trace InterFace"
PR = "r3"


# experimential ttifdaemon version:
#SRC_URI = "svn://plato.googlecode.com/svn/branches/ttifd/nonjava/target;module=${PN};rev=14357;proto=http"

# stable version:  (in doubt, use this!)
SRC_URI = "svn://plato.googlecode.com/svn/trunk/nonjava/target;module=${PN};rev=14362;proto=http"

S = ${WORKDIR}/${PN}

inherit autotools pkgconfig

do_stage() {
	autotools_stage_all
}

