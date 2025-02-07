# 0. Prerequisites

## 0.1 OpenSTA

See https://github.com/The-OpenROAD-Project/OpenSTA/blob/master/README.md

or

```bash
sudo apt install build-essential cmake tcl-dev swig bison flex libeigen3-dev
wget https://raw.githubusercontent.com/davidkebo/cudd/refs/heads/main/cudd_versions/cudd-3.0.0.tar.gz
tar -zxvf cudd-3.0.0.tar.gz
cd cudd-3.0.0
./configure
sudo make install
```

You can use the "configure --prefix" option to install CUDD in a different directory

## 0.2 LEMON Graph Library

```bash
wget http://lemon.cs.elte.hu/pub/sources/lemon-1.3.1.tar.gz
tar -zxvf lemon-1.3.1.tar.gz
cd lemon-1.3.1
cmake -B build
cmake --build build
sudo cmake --install build
```

You can use the "cmake --install build --prefix" option install LEMON in a different directory

## 0.3 Boost C++ Libraries

```bash
sudo apt install libboost-all-dev
```

# 1. How to build

```bash
git clone --recursive https://github.com/pumpkinblade/route_sta
cd route_sta
cmake -B build
cmake --build build
```

If you install CUDD and LEMON to other directory, use "cmake -B build -DCMAKE_PREFIX_PATH=\[path to install\]" instead.

# 2. How to run

```bash
cd test/
../build/route_sta ./test.tcl
========new=========
cd ISPD2025_benchmarks
../build/route_sta ./run_xxx_xxxx.tcl

```

See also https://github.com/The-OpenROAD-Project/OpenSTA/blob/master/README.md

We also implement several additional tcl commands

## Read lef/def file

### Command

The `sca::read_lef` and `sca::read_def` command reads the network and geometry information from lef/def file respectively.

```tcl
sca::read_lef
  [lef_file]
sca::read_def
  [def_file]
  [use_routing]
```

### Option

| Name            | Description                                         |
| --------------- | --------------------------------------------------- |
| `lef_file`    | Path to the lef file.                               |
| `def_file`    | Path to the def file.                               |
| `use_routing` | Flag to read the routing information from def file. |

## Read guide file

### Command

```tcl
sca::read_guide
  [guide_file]
```

### Option

| Name           | Description             |
| -------------- | ----------------------- |
| `guide_file` | Path to the guide file. |

## Write net slack to file

### Command

The `sca::write_slack` command writes the slack of each net into a file in format "`<net-name>` `<slack>`".

```tcl
sca::write_slack
  [slack_file]
```

### Option

| Name           | Description                                        |
| -------------- | -------------------------------------------------- |
| `slack_file` | Path to the file that saves the slack information. |

## Run cugr2

### Command

The "sca::run_cugr2" command runs the cugr2 global routing algorithm.

```tcl
sca::run_cugr2
```

## Write guide to file

### Command

The `sca::write_guide` command writes the slack of each net into a file.

```tcl
sca::write_guide
  [guide_file]
```

### Option

| Name           | Description                            |
| -------------- | -------------------------------------- |
| `guide_file` | Path to the file that saves the guide. |
