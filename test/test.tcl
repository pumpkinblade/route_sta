read_liberty ./test/Nangate45_slow.lib
# read_verilog gcd_nangate45.v
read_lefdef ./test/Nangate45.lef ./test/gcd_nangate45.def
link_design gcd
# create_clock [get_ports clk] -name core_clock -period 0.4850
# report_checks

