### SET PLATFORM ###
source lib_setup.tcl

### SET DESIGN ###
set DESIGN_NAME bsg_chip
set DESIGN_DIR "./visible"
set DEF_FILE "${DESIGN_DIR}/${DESIGN_NAME}/${DESIGN_NAME}.def"
set NETLIST_FILE  "${DESIGN_DIR}/${DESIGN_NAME}/${DESIGN_NAME}.v.gz"
set SDC_FILE "${DESIGN_DIR}/${DESIGN_NAME}/${DESIGN_NAME}.sdc"