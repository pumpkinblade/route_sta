# delay calc example
read_liberty nangate45_slow.lib
read_verilog FA_STR.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3 clk4 clk5}
set_input_delay -clock clk 0 {in1 in2 in3}
report_checks
write_sdf FA_STR.sdf
