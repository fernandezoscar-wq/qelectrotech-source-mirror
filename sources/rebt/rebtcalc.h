/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef REBTCALC_H
#define REBTCALC_H

#include "rebttypes.h"

namespace rebt {

/**
	@brief Design current of a circuit
	Single phase: I = P / (U cos phi)
	Three phase:  I = P / (sqrt(3) U cos phi)
	With an efficiency, P is divided by it first.
	Needs system, voltage, power and power factor.
*/
CurrentResult designCurrent(const CircuitInput &input);

/**
	@brief Resistivity of a conductor material at a temperature
	@return ohm.mm2/m
*/
double resistivity(Material material, double temperature);

/**
	@brief Voltage drop of one stretch, load at its far end
	Single phase: dU = 2 L I (R cos phi + X sin phi)
	Three phase:  dU = sqrt(3) L I (R cos phi + X sin phi)
	R = 1 / (conductivity x section), per metre; X per metre.
	The current is input.designCurrent when given, else designCurrent().
	Needs system, voltage, power factor, length, section, and either a
	conductivity or a material and a conductor temperature.
*/
VoltageDropResult voltageDrop(const CircuitInput &input);

/**
	@brief Compare a voltage drop with a limit
	Equal values (within 1e-9 %) pass a non strict limit and fail a
	strict one. A drop that could not be computed gives NotPerformed.
*/
CheckResult checkVoltageDrop(const VoltageDropResult &drop,
							 const VoltageDropLimit &limit);

} // namespace rebt

#endif // REBTCALC_H
