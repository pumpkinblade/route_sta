proc pause {{message "Hit Enter to continue ==> "}} {
    puts -nonewline $message
    flush stdout
    gets stdin
}

proc set_all_input_output_delays {{clk_period_factor .2}} {
  set clk [lindex [all_clocks] 0]
  set period [get_property $clk period]
  set delay [expr $period * $clk_period_factor]
  set_input_delay $delay -clock $clk [delete_from_list [all_inputs] [all_clocks]]
  set_output_delay $delay -clock $clk [delete_from_list [all_outputs] [all_clocks]]
}

proc setup_sta_env {} {
  create_clock [get_ports clk] -name core_clock -period 0.4850
  set_all_input_output_delays
  set_propagated_clock [all_clocks]
}

proc report_timing {} {
  # report_worst_slack -min -digits 3
  # report_worst_slack -max -digits 3
  # report_tns -digits 3
  # report_checks -path_delay min_max -format full_clock_expanded -fields {input_pin slew capacitance} -digits 3

  report_wns -digits 8
  report_tns -digits 8
  report_power
}

proc report_slack_to_file {} {
  report_slack_file
}

# ### verilog -> timing report
# read_liberty "./Nangate45/Nangate45_typ.lib"
# read_verilog "./gcd_nangate45.v"
# link_design gcd
# setup_sta_env
# report_timing
# exit

# ### def -> timing report
# read_liberty "./Nangate45/Nangate45_typ.lib"
# sca::read_lef "./Nangate45/Nangate45.lef"
# sca::read_def "./gcd_nangate45.def"
# sca::link_design gcd
# setup_sta_env
# report_timing
# exit

# ### def+guide -> timing report
# read_liberty "./Nangate45/Nangate45_typ.lib"
# sca::read_lef "./Nangate45/Nangate45.lef"
# sca::read_def "./gcd_nangate45_new.def"
# sca::link_design gcd
# sca::read_guide "./gcd_nangate45_new.guide"
# sca::estimate_parasitics
# setup_sta_env
# report_timing
# ### wns -0.06 tns -0.37 power 2.33e-3
# exit

### def+cugr2 -> timing report
read_liberty "./Nangate45/Nangate45_typ.lib"
sca::read_lef "./Nangate45/Nangate45.lef"
sca::read_def "./gcd_nangate45_new.def"
sca::link_design gcd

setup_sta_env
net_sort_enable
sca::estimate_parasitics
report_timing
sca::write_slack slackfile1
# pause "Hurry, hit enter: "

sca::run_cugr2
sca::read_guide "./gcd_nangate45_new.guide"
sca::write_guide "./gcd_nangate_new-cugr2.guide"
source ./Nangate45/setRC.tcl
sca::estimate_parasitics
sca::write_slack slackfile2

report_timing
### wns -0.06 tns -0.38 power 2.33e-3
exit

# # set_hierarchy_separator .
# read_liberty "./Nangate45/Nangate45_typ.lib"
# # sca::read_lef "./Nangate45/Nangate45.lef"
# # sca::read_def "./example1.def"
# # sca::link_design top
# read_verilog "./example1.v"
# link_design top
# create_clock -name clk -period 10 {clk1 clk2 clk3}
# set_input_delay -clock clk 0 {in1 in2}
# report_checks
# # report_arrival sca/r1/D
