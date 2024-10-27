# 0. Prerequisites
See https://github.com/The-OpenROAD-Project/OpenSTA/blob/master/README.md

# 1. How to build
```shell
git clone --recursive https://github.com/pumpkinblade/route_sta
cd route_sta
cmake -B build
cmake --build build
```

# 2. How to run
See https://github.com/The-OpenROAD-Project/OpenSTA/blob/master/README.md

We also implement several additional tcl commands

## Read lef/def file
### Command
The `r_read_lefdef` command reads the network andgeometry information from lef/def file.
```tcl
r_read_lefdef
  [lef_file]
  [def_file]
  [use_routing]
```
### Option
| Name | Description |
| ----- | ----- |
| `lef_file` | Path to the lef file. |
| `def_file` | Path to the def file. |
| `use_routing` | Flag to read the routing information from def file. |

## Write net slack to file
### Command
The `r_write_slack` command writes the slack of each net into a file in format "<net-name> <slack>".
```tcl
r_write_slack
  [slack_file]
```
### Option
| Name | Description |
| ----- | ----- |
| `slack_file` | Path to the file that saves the slack information. |

## Run cugr2
### Command
The "r_run_cugr2" command runs the cugr2 global routing algorithm.
```tcl
r_run_cugr2
```

## Write guide to file
### Command
The `r_write_guide` command writes the slack of each net into a file.
```tcl
r_write_guide
  [guide_file]
```
### Option
| Name | Description |
| ----- | ----- |
| `guide_file` | Path to the file that saves the guide. |

