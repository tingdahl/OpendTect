#_______________________Pmake___________________________________________________
#
#	Makefile : 	Visualization - OpendTect Specific
# 	Jan 2002	K. Tingdahl
#_______________________________________________________________________________

set(OD_MODULE_DEPS visBase EarthModel MPEEngine Well PreStackProcessing )
set(OD_MODULE_SOURCES
		initvissurvey.cc
		visarrowdisplay.cc
		visemobjdisplay.cc
		visemsticksetdisplay.cc
		visfaultdisplay.cc
		visfaultsticksetdisplay.cc
		vishorizondisplay.cc
		vishorizon2ddisplay.cc
		visimagedisplay.cc
		vislocationdisplay.cc
		vismarchingcubessurfacedisplay.cc
		vismpe.cc
		vismpeeditor.cc
		vismpeseedcatcher.cc
		vismultiattribsurvobj.cc
		vispicksetdisplay.cc
		visplanedatadisplay.cc
		vispointsetdisplay.cc
		vispolygonbodydisplay.cc
		vispolylinedisplay.cc
		visprestackdisplay.cc
		vispseventdisplay.cc
		visrandomposbodydisplay.cc
		visrandomtrackdisplay.cc
		visscalebardisplay.cc
		visseis2ddisplay.cc
		vissower.cc
		vissurvobj.cc
		vissurvscene.cc
		vistransmgr.cc
		visvolumedisplay.cc
		viswelldisplay.cc
)
OD_INIT_MODULE()
