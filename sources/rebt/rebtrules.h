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
#ifndef REBTRULES_H
#define REBTRULES_H

#include "rebttypes.h"

/**
	@brief The rules the calculations rest on, each with its source,
	version and limitations. One function per rule so a rule is found by
	name in the code and by id in a report.
*/
namespace rebt {
namespace rules {

/// Design current from power, voltage and power factor
NormRef designCurrent();
/// Voltage drop of one stretch, load at the end
NormRef voltageDrop();
/// Resistivity of copper and aluminium at a temperature
NormRef resistivity();

/// ITC-BT-19 2.2.2: interior circuits of a dwelling, lower than 3 %
VoltageDropLimit dwellingInterior();
/// ITC-BT-19 2.2.2: other interior installations, lighting, lower than 3 %
VoltageDropLimit otherInteriorLighting();
/// ITC-BT-19 2.2.2: other interior installations, other uses, lower than 5 %
VoltageDropLimit otherInteriorOtherUses();
/// ITC-BT-19 2.2.2: industrial, own HV transformer, lighting, at most 4.5 %
VoltageDropLimit ownTransformerLighting();
/// ITC-BT-19 2.2.2: industrial, own HV transformer, other uses, at most 6.5 %
VoltageDropLimit ownTransformerOtherUses();

} // namespace rules
} // namespace rebt

#endif // REBTRULES_H
