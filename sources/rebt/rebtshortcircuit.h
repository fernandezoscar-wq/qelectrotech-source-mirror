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
#ifndef REBTSHORTCIRCUIT_H
#define REBTSHORTCIRCUIT_H

#include "rebtprotection.h"
#include "rebttypes.h"

/**
	@brief Short circuit currents of a circuit and the checks of its
	protection against them (ITC-BT-22 1.1 b, UNE-HD 60364-4-43 434).

	Two ways to know the supply, chosen by the user for the origin:

	- GuideFactor: the network is not known and the origin is the CGP.
	  Guia-BT Anexo 3: Icc = 0.8 U0 / R, R the resistance of the lines
	  from the CGP, outgoing and return conductors, at 20 degC.
	- KnownCurrent: the Icc at the origin is known (distributor, own
	  transformer study, measurement). The source is then an impedance
	  Zs = U0 / Icc whose angle is not known; the results are bounds
	  that hold for any angle between 0 and 90 degrees:
		maximum  U0 / sqrt(Zs^2 + R^2)   (Zs taken as a pure reactance)
		minimum  U0 / (Zs + |Z line|)    (impedances added as moduli)

	Maximum: at the board the circuit leaves from, resistances at
	20 degC. Minimum: phase to neutral fault at the far end of the
	circuit, resistances at the end of the fault (IEC 60909-0):
		R = [1 + 0.004 (theta_e - 20)] R20
	with theta_e the highest short circuit temperature of the insulation
	(160 degC PVC up to 300 mm2, 140 degC above; 250 degC XLPE, EPR).
*/
namespace rebt {

enum class SourceMethod {
	GuideFactor,   ///< 0.8 U0 / R from the CGP
	KnownCurrent   ///< Icc at the origin given
};

/// Instantaneous trip characteristic of UNE-EN 60898-1 / 61009-1
enum class TripCurve { B, C, D };

/**
	@brief One line of the fault path: a feeder or the circuit itself
*/
struct LineStretch
{
	std::optional<double> length;          ///< m, one way
	std::optional<double> section;         ///< mm2, phase
	std::optional<double> neutralSection;  ///< mm2
	std::optional<Material> material;
	std::optional<Insulation> insulation;
	std::optional<double> reactance;       ///< ohm/km, per conductor
		/// interior installation: ITC-BT-19 2.2.2, neutral at least the phase
	bool interior = false;
	std::optional<double> protectiveSection;   ///< mm2, PE (indirect contacts only)
};

struct SourceData
{
	std::optional<SourceMethod> method;
	std::optional<double> maxCurrent;   ///< A, KnownCurrent
	std::optional<double> minCurrent;   ///< A, KnownCurrent
};

struct ShortCircuitInput
{
	SourceData source;
	std::optional<double> phaseVoltage;    ///< U0, V
	std::vector<LineStretch> feeders;      ///< from the origin down
	LineStretch circuit;
		// Protection at the start of the circuit
	bool protectsAgainstShortCircuit = true;
	std::optional<double> breakingCapacity;  ///< A (Icu or Icn)
	std::optional<double> ratedCurrent;      ///< IN, A
	std::optional<ProtectionStandard> standard;
	std::optional<TripCurve> curve;
	std::optional<double> tripCurrent;       ///< A, instantaneous trip, given
	std::string tripCurrentJustification;
	std::optional<double> letThrough;        ///< A2s at the maximum Icc, given
		/// A, the prospective current the I2t was given for; empty: taken
		/// as given for the maximum Icc of the board
	std::optional<double> letThroughAt;
	std::string letThroughJustification;
};

struct ShortCircuitResult
{
	Status status = Status::NotComputable;
	std::optional<double> maxCurrent;   ///< A, at the start of the circuit
	std::optional<double> minCurrent;   ///< A, at its far end
	std::optional<double> tripCurrent;  ///< A, the one used
	std::optional<double> k;            ///< of the circuit conductor
	std::optional<double> allowedTime;  ///< s, (k S / Imax)^2
	std::optional<double> conductorEnergy;  ///< k^2 S^2, A2s
	std::optional<double> letThrough;   ///< A2s, the given one when used
	CheckOutcome breaking = CheckOutcome::NotPerformed;  ///< Icu >= Imax
	CheckOutcome trip = CheckOutcome::NotPerformed;      ///< Imin >= Ia
	CheckOutcome thermal = CheckOutcome::NotPerformed;   ///< I2t <= k2S2
	CheckOutcome outcome = CheckOutcome::NotPerformed;
	std::vector<Diagnostic> diagnostics;
	std::vector<Assumption> assumptions;
	std::vector<NormRef> rules;
};

/**
	@brief k of a conductor (UNE-HD 60364-4-43 table 43A, phase
	conductors, initial temperature the highest in service)
	@return nothing for a conductor the table does not give
*/
std::optional<double> kFactor(Material material, Insulation insulation,
							  double section);

/**
	@brief Highest temperature at the end of a short circuit, degC
*/
double finalTemperature(Insulation insulation, double section);

/**
	@brief Upper limit of the instantaneous trip range, as a multiple of
	IN, of a device of @p standard with @p curve
	(B 5, C 10, D 20; UNE-EN 60898-1 and UNE-EN 61009-1 only)
*/
std::optional<double> tripFactor(ProtectionStandard standard, TripCurve curve);

ShortCircuitResult checkShortCircuit(const ShortCircuitInput &input);

struct LoopImpedanceResult
{
	Status status = Status::NotComputable;
	std::optional<double> impedance;   ///< ohm
	std::vector<Diagnostic> diagnostics;
	std::vector<Assumption> assumptions;
};

/**
	@brief |Z| of the loop formed, in every line of @p lines, by the
	conductor of section "section" and the one of "neutralSection", as
	for the minimum Icc: resistances at the end of the fault, reactances
	added when given. A missing "neutralSection" is missing, never taken
	equal. The last line is the circuit (fields without prefix), the
	others feeders ("feeder[i]/").
*/
LoopImpedanceResult hotLoopImpedance(const std::vector<LineStretch> &lines);

namespace rules {
/// Guia-BT Anexo 3: 0.8 U0 / R
NormRef guideShortCircuit();
/// Icc from the current at the origin, bounds
NormRef knownSourceShortCircuit();
/// ITC-BT-22 1.1 b: breaking capacity >= Icc max at its place
NormRef breakingCapacity();
/// UNE-EN 60898-1, 61009-1: instantaneous trip ranges
NormRef instantaneousTrip();
/// UNE-HD 60364-4-43 434.5.2: I2t <= k2 S2
NormRef thermalWithstand();
}

} // namespace rebt

#endif // REBTSHORTCIRCUIT_H
