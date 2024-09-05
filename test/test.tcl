read_liberty ./test/Nangate45_slow.lib
# read_verilog ./test/gcd_nangate45.v
read_lefdef ./test/Nangate45.lef ./test/gcd_nangate45.def
link_design gcd
create_clock [get_ports clk] -name core_clock -period 0.4850
report_checks


# Startpoint: _690_ (rising edge-triggered flip-flop clocked by core_clock)
# Endpoint: _698_ (rising edge-triggered flip-flop clocked by core_clock)
# Path Group: core_clock
# Path Type: max

#   Delay    Time   Description
# ---------------------------------------------------------
#    0.00    0.00   clock core_clock (rise edge)
#    0.00    0.00   clock network delay (ideal)
#    0.00    0.00 ^ _690_/CK (DFF_X1)
#    0.31    0.31 ^ _690_/Q (DFF_X1)
#    0.03    0.34 v _434_/ZN (INV_X2)
#    0.22    0.57 ^ _435_/ZN (XNOR2_X1)
#    0.07    0.64 v _508_/ZN (OAI21_X1)
#    0.16    0.80 ^ _510_/ZN (AOI21_X1)
#    0.08    0.87 v _512_/ZN (OAI21_X1)
#    0.09    0.96 ^ _513_/ZN (INV_X2)
#    0.04    1.01 v _574_/ZN (NAND2_X4)
#    0.09    1.10 ^ _575_/ZN (NAND3_X4)
#    0.03    1.13 v _576_/ZN (INV_X8)
#    0.05    1.19 ^ _621_/ZN (NAND2_X1)
#    0.05    1.23 v _622_/ZN (OAI21_X1)
#    0.05    1.28 ^ _623_/ZN (NAND2_X1)
#    0.04    1.33 v _626_/ZN (NAND3_X1)
#    0.00    1.33 v _698_/D (DFF_X1)
#            1.33   data arrival time

#    0.48    0.48   clock core_clock (rise edge)
#    0.00    0.48   clock network delay (ideal)
#    0.00    0.48   clock reconvergence pessimism
#            0.48 ^ _698_/CK (DFF_X1)
#   -0.16    0.32   library setup time
#            0.32   data required time
# ---------------------------------------------------------
#            0.32   data required time
#           -1.33   data arrival time
# ---------------------------------------------------------
#           -1.00   slack (VIOLATED)

######################################################

# read lefdef
# Startpoint: _683_ (rising edge-triggered flip-flop clocked by core_clock)
# Endpoint: _700_ (rising edge-triggered flip-flop clocked by core_clock)
# Path Group: core_clock
# Path Type: max

#   Delay    Time   Description
# ---------------------------------------------------------
#    0.00    0.00   clock core_clock (rise edge)
#    0.00    0.00   clock network delay (ideal)
#    0.00    0.00 ^ _683_/CK (DFF_X1)
#    0.33    0.33 ^ _683_/Q (DFF_X1)
#    0.05    0.37 v _410_/ZN (INV_X1)
#    0.11    0.48 v _411_/ZN (AND2_X1)
#    0.18    0.66 ^ _412_/ZN (AOI21_X1)
#    0.05    0.71 v _413_/ZN (INV_X1)
#    0.14    0.85 ^ _414_/ZN (AOI21_X2)
#    0.07    0.92 v _512_/ZN (OAI21_X1)
#    0.09    1.01 ^ _513_/ZN (INV_X2)
#    0.06    1.07 v _574_/ZN (NAND2_X2)
#    0.08    1.16 ^ _575_/ZN (NAND3_X4)
#    0.03    1.19 v _576_/ZN (INV_X8)
#    0.04    1.23 ^ _632_/ZN (NAND2_X1)
#    0.05    1.28 v _633_/ZN (OAI21_X1)
#    0.05    1.32 ^ _634_/ZN (NAND2_X1)
#    0.05    1.37 v _637_/ZN (NAND3_X1)
#    0.00    1.37 v _700_/D (DFF_X2)
#            1.37   data arrival time

#    0.48    0.48   clock core_clock (rise edge)
#    0.00    0.48   clock network delay (ideal)
#    0.00    0.48   clock reconvergence pessimism
#            0.48 ^ _700_/CK (DFF_X2)
#   -0.17    0.32   library setup time
#            0.32   data required time
# ---------------------------------------------------------
#            0.32   data required time
#           -1.37   data arrival time
# ---------------------------------------------------------
#           -1.06   slack (VIOLATED)
