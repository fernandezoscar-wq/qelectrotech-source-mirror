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
#ifndef REBTTYPES_H
#define REBTTYPES_H

#include <optional>
#include <string>
#include <vector>

/**
	@brief Low voltage circuit calculations (Spanish REBT, RD 842/2002).

	Plain C++17, no Qt and no user interface: everything here can be tested
	without a display. Units are SI unless stated: W, V, A, m, mm2,
	ohm/km for reactance, degC for temperature, m/(ohm.mm2) for
	conductivity.

	A value that is not known stays empty (std::optional). Nothing is ever
	filled in with a default installation value: a calculation that lacks
	data says what is missing and gives no number.

	Texts in this module are identifiers and developer notes, not
	interface strings: the user interface translates the codes.
*/
namespace rebt {

enum class System {
	SinglePhase,	///< phase and neutral, voltage is phase to neutral
	ThreePhase		///< balanced three phase, voltage is line to line
};

enum class Material {
	Copper,
	Aluminium
};

enum class Insulation {
	PVC,    ///< thermoplastic, 70 degC in normal service
	XLPE,   ///< thermosetting, 90 degC
	EPR     ///< thermosetting, 90 degC
};

/**
	@brief Where a rule comes from, and what it does not cover
*/
struct NormRef
{
	std::string id;          ///< stable rule code, e.g. "REBT-CDT-002"
	std::string source;      ///< e.g. "ITC-BT-19"
	std::string section;     ///< e.g. "2.2.2"
	std::string version;     ///< text and date the rule was written from
	std::string limitations; ///< what the rule does not check
};

/**
	@brief The data of one circuit stretch. Every field may be unknown.
*/
struct CircuitInput
{
	std::optional<System> system;
	std::optional<double> voltage;        ///< V, see System
	std::optional<double> power;          ///< W, useful power of the load
	std::optional<double> powerFactor;    ///< cos phi, in (0, 1]
	std::optional<double> efficiency;     ///< in (0, 1], motors and the like
	std::optional<double> designCurrent;  ///< A, when known instead of power
	std::optional<double> length;         ///< m, one way
	std::optional<double> section;        ///< mm2, of one phase conductor
	std::optional<Material> material;
		/// degC, to work out the resistivity
	std::optional<double> conductorTemperature;
		/// m/(ohm.mm2), when given material and temperature are not used
	std::optional<double> conductivity;
	std::optional<double> reactance;      ///< ohm/km, per conductor
};

enum class Status {
	Ok,             ///< computed, value present
	NotComputable,  ///< data missing, no value
	InvalidInput    ///< data out of range, no value
};

enum class Issue {
	MissingSystem,
	MissingVoltage,
	MissingPower,
	MissingPowerFactor,
	MissingLength,
	MissingSection,
	MissingMaterial,
	MissingTemperature,
	NotPositive,           ///< field must be greater than zero
	NotFinite,             ///< NaN or infinite
	OutOfRange,            ///< e.g. power factor outside (0, 1]
	Negative,              ///< field must not be negative
	MissingInsulation,
	MissingMethod,         ///< installation method
	MissingLoadedConductors,
	MissingAmbient,        ///< ambient temperature, for the temperature factor
	MissingGroupingFactor,
	MissingJustification,  ///< a correction factor given without its reason
	NoTable,               ///< no ampacity table for these conditions
	SectionNotInTable,     ///< the section is not a row of the table
	MissingRatedCurrent,   ///< IN of the protection
	MissingProtectionStandard,
	MissingConventionalCurrent,  ///< I2, when no standard gives it
	NoOverloadProtection,  ///< the device does not protect against overload
	MissingAdmissibleCurrent,    ///< IZ could not be worked out
	FeederLoop,            ///< boards feeding each other in a circle
	FeederUseUnknown,      ///< a feeder of the chain has no use
	FeederNotComputable,   ///< the drop of a feeder of the chain is unknown
		// Short circuit
	NoShortCircuitProtection, ///< the device does not break short circuits
	MissingSource,         ///< how the supply is known: no method chosen
	MissingSourceCurrent,  ///< Icc at the origin, maximum or minimum
	NoSourceImpedance,     ///< 0.8 U0 / R with R = 0: no line from the CGP
	MissingBreakingCapacity,
	MissingTripCurrent,    ///< current of instantaneous trip, no curve
	MissingLetThrough,     ///< I2t of the manufacturer is needed
	MissingNeutralSection, ///< outside interior installations
	NoKFactor,             ///< no k for this conductor and insulation
		// Dwellings
	MissingPoints,         ///< points of use of a dwelling circuit
	MissingConduit,        ///< outer diameter of the conduit
	MissingDwellingData,   ///< area or a planned system of the dwelling
	MissingGeneralSwitch,  ///< rated current of the IGA
	MissingResidualDevice, ///< the residual current device of a circuit
	MissingSensitivity,    ///< of a residual current device
	MissingIndividualLine, ///< IZ of the DI that feeds the dwelling
	MissingRoomType,       ///< a room of the dwelling with no type
	MissingRoomSize,       ///< area or length of a room
	MissingRoomCount,      ///< a count of points of a room
	MissingIndividualDrop, ///< drop of the DI, to compensate with it
	LetThroughBelowFault,  ///< I2t given for a current under the maximum Icc
		// Indirect contacts
	MissingEarthing,       ///< earthing system of the installation (ITC-BT-08)
	MissingEarthResistance,///< RA of the masses
	MissingOperatingCurrent, ///< Ia of the device that disconnects
	LimitAboveNormal,      ///< a limit voltage over the 50 V of normal conditions
	MissingProtectiveSection, ///< PE of a line of the fault loop
	ResidualInTNC,         ///< ITC-BT-24 4.1.1: no residual current device in TN-C
	MissingFirstFault,     ///< IT: current of a first fault, Id
	MissingMassesArrangement,  ///< IT: masses earthed apart or interconnected
	MissingNeutralArrangement  ///< IT: neutral distributed or not
};

/**
	@brief A problem found in the input, and the field it is about
*/
struct Diagnostic
{
	Issue issue;
	std::string field;

