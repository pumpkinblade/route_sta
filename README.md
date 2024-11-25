# 0. Prerequisites
## 0.1 OpenSTA
See https://github.com/The-OpenROAD-Project/OpenSTA/blob/master/README.md

or
```shell
sudo apt install build-essential cmake tcl-dev swig bison flex libeigen3-dev
wget https://raw.githubusercontent.com/davidkebo/cudd/refs/heads/main/cudd_versions/cudd-3.0.0.tar.gz
tar -zxvf cudd-3.0.0.tar.gz
cd cudd-3.0.0
./configure
sudo make install
```
You can use the "configure --prefix" option to install CUDD in a different directory

## 0.2 LEMON Graph Library
```shell
wget http://lemon.cs.elte.hu/pub/sources/lemon-1.3.1.tar.gz
tar -zxvf lemon-1.3.1.tar.gz
cd lemon-1.3.1
cmake -B build
cmake --build build
sudo cmake --install build
```
You can use the "cmake --install build --prefix" option install LEMON in a different directory

## 0.3 Boost C++ Libraries
```shell
sudo apt install libboost-all-dev
```

# 1. How to build
```shell
git clone --recursive https://github.com/pumpkinblade/route_sta
cd route_sta
cmake -B build
cmake --build build
```
If you install CUDD and LEMON to other directory, use "cmake -B build -DCMAKE_PREFIX_PATH=\[path to install\]" instead. 

# 2. How to run
```shell
./build/route_sta ./test/test.tcl
```

See also https://github.com/The-OpenROAD-Project/OpenSTA/blob/master/README.md

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

## Read guide file
### Command
```tcl
r_read_guide
  [guide_file]
```

### Option
| Name | Description |
| ----- | ----- |
| `guide_file` | Path to the guide file. |

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

