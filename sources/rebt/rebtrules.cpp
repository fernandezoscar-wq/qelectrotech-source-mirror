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
#include "rebtrules.h"

namespace {
const char *const REBT_2002 = "RD 842/2002, BOE 224 of 18/09/2002";
const char *const GUIA_BT =
	"Guia tecnica de aplicacion BT, Anexo 2 (not binding)";
}

namespace rebt {
namespace rules {

NormRef designCurrent()
{
	return {
		"REBT-I-001",
		"Circuit theory; Guia-BT Anexo 2",
		"-",
		GUIA_BT,
		"Sinusoidal steady state. Three phase load assumed balanced. "
		"No simultaneity or utilisation factor, no harmonics, no "
		"starting current."
	};
}

NormRef voltageDrop()
{
	return {
		"REBT-CDT-001",
		"Circuit theory; Guia-BT Anexo 2",
		"-",
		GUIA_BT,
		"One stretch, whole load at its far end, constant section. Not "
		"the drop accumulated from the origin of the installation."
	};
}

NormRef resistivity()
{
	return {
		"REBT-RHO-001",
		"IEC 60028 (copper), IEC 60889 (aluminium)",
		"-",
		"rho20 Cu = 1/58, Al = 0.028264 ohm.mm2/m; alpha20 Cu = 0.00393, "
		"Al = 0.00403 1/K",
		"Linear temperature law. The conductor temperature is not "
		"worked out from the load: it is an input."
	};
}

namespace {
VoltageDropLimit itcBt19(const char *id, double percent, const char *scope)
{
	VoltageDropLimit limit;
	limit.percent = percent;
	limit.strict = true; // the text says "menor del"
	limit.rule = {
		id,
		"ITC-BT-19",
		"2.2.2",
		REBT_2002,
		std::string(scope)
			+ " The limit applies from the origin of the interior installation: "
			  "checking one circuit alone is a partial check. Particular ITC "
			  "and supply conditions may set other limits."
	};
	return limit;
}
}

VoltageDropLimit dwellingInterior()
{
	return itcBt19("REBT-CDT-002", 3.0, "Any interior circuit of a dwelling.");
}

VoltageDropLimit otherInteriorLighting()
{
	return itcBt19("REBT-CDT-003", 3.0,
				   "Other interior or receiving installations, lighting.");
}

VoltageDropLimit otherInteriorOtherUses()
{
	return itcBt19("REBT-CDT-004", 5.0,
				   "Other interior or receiving installations, other uses.");
}

namespace {
VoltageDropLimit ownTransformer(const char *id, double percent, const char *scope)
{
	VoltageDropLimit limit;
	limit.percent = percent;
	limit.strict = false; // "caidas de tension maximas admisibles"
	limit.rule = {
		id,
		"ITC-BT-19",
		"2.2.2",
		REBT_2002,
		std::string(scope)
			+ " Industrial installation fed in high voltage through its own "
			  "distribution transformer, as stated for its origin board: the "
			  "interior installation starts at the transformer output. The "
			  "drop of the transformer itself is not included."
	};
	return limit;
}
}

VoltageDropLimit ownTransformerLighting()
{
	return ownTransformer("REBT-CDT-005", 4.5, "Lighting.");
}

VoltageDropLimit ownTransformerOtherUses()
{
	return ownTransformer("REBT-CDT-006", 6.5, "Other uses.");
}

} // namespace rules
} // namespace rebt
