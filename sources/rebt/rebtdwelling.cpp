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
#include "rebtdwelling.h"

#include <algorithm>
#include <cmath>

namespace rebt {

namespace {

const double TOLERANCE = 1e-9;
const char *const REBT_2002 = "RD 842/2002, BOE 224 of 18/09/2002";
const char *const ITC_BT_25 =
	"RD 842/2002, BOE 224 of 18/09/2002; amended by RD 1053/2014 (C13)";
/// ITC-BT-25 3: "como maximo el 3 %"
const double DWELLING_DROP = 3.0;

void add(std::vector<Diagnostic> &list, Issue issue, const char *field)
{
	const Diagnostic d{issue, field};
	if (std::find(list.begin(), list.end(), d) == list.end())
		list.push_back(d);
}

bool valid(const std::optional<double> &value)
{
	return value && std::isfinite(*value) && *value > 0.0;
}

DwellingSpec spec(std::optional<double> breaker, std::optional<double> section,
				  std::optional<int> points, std::optional<double> conduit,
				  std::optional<double> power = std::nullopt)
{
	return {breaker, section, points, power, conduit};
}

void add(std::vector<Diagnostic> &list, Issue issue, const std::string &field)
{
	const Diagnostic d{issue, field};
	if (std::find(list.begin(), list.end(), d) == list.end())
		list.push_back(d);
}

CheckOutcome combine(const std::vector<CheckOutcome> &all)
{
	if (std::any_of(all.begin(), all.end(),
					[](CheckOutcome o) { return o == CheckOutcome::NotPassed; }))
		return CheckOutcome::NotPassed;
	if (std::all_of(all.begin(), all.end(),
					[](CheckOutcome o) { return o == CheckOutcome::Passed; }))
		return CheckOutcome::Passed;
	return CheckOutcome::NotPerformed;
}

} // namespace

DwellingSpec dwellingSpec(DwellingCircuit circuit)
{
	switch (circuit) {
		case DwellingCircuit::C1:
		case DwellingCircuit::C6:
			return spec(10.0, 1.5, 30, 16.0);
		case DwellingCircuit::C2:
		case DwellingCircuit::C7:
			return spec(16.0, 2.5, 20, 20.0);
		case DwellingCircuit::C3:
		case DwellingCircuit::C12C3:
			return spec(25.0, 6.0, 2, 25.0);
		case DwellingCircuit::C4:
		case DwellingCircuit::C12C4:
			return spec(20.0, 4.0, 3, 20.0);
		case DwellingCircuit::C4Single:
				// note (8): 16 A per appliance; the section is not stated
			return spec(16.0, std::nullopt, 1, std::nullopt);
		case DwellingCircuit::C5:
		case DwellingCircuit::C12C5:
			return spec(16.0, 2.5, 6, 20.0);
		case DwellingCircuit::C8:
		case DwellingCircuit::C9:
				// note (2): 5750 W per circuit
			return spec(25.0, 6.0, std::nullopt, 25.0, 5750.0);
		case DwellingCircuit::C10:
			return spec(16.0, 2.5, 1, 20.0);
		case DwellingCircuit::C11:
				// note (4): 2300 W
			return spec(10.0, 1.5, std::nullopt, 16.0, 2300.0);
		case DwellingCircuit::C13:
				// note (10): power, sockets and breaker in ITC-BT-52
			return spec(std::nullopt, 2.5, 3, 20.0);
	}
	return DwellingSpec();
}

DwellingResult checkDwellingCircuit(const DwellingInput &input)
{
	DwellingResult result;
	result.rules.push_back(rules::dwellingCircuitTable());
	if (!input.circuit)
		return result;   // not a dwelling circuit: nothing to check
	result.spec = dwellingSpec(*input.circuit);
	const DwellingSpec &s = result.spec;
	bool invalid = false;
	auto read = [&](const std::optional<double> &value, const char *field,
					Issue missing) -> bool {
		if (!value) {
			add(result.diagnostics, missing, field);
			return false;
		}
		if (!valid(value)) {
			add(result.diagnostics, Issue::NotPositive, field);
			invalid = true;
			return false;
		}
		return true;
	};

	if (s.breaker && read(input.ratedCurrent, "ratedCurrent", Issue::MissingRatedCurrent))
		result.breaker = *input.ratedCurrent <= *s.breaker + TOLERANCE
						 ? CheckOutcome::Passed : CheckOutcome::NotPassed;

		// "Los conductores seran de cobre": whatever the section
	if (!input.material)
		add(result.diagnostics, Issue::MissingMaterial, "material");
	else
		result.copper = *input.material == Material::Copper
						? CheckOutcome::Passed : CheckOutcome::NotPassed;

	if (s.section && read(input.section, "section", Issue::MissingSection))
		result.section = *input.section + TOLERANCE >= *s.section
						 ? CheckOutcome::Passed : CheckOutcome::NotPassed;

	if (s.points) {
		if (!input.points)
			add(result.diagnostics, Issue::MissingPoints, "points");
		else if (*input.points <= 0) {
			add(result.diagnostics, Issue::NotPositive, "points");
			invalid = true;
		} else
			result.points = *input.points <= *s.points
							? CheckOutcome::Passed : CheckOutcome::NotPassed;
	}

	if (s.power && read(input.power, "power", Issue::MissingPower))
		result.power = *input.power <= *s.power + TOLERANCE
					   ? CheckOutcome::Passed : CheckOutcome::NotPassed;

		// Note (3): outer diameter, ITC-BT-19
	if (s.conduit && read(input.conduit, "conduit", Issue::MissingConduit))
		result.conduit = *input.conduit + TOLERANCE >= *s.conduit
						 ? CheckOutcome::Passed : CheckOutcome::NotPassed;

	result.rules.push_back(rules::dwellingDropAtRated());
	if (input.dropAtRated) {
		if (!std::isfinite(*input.dropAtRated) || *input.dropAtRated < 0.0) {
			add(result.diagnostics, Issue::Negative, "dropAtRated");
			invalid = true;
		} else {
			if (input.compensatedLimit)
					// ITC-BT-19 2.2.2: "inferior a la suma de los valores limites"
				result.drop = *input.dropAtRated + TOLERANCE < *input.compensatedLimit
							  ? CheckOutcome::Passed : CheckOutcome::NotPassed;
			else
				result.drop = *input.dropAtRated <= DWELLING_DROP + TOLERANCE
							  ? CheckOutcome::Passed : CheckOutcome::NotPassed;
		}
	}

	if (invalid) {
		result.status = Status::InvalidInput;
		result.breaker = result.copper = result.section = result.points
			= result.power = result.conduit = result.drop = CheckOutcome::NotPerformed;
		return result;
	}

		// Only the checks the table has a value for count
	std::vector<CheckOutcome> all{result.copper, result.drop};
	if (s.breaker) all.push_back(result.breaker);
	if (s.section) all.push_back(result.section);
	if (s.points) all.push_back(result.points);
	if (s.power) all.push_back(result.power);
	if (s.conduit) all.push_back(result.conduit);
	if (std::any_of(all.begin(), all.end(),
					[](CheckOutcome o) { return o == CheckOutcome::NotPassed; }))
		result.outcome = CheckOutcome::NotPassed;
	else if (std::all_of(all.begin(), all.end(),
						 [](CheckOutcome o) { return o == CheckOutcome::Passed; }))
		result.outcome = CheckOutcome::Passed;
	result.status = result.diagnostics.empty() && input.dropAtRated
					? Status::Ok : Status::NotComputable;
	return result;
}

MinimumCircuitsResult checkMinimumCircuits(const std::vector<DwellingCircuit> &circuits)
{
	MinimumCircuitsResult result;
	result.rule = rules::minimumCircuits();
	if (circuits.empty())
		return result;
	auto has = [&](std::initializer_list<DwellingCircuit> any) {
		return std::any_of(circuits.begin(), circuits.end(), [&](DwellingCircuit c) {
			return std::find(any.begin(), any.end(), c) != any.end();
		});
	};
	if (!has({DwellingCircuit::C1})) result.missing.push_back(DwellingCircuit::C1);
	if (!has({DwellingCircuit::C2})) result.missing.push_back(DwellingCircuit::C2);
	if (!has({DwellingCircuit::C3})) result.missing.push_back(DwellingCircuit::C3);
	if (!has({DwellingCircuit::C4, DwellingCircuit::C4Single}))
		result.missing.push_back(DwellingCircuit::C4);
	if (!has({DwellingCircuit::C5})) result.missing.push_back(DwellingCircuit::C5);
	result.outcome = result.missing.empty() ? CheckOutcome::Passed : CheckOutcome::NotPassed;
	return result;
}

const char *roomItemField(RoomItem item)
{
	switch (item) {
		case RoomItem::Bell: return "bell";
		case RoomItem::LightPoints: return "lightPoints";
		case RoomItem::Switches: return "switches";
		case RoomItem::Sockets: return "sockets";
		case RoomItem::Cooker: return "cooker";
		case RoomItem::ApplianceSockets: return "applianceSockets";
		case RoomItem::WetSockets: return "wetSockets";
		case RoomItem::Heating: return "heating";
		case RoomItem::AirConditioning: return "airConditioning";
		case RoomItem::Dryer: return "dryer";
		case RoomItem::VehicleCharging: return "vehicleCharging";
	}
	return "";
}

namespace {
const char *itemField(RoomItem item) { return roomItemField(item); }
} // namespace

RoomResult checkRoom(const Room &room, const RoomPlanned &planned)
{
	using I = RoomItem;
	RoomResult r;
	r.name = room.name;
	const std::string base = "room:" + room.name + "/";
	if (!room.type) {
		add(r.diagnostics, Issue::MissingRoomType, base + "type");
		return r;
	}
	bool invalid = false;
	auto count = [&](I item) -> std::optional<int> {
		for (const auto &c : room.counts) {
			if (c.first != item)
				continue;
			if (c.second < 0) {
				add(r.diagnostics, Issue::Negative, base + itemField(item));
				invalid = true;
				return std::nullopt;
			}
			return c.second;
		}
		return std::nullopt;
	};
	auto need = [&](I item, std::optional<int> required) {
		RoomRequirement q;
		q.item = item;
		q.required = required;
		q.present = count(item);
		if (!q.present)
			add(r.diagnostics, Issue::MissingRoomCount, base + itemField(item));
		if (required && q.present)
			q.outcome = *q.present >= *required ? CheckOutcome::Passed
												: CheckOutcome::NotPassed;
		r.items.push_back(q);
	};
	auto size = [&](const std::optional<double> &value, const char *field)
		-> std::optional<double> {
		if (!value) {
			add(r.diagnostics, Issue::MissingRoomSize, base + field);
			return std::nullopt;
		}
		if (!std::isfinite(*value) || *value <= 0.0) {
			add(r.diagnostics, Issue::NotPositive, base + field);
			invalid = true;
			return std::nullopt;
		}
		return value;
	};
		// "hasta 10 m2 (dos si S > 10 m2)"
	auto upTo10 = [&]() -> std::optional<int> {
		const auto a = size(room.area, "area");
		if (!a) return std::nullopt;
		return *a > 10.0 + TOLERANCE ? 2 : 1;
	};
		// "una por cada 6 m2, redondeado al entero superior", 3 at least
	auto per6 = [&]() -> std::optional<int> {
		const auto a = size(room.area, "area");
		if (!a) return std::nullopt;
		return std::max(3, static_cast<int>(std::ceil(*a / 6.0 - TOLERANCE)));
	};
		// "uno por cada punto de luz"
	auto perLight = [&]() -> std::optional<int> {
		const auto lights = count(I::LightPoints);
		if (!lights) return std::nullopt;
		return std::max(1, *lights);
	};
	auto ifPlanned = [&](const std::optional<bool> &flag, const char *field, I item,
						 std::optional<int> required) {
		if (!flag) {
			add(r.diagnostics, Issue::MissingDwellingData, field);
			RoomRequirement q;
			q.item = item;
			q.present = count(item);
			r.items.push_back(q);
		} else if (*flag) {
			need(item, required);
		}
	};

	switch (*room.type) {
		case RoomType::Access:
			need(I::Bell, 1);
			break;
		case RoomType::Hall:
			need(I::LightPoints, 1);
			need(I::Switches, 1);
			need(I::Sockets, 1);
			break;
		case RoomType::LivingRoom: {
			const auto ten = upTo10();
			need(I::LightPoints, ten);
			need(I::Switches, perLight());
			need(I::Sockets, per6());
			ifPlanned(planned.heating, "heating", I::Heating, ten);
			ifPlanned(planned.airConditioning, "airConditioning", I::AirConditioning, ten);
			break;
		}
		case RoomType::Bedroom:
			need(I::LightPoints, upTo10());
			need(I::Switches, perLight());
			need(I::Sockets, per6());
			ifPlanned(planned.heating, "heating", I::Heating, 1);
			ifPlanned(planned.airConditioning, "airConditioning", I::AirConditioning, 1);
			break;
		case RoomType::Bathroom:
			need(I::LightPoints, 1);
			need(I::Switches, 1);
			need(I::WetSockets, 1);
			ifPlanned(planned.heating, "heating", I::Heating, 1);
			break;
		case RoomType::Corridor: {
				// "uno cada 5 m de longitud": rounded up, the text does not say
			std::optional<int> lights;
			if (const auto l = size(room.length, "length")) {
				lights = std::max(1, static_cast<int>(std::ceil(*l / 5.0 - TOLERANCE)));
				r.assumptions.push_back(Assumption::RoundedUp);
			}
			need(I::LightPoints, lights);
				// "uno en cada acceso"
			std::optional<int> switches;
			if (!room.accesses)
				add(r.diagnostics, Issue::MissingRoomSize, base + "accesses");
			else if (*room.accesses <= 0) {
				add(r.diagnostics, Issue::NotPositive, base + "accesses");
				invalid = true;
			} else
				switches = *room.accesses;
			need(I::Switches, switches);
				// "hasta 5 m (dos si L > 5 m)"
			std::optional<int> sockets;
			if (room.length && std::isfinite(*room.length) && *room.length > 0.0)
				sockets = *room.length > 5.0 + TOLERANCE ? 2 : 1;
			need(I::Sockets, sockets);
			ifPlanned(planned.heating, "heating", I::Heating, 1);
			break;
		}
		case RoomType::Kitchen:
			need(I::LightPoints, upTo10());
			need(I::Switches, perLight());
			need(I::Sockets, 2);            // extractor and fridge
			need(I::Cooker, 1);
			need(I::ApplianceSockets, 3);   // washing machine, dishwasher, water heater
			need(I::WetSockets, 3);         // above the worktop (note 2)
			ifPlanned(planned.heating, "heating", I::Heating, 1);
			ifPlanned(planned.dryer, "dryer", I::Dryer, 1);
			break;
		case RoomType::TerraceOrDressing:
			need(I::LightPoints, upTo10());
			need(I::Switches, perLight());
			break;
		case RoomType::GarageOrOther: {
			const auto ten = upTo10();
			need(I::LightPoints, ten);
			need(I::Switches, perLight());
			need(I::Sockets, ten);
			ifPlanned(planned.vehicleCharging, "vehicleCharging", I::VehicleCharging, 1);
			break;
		}
	}

	if (invalid) {
		for (RoomRequirement &q : r.items)
			q.outcome = CheckOutcome::NotPerformed;
		return r;
	}
	std::vector<CheckOutcome> all;
	for (const RoomRequirement &q : r.items)
		all.push_back(q.outcome);
	r.outcome = combine(all);
	return r;
}

DwellingCheck checkDwelling(const DwellingData &d)
{
	using D = DwellingCircuit;
	DwellingCheck result;
	if (d.circuits.empty())
		return result;   // not a dwelling: nothing to check
	result.rules = {rules::minimumCircuits(), rules::elevatedElectrification(),
					rules::generalSwitch(), rules::residualDevices()};
	auto has = [&](D c) {
		return std::find(d.circuits.begin(), d.circuits.end(), c) != d.circuits.end();
	};

		// C1 to C5
	const MinimumCircuitsResult minimum = checkMinimumCircuits(d.circuits);
	result.minimumCircuits = minimum.outcome;
	result.missing = minimum.missing;

		// Grade: elevated as soon as one condition of 2.3.2 is known to hold
	const bool area_over = d.area && *d.area > 160.0;
	if (area_over) result.reasons.push_back(ElevatedReason::Area);
	if (d.heating.value_or(false)) result.reasons.push_back(ElevatedReason::Heating);
	if (d.airConditioning.value_or(false))
		result.reasons.push_back(ElevatedReason::AirConditioning);
	if (d.automation.value_or(false)) result.reasons.push_back(ElevatedReason::Automation);
	const bool charging = d.vehicleCharging.value_or(false);
	if (charging && d.singleFamily.value_or(false))
		result.reasons.push_back(ElevatedReason::VehicleCharging);
	for (D c : {D::C6, D::C7, D::C8, D::C9, D::C11, D::C12C3, D::C12C4, D::C12C5}) {
		if (has(c)) {
			result.reasons.push_back(ElevatedReason::ExtraCircuit);
			break;
		}
	}
	if (d.area && (!std::isfinite(*d.area) || *d.area <= 0.0))
		add(result.diagnostics, Issue::NotPositive, "area");
	if (!result.reasons.empty()) {
		result.grade = Electrification::Elevated;
	} else {
		bool known = true;
		auto need = [&](bool is_known, const char *field) {
			if (!is_known) {
				add(result.diagnostics, Issue::MissingDwellingData, field);
				known = false;
			}
		};
		need(d.area.has_value(), "area");
		need(d.heating.has_value(), "heating");
		need(d.airConditioning.has_value(), "airConditioning");
		need(d.automation.has_value(), "automation");
		need(d.vehicleCharging.has_value(), "vehicleCharging");
		if (charging)
			need(d.singleFamily.has_value(), "singleFamily");
		if (known)
			result.grade = Electrification::Basic;
	}

		// Circuits required by what is planned
	{
		bool known = true;
		std::vector<D> required;
		auto require = [&](const std::optional<bool> &planned, D circuit) {
			if (!planned) known = false;
			else if (*planned) required.push_back(circuit);
		};
		if (!d.area) known = false;
		else if (area_over) required.push_back(D::C7);
		require(d.heating, D::C8);
		require(d.airConditioning, D::C9);
		require(d.automation, D::C11);
		require(d.dryer, D::C10);
			// shared car parks: the common areas schemes of ITC-BT-52
		if (charging) {
			if (!d.singleFamily) known = false;
			else if (*d.singleFamily) required.push_back(D::C13);
		} else if (!d.vehicleCharging) {
			known = false;
		}
		bool missing = false;
		for (D c : required) {
			if (!has(c)) {
				result.missing.push_back(c);
				missing = true;
			}
		}
		result.plannedCircuits = missing ? CheckOutcome::NotPassed
								 : known ? CheckOutcome::Passed : CheckOutcome::NotPerformed;
	}

		// General switch: at least 25 A (2.1); elevated, not under 9200 W
		// at 230 V (ITC-BT-10 2.2), that is 40 A
	result.minimumSwitch = result.grade == Electrification::Elevated ? 40.0 : 25.0;
	const bool switch_ok = d.generalSwitch && std::isfinite(*d.generalSwitch)
						   && *d.generalSwitch > 0.0;
	if (!d.generalSwitch)
		add(result.diagnostics, Issue::MissingGeneralSwitch, "generalSwitch");
	else if (!switch_ok)
		add(result.diagnostics, Issue::NotPositive, "generalSwitch");
	if (switch_ok) {
		if (*d.generalSwitch + TOLERANCE < 25.0)
			result.generalSwitch = CheckOutcome::NotPassed;
		else if (result.grade == Electrification::Elevated)
			result.generalSwitch = *d.generalSwitch + TOLERANCE >= 40.0
								   ? CheckOutcome::Passed : CheckOutcome::NotPassed;
		else if (result.grade == Electrification::Basic || *d.generalSwitch + TOLERANCE >= 40.0)
			result.generalSwitch = CheckOutcome::Passed;
	}

		// Capacity of the DI: at least the IGA (1)
	if (!d.individualLine)
		add(result.diagnostics, Issue::MissingIndividualLine, "individualLine");
	else if (switch_ok)
		result.individualLine = *d.individualLine + TOLERANCE >= *d.generalSwitch
								? CheckOutcome::Passed : CheckOutcome::NotPassed;

		// Residual current devices
	const int n = static_cast<int>(d.circuits.size());
	result.residualNeeded = (n + 4) / 5;
	if (d.unprotectedCircuits > 0)
		add(result.diagnostics, Issue::MissingResidualDevice, "residualDevice");
	const int devices = static_cast<int>(d.residualDevices.size());
	if (devices >= result.residualNeeded)
		result.residualCount = CheckOutcome::Passed;
	else if (d.unprotectedCircuits == 0)
		result.residualCount = CheckOutcome::NotPassed;
	{
		std::vector<CheckOutcome> sens, rating;
		for (const ResidualDevice &r : d.residualDevices) {
			const std::string field = "residualDevice:" + r.name;
			if (!r.sensitivity) {
				add(result.diagnostics, Issue::MissingSensitivity, field);
				sens.push_back(CheckOutcome::NotPerformed);
			} else {
				sens.push_back(*r.sensitivity <= 30.0 + TOLERANCE
							   ? CheckOutcome::Passed : CheckOutcome::NotPassed);
			}
			if (r.combined)
				continue;
			if (!r.rated) {
				add(result.diagnostics, Issue::MissingRatedCurrent, field);
				rating.push_back(CheckOutcome::NotPerformed);
			} else if (!switch_ok) {
				rating.push_back(CheckOutcome::NotPerformed);
			} else {
				rating.push_back(*r.rated + TOLERANCE >= *d.generalSwitch
								 ? CheckOutcome::Passed : CheckOutcome::NotPassed);
			}
		}
		if (d.unprotectedCircuits > 0) {
			sens.push_back(CheckOutcome::NotPerformed);
			rating.push_back(CheckOutcome::NotPerformed);
		}
		result.residualSensitivity = combine(sens);
		result.residualRating = combine(rating);
	}

		// Points of use of each room (table 2)
	result.rules.push_back(rules::roomPoints());
	if (d.rooms.empty()) {
		add(result.diagnostics, Issue::MissingRoomType, "rooms");
	} else {
		const RoomPlanned planned{d.heating, d.airConditioning, d.dryer,
								  d.vehicleCharging};
		std::vector<CheckOutcome> rooms;
		for (const Room &room : d.rooms) {
			result.rooms.push_back(checkRoom(room, planned));
			rooms.push_back(result.rooms.back().outcome);
		}
		result.roomPoints = combine(rooms);
	}

		// C13: a residual current device of its own (ITC-BT-52)
	std::vector<CheckOutcome> all{result.roomPoints,
								  result.minimumCircuits, result.plannedCircuits,
								  result.generalSwitch, result.individualLine,
								  result.residualCount, result.residualSensitivity,
								  result.residualRating};
	result.vehicleCircuit = has(D::C13);
	if (result.vehicleCircuit) {
		const int c13 = static_cast<int>(std::count(d.circuits.begin(), d.circuits.end(),
												   D::C13));
		int covered = 0;
		bool shared = false;
		for (const ResidualDevice &r : d.residualDevices) {
			const int here = static_cast<int>(std::count(r.circuits.begin(),
														 r.circuits.end(), D::C13));
			covered += here;
			if (here > 0 && static_cast<int>(r.circuits.size()) > here)
				shared = true;
		}
		if (shared)
			result.vehicleExclusive = CheckOutcome::NotPassed;
		else if (covered == c13)
			result.vehicleExclusive = CheckOutcome::Passed;
		all.push_back(result.vehicleExclusive);
	}
	result.outcome = combine(all);
	return result;
}

namespace rules {

NormRef dwellingCircuitTable()
{
	return {
		"REBT-VIV-001",
		"ITC-BT-25",
		"3, table 1",
		ITC_BT_25,
		"The breaker of the table is taken as the highest rating: a lower "
		"one is not flagged (IB <= IN is checked apart). Sections are of "
		"copper, for 2 conductors and earth, PVC, in conduit embedded in a "
		"wall (note 5): other conditions may need more, see IB <= IZ. C4 "
		"per appliance (note 8): 16 A, section not stated. C13: breaker, "
		"power and sockets of ITC-BT-52, not checked. Conduit: outer "
		"diameter (note 3); C6, C7 and C12 take those of their type. Fs, Fu "
		"and the points of each room (table 2) not checked."
	};
}

NormRef dwellingDropAtRated()
{
	return {
		"REBT-VIV-002",
		"ITC-BT-25",
		"3",
		std::string(ITC_BT_25) + "; \"como maximo el 3 %\"",
		"With a current equal to IN of the breaker, the whole length to the "
		"farthest point, from the origin of the interior installation, "
		"through the interior feeders. cos phi of the load, or 1 as in "
		"Guia-BT-25 table B when not given. The compensation with the DI "
		"allowed by ITC-BT-19 2.2.2 is not applied."
	};
}

NormRef minimumCircuits()
{
	return {
		"REBT-VIV-003",
		"ITC-BT-25",
		"2.3.1",
		ITC_BT_25,
		"C1 to C5 at least, for every dwelling: the circuits of the boards "
		"whose interior installation starts at the same board. C4 split per "
		"appliance counts as C4; a C6 is not a C1, nor a C12 a C3."
	};
}

NormRef elevatedElectrification()
{
	return {
		"REBT-VIV-004",
		"ITC-BT-25",
		"2.3.2",
		ITC_BT_25,
		"Elevated when the useful area exceeds 160 m2, or electric heating, "
		"air conditioning or automation is planned, or EV charging in a "
		"single family dwelling, or a circuit beyond the basic ones exists "
		"(C6, C7, C8, C9, C11, C12). Required then: C7 over 160 m2, C8, C9, "
		"C10 for an independent dryer, C11, C13 (single family only: shared "
		"car parks follow ITC-BT-52) for what is planned. C6 and C12 for C5 "
		"follow from the points of table 1; the other C12 are not required: "
		"an extra C3 or C4 is the user's own choice. Grade unknown when a "
		"datum is; a dryer is not a condition of elevated electrification."
	};
}

NormRef roomPoints()
{
	return {
		"REBT-VIV-007",
		"ITC-BT-25",
		"4, table 2",
		ITC_BT_25 + std::string("; Guia-BT-25 for the switch types and the bell"),
		"Minimum points per room, by type. Sockets: one per 6 m2 rounded up, "
		"3 at least. Corridors: one light point per 5 m, rounded up (the "
		"text does not say), one switch at each access. Switches: one per "
		"light point; interruptor, conmutador and the like count alike. C8, "
		"C9, C10 and C13 points only when planned. Locations (note 2, the "
		"kitchen worktop) and multiple TV bases (note 1) are not checked; "
		"rooms of no listed type go as 'others'. The bell is not a point "
		"of use of C1."
	};
}

NormRef compensatedDrop()
{
	return {
		"REBT-CDT-007",
		"ITC-BT-19",
		"2.2.2",
		std::string(REBT_2002) + "; also ITC-BT-25 3",
		"When stated for the board fed by the DI: the drop of the interior "
		"installation and that of the DI added, lower than the sum of both "
		"limits. The DI is then not checked against its own limit alone."
	};
}

NormRef generalSwitch()
{
	return {
		"REBT-VIV-005",
		"ITC-BT-25; ITC-BT-10",
		"2.1, 1; ITC-BT-10 2.2",
		std::string(ITC_BT_25) + "; ITC-BT-10 2.2: elevated, not under 9200 W, "
								 "the capacity set by the IGA; 40 A at 230 V as "
								 "table A of Guia-BT-25",
		"IGA at least 25 A; with elevated electrification, at least 40 A. "
		"The capacity of the DI (its IZ) at least the IGA. The capacity of "
		"the interior installation as a whole and the ICP are not checked."
	};
}

NormRef residualDevices()
{
	return {
		"REBT-VIV-006",
		"ITC-BT-25",
		"2.1, 2.3.2",
		ITC_BT_25,
		"At least one residual current device per five circuits installed, "
		"every circuit at 30 mA at most, rated current at least that of the "
		"IGA (not asked of an RCBO of one circuit), and one of its own for "
		"C13 (ITC-BT-52). Counted per dwelling, as the text does: how the "
		"circuits are shared among the devices is not checked. Other "
		"measures against indirect contact (ITC-BT-24 4.1) not considered."
	};
}

} // namespace rules

} // namespace rebt
