# Solstice

The purpose of this program is to compute the total power collected by a
concentrated solar plant, and to evaluate various efficiencies for each primary
reflector: it compute losses due to cosine effect, to shadowing and
masking, to orientation and surface irregularities, to reflectivity and to
atmospheric transmission. The efficiency for each one of these effects is
subsequently computed for each reflector, which provides insightful information
when looking for the optimal design of a concentrated solar plant. Note that
Solstice relies on Monte-Carlo method, which means that every result is
provided with its numerical accuracy.

In addition of the aforementioned computations, Solstice can render an image of
the solar plant, either with a simple ray-caster or with a path-tracing
algorithm that correctly handles the materials of the scene.

Solstice is designed to handle complex solar plants: any number of reflectors
can be specified (planes, conics, cylindro-parabolic, etc.) and positioned in
3D space, with a possibility for 1-axis and 2-axis auto-orientation with
respect to the sun direction. CAO geometries can be added to the solar plant
thanks to the support of the STereo Lithography file format. Multiple materials
can be used, as long as the relevant physical properties are provided (matte,
mirror, dielectric, etc.). Spectral effects are also taken into account: it is
possible to define the spectral distribution of any physical property,
including the input solar spectrum and the absorption of the atmosphere, at any
spectral resolution.

Solstice has been developed in the scope of the Solstice project, in
collaboration with the
[Laboratory of Excellence Solstice](http://www.labex-solstice.fr) and the
[PROMES](http://www.promes.cnrs.fr/index.php?page=home-en) laboratory of the
National Center for Scientific Research ([CNRS](http://www.cnrs.fr/index.php)).
Refer to the Solstice man pages for more informations on the provided
functionalities.

## How to build

This program relies on the [CMake](http://www.cmake.org) and the
[RCMake](https://gitlab.com/vaplv/rcmake/) package to build.
It also depends on the
[LibYAML](http://pyyaml.org/wiki/LibYAML),
[RSys](https://gitlab.com/vaplv/rsys/),
[Solstice-Anim](https://gitlab.com/meso-star/solstice-anim/),
[Solstice-Solver](https://gitlab.com/meso-star/solstice-solver/),
[Star-3DUT](https://gitlab.com/meso-star/star-3dut/),
[Star-SP](https://gitlab.com/meso-star/star-sp/) and
[Star-STL](https://gitlab.com/meso-star/star-stm/) libraries.
The documentation is written with the
[AsciiDoc](http://www.methods.co.nz/asciidoc/) text format and relies on its
tool suite to generate HTML and/or ROFF man pages. If the AsciiDoc tools cannot
be find, the documentation will be not built.

First ensure that CMake is installed on your system. Then install the RCMake
package as well as the aforementioned prerequisites. Finally generate the
project from the `cmake/CMakeLists.txt` file by appending to the
`CMAKE_PREFIX_PATH` variable the install directories of its dependencies. The
resulting project can be edited, built, tested and installed as any CMake
project. Refer to the [CMake](https://cmake.org/documentation) for further
informations ond CMake.

## Licenses

Solstice is developed by [|Meso|Star>](http://www.meso-star.com) for the
[National Center for Scientific Research](http://www.cnrs.fr/index.php) (CNRS).
This is a free software copyright (C) CNRS 2016-2017 released under the GPL v3+
license: GNU GPL version 3 or later. You are welcome to redistribute it under
certain conditions; refer to the COPYING file for details.

