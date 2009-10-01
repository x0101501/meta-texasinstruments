CCASE_SPEC = "\
	${@base_contains("DISTRO_FEATURES", "testpatterns", "", "element patterns /main/0", d)}%\
	# LO Migration Patches%\
	element /vobs/wtbu/OMAPSW_MPU/linux/video/src/openmax_il/post_processor/src/OMX_PostProc_Utils.c ACT_VIDEOL23.17RC7-DISP%\	
	element /vobs/wtbu/OMAPSW_MPU/linux/video/src/openmax_il/video_decode/src/Makefile ACT_3630VIDEO-INT%\	
	element /vobs/wtbu/OMAPSW_MPU/linux/video/src/openmax_il/post_processor/... ACT_VIDEOL23.17RC7%\
	element /vobs/wtbu/OMAPSW_MPU/linux/system/src/openmax_il/audio_manager/src/AudioManager.c LINUX-MMSYSTEM_RLS_${PV}P4%\
	element /vobs/wtbu/OMAPSW_MPU/linux/system/src/openmax_il/audio_manager/... ACT_X0087450-FIX4ISSUESWITHLOKERNEL%\
	element /vobs/wtbu/OMAPSW_MPU/linux/system/src/openmax_il/resource_manager/... ACT_X0087450-FIX4ISSUESWITHLOKERNEL%\
	element /vobs/wtbu/OMAPSW_MPU/linux/video/...  LINUX-MMVIDEO_RLS_3.24%\
	element /vobs/wtbu/OMAPSW_MPU/linux/image/...  LINUX-MMVIDEO_RLS_3.24%\	
	element /vobs/wtbu/OMAPSW_MPU/linux/application/...  LINUX-MMVIDEO_RLS_3.24%\		
	# OMX Audio%\
	element /vobs/wtbu/OMAPSW_MPU/linux/audio/... LINUX-MMAUDIO_RLS_${PV}P5%\
	# OMX Video%\
	element /vobs/wtbu/OMAPSW_MPU/linux/video/... LINUX-MMVIDEO_RLS_${PV}P2%\
	# OMX Image%\
	element /vobs/wtbu/OMAPSW_MPU/linux/image/... LINUX-MMIMAGE_RLS_${PV}%\
	# LCML & core%\
	element /vobs/wtbu/OMAPSW_MPU/linux/system/... LINUX-MMSYSTEM_RLS_${PV}P4%\
	# OMX Application%\
	element /vobs/wtbu/OMAPSW_MPU/linux/application/... LINUX-MMAPPLICATION_RLS_${PV}P1%\
	# OMX INST2 utilities%\
        element /vobs/wtbu/OMAPSW_MPU/linux/utilities/src/inst2/... LINUX-MMUTILS_RLS_3.02.03%\
	# ROOT folder & Make files%\
	element /vobs/wtbu/OMAPSW_MPU/linux/... LINUX-MMROOT_RLS_3.20%\
	# special hack needed because root of vob is not labeled:%\
	element /vobs/wtbu/OMAPSW_MPU /main/LATEST%\
	# don't pick up anything that is not labeled%\
	element * /main/0%\
	"

# Workaround for LO convergence
# Disable resource manager support

export OMX_RESOURCEMANAGER_DISABLE=1
