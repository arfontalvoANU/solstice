/* Copyright (C) CNRS 2016-2018
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>. */

#ifndef SOLSTICE_SUN_SPECTRUM_H
#define SOLSTICE_SUN_SPECTRUM_H

#include <rsys/rsys.h>

/*
 * Each built-in spectrum is a list of double2: [Wavelength in nano-meter,
 * DNI]. Its associated size is the number of double[2] into the spectrum
 */

extern LOCAL_SYM const double solstice_sun_spectrum_dummy[];
extern LOCAL_SYM const size_t solstice_sun_spectrum_dummy_size;

extern LOCAL_SYM const double solstice_sun_spectrum_smarts[];
extern LOCAL_SYM const size_t solstice_sun_spectrum_smarts_size;

#endif /* SOLSTICE_SUN_SPECTRUM_H */

