source lib_setup.tcl
source set_design.tcl
source helper.tcl

### SET DESIGN ###
set DESIGN_NAME ariane
set DESIGN_DIR "./visible"
set DEF_FILE "${DESIGN_DIR}/${DESIGN_NAME}/${DESIGN_NAME}.def"
set NETLIST_FILE "${DESIGN_DIR}/${DESIGN_NAME}/${DESIGN_NAME}.v.gz"
set SDC_FILE "${DESIGN_DIR}/${DESIGN_NAME}/${DESIGN_NAME}.sdc"

foreach lib_file ${LIB_FILES} {
  read_liberty ${lib_file}
}
foreach lef_file ${LEF_FILES} {
  sca::read_lef ${lef_file}
}

# read_verilog ${NETLIST_FILE}
# link_design ${DESIGN_NAME}
set_hierarchy_separator .
sca::read_def ${DEF_FILE}
sca::link_design ${DESIGN_NAME}
read_sdc ${SDC_FILE}
report_checks
report_wns
report_tns
# report_power
# report_net clk_i
# report_checks -unconstrained
# report_arrival ex_stage_i/i_mult/word_op_q_reg/D

# exit
