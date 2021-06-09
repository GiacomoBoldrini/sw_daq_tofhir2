# sw_daq_tofhir2
Software for TOFHiR 2

# General
- Machine: DELL
- Processor: Intel Core i5-6200U CPU @ 2.30GHz x 4
- OS: Ubuntu 20.04.2 LTS
- OS Type: 64-bit

# Guidelines
- Install the dependencies with `INSTALL_SW_REQUIREMENTS_LINUX`. You do not need to install drivers if your kernel is already compiled. I suppose you have root package installed, if not follow the instructions at https://root.cern/install/
- Dependencies on ubuntu presents essential packages (compilers like gcc and so on under the `build-essential`, boost libraries https://www.boost.org/, iniparser https://github.com/ndevilla/iniparser for parsing .ini config filles, xterm and dkms

# Build packages
```
cmake .
```
Python bindings can be specified directly if not found. Refer to cmake documentation for more https://cmake.org/:
```
cmake . -DPYTHON_INCLUDE_DIR=/usr/inclue/python3.8 -DPYTHON_LIBRARY=/usr/lib/x86_64-linux-gnu/libpython3.8.so
```

to check the include and library path of your python:
```
python -c "from sysconfig import get_paths as gp; print(gp())"
python3 -c "from sysconfig import get_paths as gp; print(gp())"
```
