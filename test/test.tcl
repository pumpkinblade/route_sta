proc set_all_input_output_delays {{clk_period_factor .2}} {
  set clk [lindex [all_clocks] 0]
  set period [get_property $clk period]
  set delay [expr $period * $clk_period_factor]
  set_input_delay $delay -clock $clk [delete_from_list [all_inputs] [all_clocks]]
  set_output_delay $delay -clock $clk [delete_from_list [all_outputs] [all_clocks]]
}
# sca::read_lef ./test/Nangate45/Nangate45.lef
# sca::read_def ./test/gcd_nangate45.def 0
# sca::run_cugr2
# read_liberty ./test/Nangate45/Nangate45_slow.lib
# link_design gcd
# create_clock [get_ports clk] -name core_clock -period 0.4850
# sca::write_slack ./test/results/slack.txt

# sca::read_lef ./test/Nangate45/Nangate45.lef
# sca::read_lef ./test/Nangate45/fakeram45_256x16.lef
# sca::read_def ./test/ispd24/ariane133_51.def 0
# sca::run_cugr2

# read_liberty ./test/Nangate45/Nangate45_slow.lib
# sca::read_lef ./test/Nangate45/Nangate45.lef
# sca::read_def ./test/example1.def 0
# # read_verilog ./test/example1.v
# link_design top
# # read_spef ./test/example1.dspef
# sca::test
# create_clock -name clk -period 10 {clk1 clk2 clk3}
# set_input_delay -clock clk 0 {in1 in2}
# report_checks

# read_verilog "./test/gcd_nangate45.v"
sca::read_lef ./test/Nangate45/Nangate45.lef
sca::read_def ./test/gcd_nangate45_new.def 0
sca::read_guide ./test/gcd_nangate45_new.guide
# sca::test
# sca::run_cugr2
read_liberty "./test/Nangate45/Nangate45_typ.lib"
link_design gcd
create_clock [get_ports clk] -name core_clock -period 0.4850
set_all_input_output_delays
set_propagated_clock [all_clocks]
sca::estimate_parasitics
report_checks -path_delay min_max -format full_clock_expanded -fields {input_pin slew capacitance} -digits 3
report_worst_slack -min -digits 3
report_worst_slack -max -digits 3
report_tns -digits 3
exit

