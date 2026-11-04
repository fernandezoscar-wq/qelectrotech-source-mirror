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
#ifndef REBTINDIRECT_H
#define REBTINDIRECT_H

#include "rebtprotection.h"
#include "rebtshortcircuit.h"
#include "rebttypes.h"

#include <string>
#include <vector>

/**
	@brief Protection against indirect contacts by automatic disconnection
	of the supply (ITC-BT-24 4.1):

	TT (4.1.2)   RA x Ia <= U
	TN (4.1.1)   Zs x Ia <= U0          (TN-C: no residual current device)
	IT (4.1.3)   first fault  RA x Id <= UL
				 second fault, masses earthed apart: as TT
				 second fault, masses interconnected:
					 2 Zs x Ia <= U    neutral not distributed, U = sqrt(3) U0
					 2 Z's x Ia <= U0  neutral distributed

	RA  earth electrode plus protective conductors of the masses, ohm,
		given by the user (measured or estimated per ITC-BT-18)
	Ia  IDn of the residual current device; with none, the instantaneous
		trip of the overcurrent device (it clears in under 0.1 s, within
		every time of tables 1 and 2)
	U   conventional limit contact voltage, 50 V in normal conditions
	Zs  TN: source plus phase and protective conductors of every line from
		the origin; IT: phase and protective conductors from the IT board
		(Z's: neutral and protective), no source. Resistances at the end
		of the fault, as for the minimum Icc.
	Id  IT first fault current, given (it depends on the capacitances)
*/
namespace rebt {

/// ITC-BT-08. TN: TN-S or TN-C-S; TNC: TN-C, no residual current device
enum class EarthingSystem { TT, TN, IT, TNC };

struct IndirectContactInput
{
	std::optional<EarthingSystem> earthing;
	std::optional<double> earthResistance;   ///< RA, ohm
	std::string earthResistanceJustification;
	std::optional<double> limitVoltage;      ///< U, V; empty: 50 V
	std::string limitVoltageJustification;
		// The device that disconnects the circuit
	std::optional<double> residualCurrent;   ///< IDn, A, of the RCD of the circuit
		// otherwise its overcurrent protection
	std::optional<double> ratedCurrent;      ///< IN, A
	std::optional<ProtectionStandard> standard;
	std::optional<TripCurve> curve;
	std::optional<double> tripCurrent;       ///< A, instantaneous trip, given
	std::string tripCurrentJustification;
		// Fault loop (TN, IT): the lines with protectiveSection
	std::optional<double> phaseVoltage;      ///< U0, V
	SourceData source;                       ///< TN, the origin of the installation
	std::vector<LineStretch> feeders;        ///< from the origin (TN) or IT board down
	LineStretch circuit;
	bool pathKnown = true;                   ///< the chain up to the origin was followed
		// IT
	std::optional<double> firstFaultCurrent; ///< Id, A
	std::string firstFaultJustification;
	std::optional<bool> massesInterconnected;
	std::optional<bool> neutralDistributed;
};

struct IndirectContactResult
{
	Status status = Status::NotComputable;
	std::optional<double> operatingCurrent;  ///< Ia, A, the one used
	std::optional<double> limitVoltage;      ///< U (TT, IT), V
	std::optional<double> contactVoltage;    ///< RA x Ia, V (TT, IT apart)
	std::optional<double> maxResistance;     ///< U / Ia, ohm
		// TN and IT interconnected
	std::optional<double> loopImpedance;     ///< Zs (or Z's), ohm
	std::optional<double> loopVoltage;       ///< Zs x Ia, or 2 Zs x Ia, V
	std::optional<double> loopLimit;         ///< U0, or U, V
	std::optional<double> maxImpedance;      ///< largest Zs, ohm
		// IT first fault
	std::optional<double> firstFaultVoltage; ///< RA x Id, V
	CheckOutcome firstFault = CheckOutcome::NotPerformed;
	CheckOutcome secondFault = CheckOutcome::NotPerformed;
	CheckOutcome outcome = CheckOutcome::NotPerformed;
	std::vector<Diagnostic> diagnostics;
	std::vector<Assumption> assumptions;
	std::vector<NormRef> rules;
};

/// 50 V, normal conditions (ITC-BT-24 4.1)
constexpr double NORMAL_LIMIT_VOLTAGE = 50.0;

IndirectContactResult checkIndirectContact(const IndirectContactInput &input);

namespace rules {
/// REBT-CIN-001, RA x Ia <= U in TT
NormRef indirectContactTT();
/// REBT-CIN-002, Zs x Ia <= U0 in TN
NormRef indirectContactTN();
/// REBT-CIN-003, RA x Id <= UL, first fault in IT
NormRef indirectContactITFirst();
/// REBT-CIN-004, second fault in IT
NormRef indirectContactITSecond();
} // namespace rules

} // namespace rebt

#endif // REBTINDIRECT_H