	bool operator==(const Diagnostic &other) const
	{ return issue == other.issue && field == other.field; }
};

/**
	@brief Something the calculation took for granted
*/
enum class Assumption {
	EfficiencyNotApplied,      ///< no efficiency given: power taken as absorbed
	CurrentGiven,              ///< design current taken from the input
	ConductivityGiven,         ///< conductivity taken from the input
	ResistivityFromMaterial,   ///< resistivity from IEC constants and temperature
	ReactanceNeglected,        ///< no reactance given: X = 0
	LoadAtEnd,                 ///< whole load at the far end of the stretch
	BalancedLoad,              ///< three phase load is balanced
	LoadedConductorsFromSystem,///< 2 in single phase, 3 in three phase
	TemperatureFactorComputed, ///< from the formula, not read in a table
	ConventionalCurrentFromStandard, ///< I2 = factor x IN, from the device standard
	OriginBoard,               ///< a board fed by nothing starts the interior installation
		// Short circuit
	GuideSourceFactor,         ///< supply unknown: 0.8 U0 at the CGP (Guia-BT Anexo 3)
	SourceCurrentGiven,        ///< Icc at the origin given by the user
	ResistanceAt20,            ///< maximum Icc: conductors at 20 degC
	ResistanceAtFinalTemperature, ///< minimum Icc: at the end of the fault
	NeutralAsPhase,            ///< neutral section taken equal to the phase
	TripCurrentFromCurve,      ///< instantaneous trip from the B/C/D curve
	InstantaneousTripTime,     ///< the device clears in under 0.1 s, no I2t given
		// Dwellings
	DropAtRatedCurrent,        ///< ITC-BT-25 3: drop with IN of the breaker
	PowerFactorOne,            ///< no cos phi given: 1, as Guia-BT-25 table B
	OwnTransformerOrigin,      ///< origin at the output of an own HV transformer
	RoundedUp,                 ///< "uno cada 5 m": rounded up, not stated
	CompensatedWithIndividual, ///< drop compensated with the DI (ITC-BT-19 2.2.2)
		// Indirect contacts
	NormalLimitVoltage,        ///< U = 50 V, normal conditions (ITC-BT-24 4.1)
	ResidualOperatingCurrent,  ///< Ia = IDn of the residual current device
	EarthResistanceGiven,      ///< RA as given: electrode plus protective conductors
	GuideLoop,                 ///< TN, supply unknown: Zs = R / 0.8 (Guia-BT Anexo 3)
	LoopFromItOrigin,          ///< IT: Zs of the lines from the IT board, no source
	InstantaneousWithinTable   ///< Ia clears in under 0.1 s: within every time of the tables
};

struct CurrentResult
{
	Status status = Status::NotComputable;
	std::optional<double> current;  ///< A
	std::vector<Diagnostic> diagnostics;
	std::vector<Assumption> assumptions;
	std::vector<NormRef> rules;
};

struct VoltageDropResult
{
	Status status = Status::NotComputable;
	std::optional<double> current;  ///< A, the current used
	std::optional<double> drop;     ///< V
	std::optional<double> percent;  ///< % of the voltage of the input
	std::vector<Diagnostic> diagnostics;
	std::vector<Assumption> assumptions;
	std::vector<NormRef> rules;
};

/**
	@brief A maximum voltage drop and the rule it comes from
*/
struct VoltageDropLimit
{
	double percent = 0.0;
	bool strict = true;   ///< true: drop must be lower than percent
	NormRef rule;
};

enum class CheckOutcome {
	Passed,
	NotPassed,
	NotPerformed    ///< the value to check could not be computed
};

struct CheckResult
{
	CheckOutcome outcome = CheckOutcome::NotPerformed;
	std::optional<double> value;
	double limit = 0.0;
	NormRef rule;
};

} // namespace rebt

#endif // REBTTYPES_H
