source lib_setup.tcl
# source set_design.tcl
# source helper.tcl

proc setup_sta_env {} {
  create_clock [get_ports clk] -name core_clock -period 0.4850
  set_all_input_output_delays
  set_propagated_clock [all_clocks]
}

proc load_design {libs lefs design netlist def sdc} {
  foreach libFile $libs {
    read_liberty $libFile
  }
  puts "success:readlib"

  foreach lef $lefs {
    sca::read_lef $lef
  }
  puts "success:readlef"

  sca::read_def $def
  puts "success:readdef"

  read_verilog $netlist

  # link_design $design
  sca::link_design $design
  
  #read_spef $spef
  #read_db $odbf
  read_sdc $sdc
  puts "success:readsdc"
}

### SET DESIGN ###
set DESIGN_NAME ariane
set DESIGN_DIR "./visible"
set DEF_FILE "${DESIGN_DIR}/${DESIGN_NAME}/${DESIGN_NAME}.def"
set NETLIST_FILE "${DESIGN_DIR}/${DESIGN_NAME}/${DESIGN_NAME}.v.gz"
set SDC_FILE "${DESIGN_DIR}/${DESIGN_NAME}/${DESIGN_NAME}.sdc"

load_design $LIB_FILES $LEF_FILES $DESIGN_NAME $NETLIST_FILE $DEF_FILE $SDC_FILE
source "./NanGate45/setRC.tcl"

net_sort_enable
# sca::estimate_parasitics
# puts "success:estimate_parasitics"
# sca::write_slack arianeslackfile1
# report_wns
# report_tns

sca::run_cugr2
# sca::link_design ${DESIGN_NAME}
# read_sdc ${SDC_FILE}
#sca::read_guide "./gcd_nangate45_new.guide"
#sca::write_guide "./gcd_nangate_new-cugr2.guide"
sca::estimate_parasitics
puts "success:estimate_parasitics"
sca::write_slack arianeslackfile2
report_wns
report_tns
report_power
# report_net clk_i
# report_checks -unconstrained
# report_arrival ex_stage_i/i_mult/word_op_q_reg/D

exit
