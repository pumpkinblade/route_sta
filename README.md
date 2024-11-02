# 0. Prerequisites
## 0.1 OpenSTA
See https://github.com/The-OpenROAD-Project/OpenSTA/blob/master/README.md

## 0.2 LEMON Graph Library
See https://lemon.cs.elte.hu/trac/lemon

## 0.3 Boost C++ Libraries
```shell
# Ubuntu
sudo apt install libboost-all-dev
# Archlinux
sudo pacman -S --needed boost boost-libs
```

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
The `r_read_lef` and `r_read_def` command reads the network and geometry information from lef/def file respectively.
```tcl
r_read_lef
  [lef_file]
r_read_def
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

