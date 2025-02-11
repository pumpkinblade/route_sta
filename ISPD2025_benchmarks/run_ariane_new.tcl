source lib_setup.tcl

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

  sca::link_design $design

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

# net_sort_enable

sca::run_cugr2
sca::estimate_parasitics
puts "success:estimate_parasitics"
sca::write_slack arianeslackfile2
sca::write_guide "./ariane_nangate_new-cugr2.guide"
report_wns
report_tns
report_power
exit
