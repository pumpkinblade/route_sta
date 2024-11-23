# r_read_lef ./test/Nangate45/Nangate45.lef
# r_read_def ./test/gcd_nangate45.def 0
# r_run_cugr2
# read_liberty ./test/Nangate45/Nangate45_slow.lib
# link_design gcd
# create_clock [get_ports clk] -name core_clock -period 0.4850
# r_write_slack ./test/results/slack.txt

# r_read_lef ./test/Nangate45/Nangate45.lef
# r_read_lef ./test/Nangate45/fakeram45_256x16.lef
# r_read_def ./test/ispd24/ariane133_51.def 0
# r_run_cugr2

# read_liberty ./test/Nangate45/Nangate45_slow.lib
# r_read_lef ./test/Nangate45/Nangate45.lef
# r_read_def ./test/example1.def 0
# # read_verilog ./test/example1.v
# link_design top
# # read_spef ./test/example1.dspef
# r_test
# create_clock -name clk -period 10 {clk1 clk2 clk3}
# set_input_delay -clock clk 0 {in1 in2}
# report_checks

# read_verilog "./test/gcd_nangate45.v"
r_read_lef ./test/Nangate45/Nangate45.lef
r_read_def ./test/gcd_nangate45.def 0
# r_read_guide ./test/results/gcd_nangate45_new-ref.guide
r_run_cugr2
read_liberty "./test/Nangate45/Nangate45_typ.lib"
link_design gcd
create_clock [get_ports clk] -name core_clock -period 0.4850
r_estimate_parasitics
report_worst_slack -min -digits 3
report_worst_slack -max -digits 3
report_tns -digits 3
report_checks -digits 3 -path_delay min_max
r_write_guide ./test/results/gcd_nangate45_new.guide
exit

