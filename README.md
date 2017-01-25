# Solstice

The purpose of this program is to estimate the solar flux in complex solar
facilities. It has been developed in the scope of the Solstice project, in
collaboration with the
[Laboratory of Excellence Solstice](http://www.labex-solstice.fr) and the
[PROMES](http://www.promes.cnrs.fr/index.php?page=home-en) laboratory of the
National Center for Scientific Research ([CNRS](http://www.cnrs.fr/index.php)).

## How to build

This program relies on the [CMake](http://www.cmake.org) and the
[RCMake](https://gitlab.com/vaplv/rcmake/) package to build.
It also depends on the
[LibYAML](http://pyyaml.org/wiki/LibYAML),
[RSys](https://gitlab.com/vaplv/rsys/),
[Solstice-Anim](https://gitlab.com/meso-star/solstice-anim/),
[Solstice-Solver](https://gitlab.com/meso-star/solstice-solver/),
[Star-3DUT](https://gitlab.com/meso-star/star-3dut/) and,
[Star-SP](https://gitlab.com/meso-star/star-sp/) libraries.

First ensure that CMake is installed on your system. Then install the RCMake
package as well as all the aforementioned prerequisites. Finally generate the
project from the `cmake/CMakeLists.txt` file by appending to the
`CMAKE_PREFIX_PATH` variable the install directories of its dependencies.

## Licenses

Solstice is developed by [|Meso|Star>](http://www.meso-star.com) for the
[National Center for Scientific Research](http://www.cnrs.fr/index.php) (CNRS).
It is a free software copyright (C) CNRS 2016-2017 and it is released under the
[OSI](http://opensource.org)-approved GPL v3+ license. You are welcome to
redistribute it under certain conditions; refer to the COPYING file for
details.

