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
#ifndef REBTDWELLING_H
#define REBTDWELLING_H

#include "rebttypes.h"

/**
	@brief Circuits of a dwelling (ITC-BT-25, as amended by RD 1053/2014)

	Table 1 gives for each circuit the breaker, the smallest copper
	section and the most points of use; notes (2) and (4) the most power
	of C8, C9 and C11. Only what the table states is checked: an empty
	value of the specification is a check not performed, never a guess.
*/
namespace rebt {

enum class DwellingCircuit {
	C1,        ///< lighting
	C2,        ///< sockets of general use and fridge
	C3,        ///< cooker and oven
	C4,        ///< washing machine, dishwasher and water heater
	C4Single,  ///< note (8): one independent circuit per appliance, 16 A
	C5,        ///< bathroom and kitchen worktop
	C6,        ///< additional C1, per 30 points of light
	C7,        ///< additional C2, per 20 sockets or over 160 m2
	C8,        ///< electric heating
	C9,        ///< air conditioning
	C10,       ///< dryer
	C11,       ///< home automation
	C12C3,     ///< additional circuit of type C3
	C12C4,     ///< additional circuit of type C4
	C12C5,     ///< additional C5, when its sockets exceed 6
	C13        ///< electric vehicle charging (ITC-BT-52)
};

/// What table 1 gives for one circuit; empty: the table does not say
struct DwellingSpec
{
	std::optional<double> breaker;     ///< A, taken as the highest rating
	std::optional<double> section;     ///< mm2, copper, smallest
	std::optional<int> points;         ///< most points of use or sockets
	std::optional<double> power;       ///< W, most power of the circuit
	std::optional<double> conduit;     ///< mm, smallest outer diameter
};

DwellingSpec dwellingSpec(DwellingCircuit circuit);

struct DwellingInput
{
	std::optional<DwellingCircuit> circuit;
	std::optional<double> ratedCurrent;   ///< IN of its breaker, A
	std::optional<double> section;        ///< mm2
	std::optional<Material> material;
	std::optional<int> points;            ///< points of use or sockets
	std::optional<double> power;          ///< W, of the circuit
	std::optional<double> conduit;        ///< mm, outer diameter
		/// % from the origin, worked out with IN (ITC-BT-25 3)
	std::optional<double> dropAtRated;
		/// Compensated with the DI (ITC-BT-19 2.2.2): dropAtRated then
		/// includes the DI, and must be lower than 3 % + its limit
	std::optional<double> compensatedLimit;
};

struct DwellingResult
{
	Status status = Status::NotComputable;
	DwellingSpec spec;
	CheckOutcome breaker = CheckOutcome::NotPerformed;   ///< IN <= table
	CheckOutcome copper = CheckOutcome::NotPerformed;    ///< copper conductors
	CheckOutcome section = CheckOutcome::NotPerformed;   ///< S >= table
	CheckOutcome points = CheckOutcome::NotPerformed;    ///< n <= table
	CheckOutcome power = CheckOutcome::NotPerformed;     ///< P <= table
	CheckOutcome conduit = CheckOutcome::NotPerformed;   ///< D >= table
	CheckOutcome drop = CheckOutcome::NotPerformed;      ///< < 3 % with IN
	CheckOutcome outcome = CheckOutcome::NotPerformed;
	std::vector<Diagnostic> diagnostics;
	std::vector<NormRef> rules;
};

/**
	@brief The checks of table 1 that apply to the circuit, and the
	voltage drop of section 3. A check the table has no value for is not
	counted; one whose data are missing makes the whole not performed.
*/
DwellingResult checkDwellingCircuit(const DwellingInput &input);

struct MinimumCircuitsResult
{
	CheckOutcome outcome = CheckOutcome::NotPerformed;
	std::vector<DwellingCircuit> missing;   ///< of C1 to C5
	NormRef rule;
};

/**
	@brief Basic electrification: at least C1 to C5 (ITC-BT-25 2.3.1).
	C4 is present as C4 or as independent circuits per appliance.
	@param circuits the dwelling circuits of one board; none: not performed
*/
MinimumCircuitsResult checkMinimumCircuits(const std::vector<DwellingCircuit> &circuits);

enum class Electrification { Basic, Elevated };

/// Rooms of table 2 of ITC-BT-25
enum class RoomType {
	Access,             ///< acceso
	Hall,               ///< vestibulo
	LivingRoom,         ///< sala de estar o salon
	Bedroom,
	Bathroom,
	Corridor,           ///< pasillo o distribuidor
	Kitchen,
	TerraceOrDressing,  ///< terraza, vestidor
	GarageOrOther       ///< garaje unifamiliar y otros
};

/// What table 2 counts in a room
enum class RoomItem {
	Bell,             ///< pulsador de timbre, C1
	LightPoints,      ///< C1
	Switches,         ///< interruptor or conmutador 10 A, C1
	Sockets,          ///< base 16 A 2p+T, C2
	Cooker,           ///< base 25 A 2p+T, C3
	ApplianceSockets, ///< C4: washing machine, dishwasher, water heater
	WetSockets,       ///< C5: bathroom, above the kitchen worktop
	Heating,          ///< toma de calefaccion, C8
	AirConditioning,  ///< C9
	Dryer,            ///< C10
	VehicleCharging   ///< C13
};

/// One room: its type, its size and what it has. All may be unknown.
struct Room
{
	std::string name;
	std::optional<RoomType> type;
	std::optional<double> area;       ///< m2
	std::optional<double> length;     ///< m, corridors
	std::optional<int> accesses;      ///< corridors: one switch at each
	std::vector<std::pair<RoomItem, int>> counts;   ///< those given
};

struct RoomRequirement
{
	RoomItem item;
	std::optional<int> required;   ///< empty: depends on an unknown datum
	std::optional<int> present;
	CheckOutcome outcome = CheckOutcome::NotPerformed;
};

struct RoomResult
{
	std::string name;
	std::vector<RoomRequirement> items;
	CheckOutcome outcome = CheckOutcome::NotPerformed;
	std::vector<Diagnostic> diagnostics;
	std::vector<Assumption> assumptions;
};

/// What is planned, which decides the C8, C9, C10, C13 points
struct RoomPlanned
{
	std::optional<bool> heating;
	std::optional<bool> airConditioning;
	std::optional<bool> dryer;
	std::optional<bool> vehicleCharging;
};

/**
	@brief The minimum points of use of table 2 of ITC-BT-25 in one room.
	Points of circuits not planned are not required.
*/
RoomResult checkRoom(const Room &room, const RoomPlanned &planned);

/// Stable name of @p item, as in a file and in a diagnostic field
const char *roomItemField(RoomItem item);

/// Why a dwelling has elevated electrification (ITC-BT-25 2.3.2)
enum class ElevatedReason {
	Area,              ///< useful area over 160 m2
	Heating,           ///< electric heating planned
	AirConditioning,
	Automation,        ///< automation, energy management, security
	VehicleCharging,   ///< EV charging in a single family dwelling
	ExtraCircuit       ///< a C6, C7, C8, C9, C11 or C12: more than the basic ones
};

/// A residual current device and the dwelling circuits it protects
struct ResidualDevice
{
	std::string name;
	std::optional<double> sensitivity;   ///< mA
	std::optional<double> rated;         ///< A
	bool combined = false;   ///< RCBO of one circuit: its rating is the circuit's
	std::vector<DwellingCircuit> circuits;
};

/**
	@brief One dwelling, from the board where its interior installation
	starts. Every planned system may be unknown.
*/
struct DwellingData
{
	std::optional<double> area;            ///< m2, useful
	std::optional<bool> heating;           ///< electric heating planned
	std::optional<bool> airConditioning;
	std::optional<bool> automation;
	std::optional<bool> vehicleCharging;   ///< EV parking places planned
	std::optional<bool> singleFamily;      ///< vivienda unifamiliar
	std::optional<bool> dryer;             ///< an independent dryer planned
	std::vector<Room> rooms;
	std::optional<double> generalSwitch;   ///< A, IGA
	std::optional<double> individualLine;  ///< A, IZ of the DI feeding it
	std::vector<DwellingCircuit> circuits;
	std::vector<ResidualDevice> residualDevices;
	int unprotectedCircuits = 0;           ///< dwelling circuits with no device given
};

struct DwellingCheck
{
	std::optional<Electrification> grade;
	std::vector<ElevatedReason> reasons;
	CheckOutcome minimumCircuits = CheckOutcome::NotPerformed;   ///< C1 to C5
	std::vector<DwellingCircuit> missing;    ///< of C1 to C5 and the planned ones
	CheckOutcome plannedCircuits = CheckOutcome::NotPerformed;   ///< C7, C8, C9, C10, C11, C13
	std::vector<RoomResult> rooms;
	CheckOutcome roomPoints = CheckOutcome::NotPerformed;   ///< table 2, every room
	std::optional<double> minimumSwitch;     ///< A: 25, or 40 when elevated
	CheckOutcome generalSwitch = CheckOutcome::NotPerformed;
	CheckOutcome individualLine = CheckOutcome::NotPerformed;    ///< IZ DI >= IGA
	CheckOutcome residualCount = CheckOutcome::NotPerformed;     ///< one per 5
	int residualNeeded = 0;
	CheckOutcome residualSensitivity = CheckOutcome::NotPerformed;   ///< <= 30 mA
	CheckOutcome residualRating = CheckOutcome::NotPerformed;    ///< IN >= IGA
	bool vehicleCircuit = false;   ///< the dwelling has a C13
	CheckOutcome vehicleExclusive = CheckOutcome::NotPerformed;  ///< C13 alone
	CheckOutcome outcome = CheckOutcome::NotPerformed;
	std::vector<Diagnostic> diagnostics;
	std::vector<NormRef> rules;
};

/**
	@brief Everything of ITC-BT-25 that is about the dwelling as a whole.
	A dwelling with no dwelling circuit is not checked.
*/
DwellingCheck checkDwelling(const DwellingData &dwelling);

namespace rules {
/// ITC-BT-25 3, table 1
NormRef dwellingCircuitTable();
/// ITC-BT-25 3: drop < 3 % with IN, to the farthest point
NormRef dwellingDropAtRated();
/// ITC-BT-25 2.3.1: C1 to C5 at least
NormRef minimumCircuits();
/// ITC-BT-25 2.3.2: elevated electrification and its circuits
NormRef elevatedElectrification();
/// ITC-BT-25 2.1, 1; ITC-BT-10: the general switch, the DI
NormRef generalSwitch();
/// ITC-BT-25 2.1, 2.3: residual current devices
NormRef residualDevices();
/// ITC-BT-25 4, table 2: points of use per room
NormRef roomPoints();
/// ITC-BT-19 2.2.2: drop compensated between interior and DI
NormRef compensatedDrop();
}

} // namespace rebt

#endif // REBTDWELLING_H
