proc load_design {libs lefs design netlist def sdc} {
  foreach libFile $libs {
    read_liberty $libFile
  }

  foreach lef $lefs {
    read_lef $lef
  }

#   read_verilog $netlist
#   link $design
  read_def $def
  #read_spef $spef
  #read_db $odbf
  read_sdc $sdc
  #set_propagated_clock [all_clocks]
  # Ensure all OR created (rsz/cts) instances are connected
  # add_global_connection -net {VDD} -inst_pattern {.*} -pin_pattern {^VDD$} -power
  # add_global_connection -net {VSS} -inst_pattern {.*} -pin_pattern {^VSS$} -ground
  # global_connect
}