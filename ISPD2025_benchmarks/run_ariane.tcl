source lib_setup.tcl
# source set_design.tcl
source helper.tcl

proc setup_sta_env {} {
  create_clock [get_ports clk] -name core_clock -period 0.4850
  set_all_input_output_delays
  set_propagated_clock [all_clocks]
}

### SET DESIGN ###
set DESIGN_NAME ariane
set DESIGN_DIR "./visible"
set DEF_FILE "${DESIGN_DIR}/${DESIGN_NAME}/${DESIGN_NAME}.def"
set NETLIST_FILE "${DESIGN_DIR}/${DESIGN_NAME}/${DESIGN_NAME}.v.gz"
set SDC_FILE "${DESIGN_DIR}/${DESIGN_NAME}/${DESIGN_NAME}.sdc"

# set LIB_FILES {"./NanGate45/lib/NangateOpenCellLibrary_typical.lib"}
# set LEF_FILES {"./NanGate45/lef/NangateOpenCellLibrary.tech.lef" "./NanGate45/lef/NangateOpenCellLibrary.macro.mod.lef"}
# set LEF_FILES {"./NanGate45/lef/NangateOpenCellLibrary.tech.lef"}

foreach lib_file ${LIB_FILES} {
  read_liberty ${lib_file}
}
puts "success:readlib"
foreach lef_file ${LEF_FILES} {
  sca::read_lef ${lef_file}
}
puts "success:readlef"
sca::read_def ${DEF_FILE}
puts "success:readdef"

# read_verilog ${NETLIST_FILE}
# link_design ${DESIGN_NAME}

# irislin
# link_design ${DESIGN_NAME}
sca::link_design ${DESIGN_NAME}
puts "success:linkdesign"
# setup_sta_env
sca::estimate_parasitics
puts "success:estimate_parasitics"
sca::write_slack arianeslackfile1
report_wns
report_tns

sca::run_cugr2
# sca::link_design ${DESIGN_NAME}
# read_sdc ${SDC_FILE}
sca::estimate_parasitics
puts "success:estimate_parasitics"
sca::write_slack arianeslackfile2
report_wns
report_tns
# report_power
# report_net clk_i
# report_checks -unconstrained
# report_arrival ex_stage_i/i_mult/word_op_q_reg/D

exit
