source lib_setup.tcl
source set_design.tcl
source helper.tcl

load_design $LIB_FILES $LEF_FILES $DESIGN_NAME $NETLIST_FILE $DEF_FILE $SDC_FILE
source "./NanGate45/setRC.tcl"
estimate_parasitics -placement
report_tns
report_wns
report_power

global_route -congestion_report_file ${DESIGN_NAME}.congestion -guide_file ${DESIGN_NAME}.guide
write_global_route_segments ${DESIGN_NAME}.route_segments
# set_propagated_clock [all_clocks]
estimate_parasitics -global_routing
report_tns
report_wns
report_power