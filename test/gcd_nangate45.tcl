read_liberty Nangate45_slow.lib
read_verilog gcd_nangate45.v
link_design gcd
create_clock [get_ports clk] -name core_clock -period 0.4850
report_checks

