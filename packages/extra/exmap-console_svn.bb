require exmap-console.inc

PV = "0.4+svnr${SRCREV}"
PR = "r5"

SRC_URI = "svn://svn.o-hand.com/repos/misc/trunk;module=exmap-console;proto=http \
           file://exmap-console-proc_fs.patch;patch=1 \
          "

S = "${WORKDIR}/exmap-console"
