set libdir "./NanGate45/lib"
set lefdir "./NanGate45/lef"

# set LEF_FILES [glob ${lefdir}/*.lef]
# set LIB_FILES [glob ${libdir}/*.lib]

# set qrcdir "./NanGate45/qrc"

set LIB_FILES "
    ${libdir}/NangateOpenCellLibrary_typical.lib \
    ${libdir}/fakeram45_256x16.lib \
    ${libdir}/fakeram45_256x64.lib \
    ${libdir}/fakeram45_32x32.lib \
    ${libdir}/fakeram45_128x116.lib \
    ${libdir}/fakeram45_256x48.lib \
    ${libdir}/fakeram45_512x64.lib \
    ${libdir}/fakeram45_64x62.lib \
    ${libdir}/fakeram45_64x124.lib \
    "

set LEF_FILES "
    ${lefdir}/NangateOpenCellLibrary.tech.lef \
    ${lefdir}/NangateOpenCellLibrary.macro.mod.lef \
    ${lefdir}/fakeram45_256x16.lef \
    ${lefdir}/fakeram45_256x64.lef \
    ${lefdir}/fakeram45_32x32.lef \
    ${lefdir}/fakeram45_128x116.lef \
    ${lefdir}/fakeram45_256x48.lef \
    ${lefdir}/fakeram45_512x64.lef \
    ${lefdir}/fakeram45_64x62.lef \
    ${lefdir}/fakeram45_64x124.lef \
    "


# set QRC_FILE "${qrcdir}/NG45.tch"

# setDesignMode -process 45

