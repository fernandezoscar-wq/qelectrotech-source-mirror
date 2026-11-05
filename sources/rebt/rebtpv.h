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
#ifndef REBTPV_H
#define REBTPV_H

#include "rebttypes.h"

#include <string>

/**
	@brief The DC side of a photovoltaic generator: one group of equal
	strings in parallel on one input (MPPT) of an inverter.

	- Voltages with the temperature (module datasheet coefficients):
		UOC MAX  = Nm x Voc x [1 + bVoc/100 (Tmin - 25)]
		UMPP min = Nm x Vmpp x [1 + bVmpp/100 (Tmax - 25)]
		UMPP max = Nm x Vmpp x [1 + bVmpp/100 (Tmin - 25)]
	  UOC MAX against the inverter and the module (UNE-HD 60364-7-712,
	  712.512.1.1.102.1), the UMPP range against the MPPT window and the
	  input currents against the inverter (manufacturer's data).
	- String protection (712.430.3.103, 712.433.1.101.2):
		needed when (Ns - 1) ISC MAX > IMOD_MAX_OCPR
		1.5 ISC < In < 2.4 ISC, In <= IMOD_MAX_OCPR
	- String cable current (table 712.2): In of the string protection,
	  or 1.25 ISC (Ns - 1) without it.
	- DC voltage drop of the string cable, with IMPP and UMPP at STC,
	  under 1.5 % (IDAE PCT-C-REV 5.5.2).

	UNE-HD 60364-7-712 is not among the standards of ITC-BT-02: its
	requirements are good practice, not binding by the REBT.
*/
namespace rebt {

struct PvModule
{
	std::optional<double> openCircuitVoltage;    ///< Voc, V, STC
	std::optional<double> mppVoltage;            ///< Vmpp, V, STC
	std::optional<double> shortCircuitCurrent;   ///< Isc, A, STC
	std::optional<double> mppCurrent;            ///< Impp, A, STC
	std::optional<double> vocCoefficient;        ///< %/degC, negative
	std::optional<double> vmppCoefficient;       ///< %/degC, negative
	std::optional<double> maxSystemVoltage;      ///< V
	std::optional<double> maxSeriesFuse;         ///< IMOD_MAX_OCPR, A
	std::string source;                          ///< datasheet the values come from
};

struct PvInverterInput
{
	std::optional<double> maxDcVoltage;          ///< V
	std::optional<double> mpptMinVoltage;        ///< V
	std::optional<double> mpptMaxVoltage;        ///< V
	std::optional<double> maxInputCurrent;       ///< A, per MPPT
	std::optional<double> maxShortCircuitCurrent;///< A, per MPPT
	std::string source;
};

struct PvStringInput
{
	PvModule module;
	PvInverterInput inverter;
	std::optional<int> modulesPerString;         ///< Nm
	std::optional<int> parallelStrings;          ///< Ns, on this input
	std::optional<double> minCellTemperature;    ///< degC, lowest of the site
	std::optional<double> maxCellTemperature;    ///< degC, highest in operation
	std::string temperatureJustification;
	std::optional<double> stringFuse;            ///< In of the string protection, A
		// String cable, from the farthest module to the inverter or box
	std::optional<double> cableLength;           ///< m, one way
	std::optional<double> cableSection;          ///< mm2
	std::optional<Material> cableMaterial;
	std::optional<double> conductorTemperature;  ///< degC, for the resistivity
	std::optional<double> cableAdmissible;       ///< Iz of the string cable, A
};

struct PvStringResult
{
	Status status = Status::NotComputable;
	std::optional<double> openCircuitMax;        ///< UOC MAX, V
	std::optional<double> mppMin;                ///< UMPP at Tmax, V
	std::optional<double> mppMax;                ///< UMPP at Tmin, V
	std::optional<double> inputShortCircuit;     ///< Ns x Isc, A
	std::optional<double> inputMppCurrent;       ///< Ns x Impp, A
	std::optional<double> iscMax;                ///< 1.25 Isc, A
	std::optional<bool> protectionRequired;
	std::optional<double> fuseMin;               ///< 1.5 Isc, A (exclusive)
	std::optional<double> fuseMax;               ///< min(2.4 Isc exclusive, IMOD_MAX_OCPR)
	std::optional<double> cableRequired;         ///< A
	std::optional<double> dropPercent;           ///< DC drop of the string cable, %
	CheckOutcome voltage = CheckOutcome::NotPerformed;     ///< UOC MAX limits
	CheckOutcome mppt = CheckOutcome::NotPerformed;        ///< MPPT window
	CheckOutcome current = CheckOutcome::NotPerformed;     ///< input currents
	CheckOutcome protection = CheckOutcome::NotPerformed;  ///< string protection
	CheckOutcome cable = CheckOutcome::NotPerformed;       ///< Iz >= required
	CheckOutcome drop = CheckOutcome::NotPerformed;        ///< under 1.5 %
	CheckOutcome outcome = CheckOutcome::NotPerformed;
	std::vector<Diagnostic> diagnostics;
	std::vector<Assumption> assumptions;
	std::vector<NormRef> rules;
};

/// ISC MAX over ISC STC, on the safe side (Annex B of the standard not applied)
constexpr double PV_ISC_MAX_FACTOR = 1.25;
/// IDAE PCT-C-REV 5.5.2, "inferior del 1,5 %"
constexpr double PV_DC_DROP_LIMIT = 1.5;

PvStringResult checkPvString(const PvStringInput &input);

namespace rules {
NormRef pvOpenCircuitVoltage();   ///< REBT-FV-001
NormRef pvMpptWindow();           ///< REBT-FV-002
NormRef pvInputCurrents();        ///< REBT-FV-003
NormRef pvStringProtection();     ///< REBT-FV-004
NormRef pvStringCable();          ///< REBT-FV-005
NormRef pvDcVoltageDrop();        ///< REBT-FV-006
NormRef pvAcLine();               ///< REBT-FV-007, ITC-BT-40 5
} // namespace rules

} // namespace rebt

#endif // REBTPV_H
