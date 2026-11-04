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
#include "rebt/rebtdwelling.h"
#include "rebt/rebtrules.h"

#include <QtTest>

#include <algorithm>

using namespace rebt;

Q_DECLARE_METATYPE(rebt::DwellingCircuit)
Q_DECLARE_METATYPE(rebt::ElevatedReason)

namespace {

bool hasIssue(const std::vector<Diagnostic> &list, Issue issue, const std::string &field)
{
	return std::find(list.begin(), list.end(), Diagnostic{issue, field}) != list.end();
}

const double NONE = -1;

/// A circuit that meets table 1 for @p c, with the given values
DwellingInput meeting(DwellingCircuit c, double in, double s, int points, double power,
					  double conduit = 25)
{
	DwellingInput d;
	d.circuit = c;
	if (in > 0) d.ratedCurrent = in;
	d.section = s > 0 ? s : 2.5;
	d.material = Material::Copper;
	d.points = points > 0 ? points : 1;
	d.power = power > 0 ? power : 1000.0;
	d.dropAtRated = 2.0;
	d.conduit = conduit > 0 ? conduit : 25.0;
	return d;
}

} // namespace

/**
	Table 1 of ITC-BT-25 as published (BOE; RD 1053/2014 for C13),
	copied by hand into the rows below, not from the program.
*/
class tst_rebtdwelling : public QObject
{
		Q_OBJECT

		/// A complete basic dwelling, everything stated
		DwellingData basic()
		{
			using D = DwellingCircuit;
			DwellingData d;
			d.area = 90;
			d.heating = false;
			d.airConditioning = false;
			d.automation = false;
			d.vehicleCharging = false;
			d.dryer = false;
			Room hall;
			hall.name = "Vestíbulo";
			hall.type = RoomType::Hall;
			hall.counts = {{RoomItem::LightPoints, 1}, {RoomItem::Switches, 1},
						   {RoomItem::Sockets, 1}};
			d.rooms = {hall};
			d.generalSwitch = 25;
			d.individualLine = 37;
			d.circuits = {D::C1, D::C2, D::C3, D::C4, D::C5};
			ResidualDevice id1;
			id1.name = "ID1";
			id1.sensitivity = 30;
			id1.rated = 40;
			id1.circuits = d.circuits;
			d.residualDevices = {id1};
			return d;
		}

	private slots:
		/// Requirement of @p item in @p r, -1 when not required, -2 when unknown
		static int required(const RoomResult &r, RoomItem item)
		{
			for (const RoomRequirement &q : r.items)
				if (q.item == item)
					return q.required ? *q.required : -2;
			return -1;
		}

		static Room room(RoomType type, double area, double length = 0)
		{
			Room r;
			r.name = "R";
			r.type = type;
			if (area > 0) r.area = area;
			if (length > 0) r.length = length;
			return r;
		}

		void roomRequirements_data()
		{
			QTest::addColumn<int>("type");
			QTest::addColumn<double>("area");
			QTest::addColumn<double>("length");
			QTest::addColumn<int>("item");
			QTest::addColumn<int>("expected");
			using T = RoomType;
			using I = RoomItem;
			auto row = [](const char *name, T t, double a, double l, I i, int n) {
				QTest::newRow(name) << int(t) << a << l << int(i) << n;
			};
				// "hasta 10 m2 (dos si S > 10 m2)"
			row("salon 10 m2 light", T::LivingRoom, 10, 0, I::LightPoints, 1);
			row("salon 10.5 m2 light", T::LivingRoom, 10.5, 0, I::LightPoints, 2);
				// "una por cada 6 m2, redondeado al entero superior", 3 at least
			row("salon 12 m2 sockets", T::LivingRoom, 12, 0, I::Sockets, 3);
			row("salon 18 m2 sockets", T::LivingRoom, 18, 0, I::Sockets, 3);
			row("salon 18.1 m2 sockets", T::LivingRoom, 18.1, 0, I::Sockets, 4);
			row("salon 25 m2 sockets", T::LivingRoom, 25, 0, I::Sockets, 5);
			row("bedroom 12 m2 light", T::Bedroom, 12, 0, I::LightPoints, 2);
			row("bedroom 30 m2 sockets", T::Bedroom, 30, 0, I::Sockets, 5);
			row("bathroom C5", T::Bathroom, 4, 0, I::WetSockets, 1);
				// "uno cada 5 m"; "hasta 5 m (dos si L > 5 m)"
			row("corridor 5 m light", T::Corridor, 0, 5, I::LightPoints, 1);
			row("corridor 7 m light", T::Corridor, 0, 7, I::LightPoints, 2);
			row("corridor 12 m light", T::Corridor, 0, 12, I::LightPoints, 3);
			row("corridor 5 m sockets", T::Corridor, 0, 5, I::Sockets, 1);
			row("corridor 5.2 m sockets", T::Corridor, 0, 5.2, I::Sockets, 2);
			row("kitchen sockets", T::Kitchen, 9, 0, I::Sockets, 2);
			row("kitchen cooker", T::Kitchen, 9, 0, I::Cooker, 1);
			row("kitchen C4", T::Kitchen, 9, 0, I::ApplianceSockets, 3);
			row("kitchen worktop", T::Kitchen, 9, 0, I::WetSockets, 3);
			row("kitchen 12 m2 light", T::Kitchen, 12, 0, I::LightPoints, 2);
			row("terrace light", T::TerraceOrDressing, 15, 0, I::LightPoints, 2);
			row("terrace no sockets", T::TerraceOrDressing, 15, 0, I::Sockets, -1);
			row("garage 20 m2 sockets", T::GarageOrOther, 20, 0, I::Sockets, 2);
			row("access bell", T::Access, 0, 0, I::Bell, 1);
			row("hall socket", T::Hall, 0, 0, I::Sockets, 1);
				// not planned: not required
			row("bedroom heating not planned", T::Bedroom, 12, 0, I::Heating, -1);
			row("kitchen dryer not planned", T::Kitchen, 9, 0, I::Dryer, -1);
		}

		void roomRequirements()
		{
			QFETCH(int, type);
			QFETCH(double, area);
			QFETCH(double, length);
			QFETCH(int, item);
			QFETCH(int, expected);
			const RoomPlanned none{false, false, false, false};
			const RoomResult r = checkRoom(room(RoomType(type), area, length), none);
			QCOMPARE(required(r, RoomItem(item)), expected);
		}

		void plannedPoints()
		{
			const RoomPlanned all{true, true, true, true};
				// salon: heating and air "hasta 10 m2 (dos si S > 10 m2)"
			RoomResult r = checkRoom(room(RoomType::LivingRoom, 25), all);
			QCOMPARE(required(r, RoomItem::Heating), 2);
			QCOMPARE(required(r, RoomItem::AirConditioning), 2);
			r = checkRoom(room(RoomType::Bedroom, 25), all);
			QCOMPARE(required(r, RoomItem::Heating), 1);
			r = checkRoom(room(RoomType::Kitchen, 9), all);
			QCOMPARE(required(r, RoomItem::Dryer), 1);
			r = checkRoom(room(RoomType::GarageOrOther, 9), all);
			QCOMPARE(required(r, RoomItem::VehicleCharging), 1);
				// unknown: neither required nor dropped, and said
			r = checkRoom(room(RoomType::Bathroom, 4), RoomPlanned());
			QCOMPARE(required(r, RoomItem::Heating), -2);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingDwellingData, "heating"));
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
		}

		void roomCounts()
		{
			Room bedroom = room(RoomType::Bedroom, 12);
			bedroom.name = "Dormitorio 1";
				// 2 light points, 2 switches, 3 sockets
			bedroom.counts = {{RoomItem::LightPoints, 2}, {RoomItem::Switches, 2},
							  {RoomItem::Sockets, 3}};
			const RoomPlanned none{false, false, false, false};
			RoomResult r = checkRoom(bedroom, none);
			QCOMPARE(r.outcome, CheckOutcome::Passed);
				// one switch per light point actually there
			bedroom.counts = {{RoomItem::LightPoints, 3}, {RoomItem::Switches, 2},
							  {RoomItem::Sockets, 3}};
			r = checkRoom(bedroom, none);
			QCOMPARE(required(r, RoomItem::Switches), 3);
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
				// a count not given: not performed, said
			bedroom.counts = {{RoomItem::LightPoints, 2}, {RoomItem::Switches, 2}};
			r = checkRoom(bedroom, none);
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingRoomCount, "room:Dormitorio 1/sockets"));
				// one short: not passed even with another unknown
			bedroom.counts = {{RoomItem::LightPoints, 1}, {RoomItem::Switches, 2}};
			QCOMPARE(checkRoom(bedroom, none).outcome, CheckOutcome::NotPassed);
				// no area: the area rules are not known
			bedroom.area.reset();
			r = checkRoom(bedroom, none);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingRoomSize, "room:Dormitorio 1/area"));
			QCOMPARE(required(r, RoomItem::LightPoints), -2);
				// no type: nothing
			Room unknown;
			unknown.name = "X";
			r = checkRoom(unknown, none);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingRoomType, "room:X/type"));
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
				// negative counts and sizes are refused
			bedroom = room(RoomType::Bedroom, -3);
			QCOMPARE(checkRoom(bedroom, none).outcome, CheckOutcome::NotPerformed);
			bedroom = room(RoomType::Bedroom, 12);
			bedroom.counts = {{RoomItem::LightPoints, -1}, {RoomItem::Switches, 2},
							  {RoomItem::Sockets, 3}};
			r = checkRoom(bedroom, none);
			QVERIFY(hasIssue(r.diagnostics, Issue::Negative, "room:R/lightPoints"));
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
		}

		void corridorSwitchesAtEachAccess()
		{
			Room c = room(RoomType::Corridor, 0, 7);
			const RoomPlanned none{false, false, false, false};
			RoomResult r = checkRoom(c, none);
			QCOMPARE(required(r, RoomItem::Switches), -2);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingRoomSize, "room:R/accesses"));
			QVERIFY(std::find(r.assumptions.begin(), r.assumptions.end(),
							  Assumption::RoundedUp) != r.assumptions.end());
			c.accesses = 3;
			c.counts = {{RoomItem::LightPoints, 2}, {RoomItem::Switches, 3},
						{RoomItem::Sockets, 2}};
			r = checkRoom(c, none);
			QCOMPARE(required(r, RoomItem::Switches), 3);
			QCOMPARE(r.outcome, CheckOutcome::Passed);
		}

		void roomsInTheDwelling()
		{
			DwellingData d = basic();
			d.rooms.clear();
			DwellingCheck r = checkDwelling(d);
			QCOMPARE(r.roomPoints, CheckOutcome::NotPerformed);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingRoomType, "rooms"));
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
				// a room short of points fails the dwelling
			d = basic();
			d.rooms.front().counts = {{RoomItem::LightPoints, 1}, {RoomItem::Switches, 1},
									  {RoomItem::Sockets, 0}};
			r = checkDwelling(d);
			QCOMPARE(r.roomPoints, CheckOutcome::NotPassed);
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
			QCOMPARE(r.rooms.size(), std::size_t(1));
				// a dryer planned: C10 required, and its socket in the kitchen
			d = basic();
			d.dryer = true;
			r = checkDwelling(d);
			QCOMPARE(r.plannedCircuits, CheckOutcome::NotPassed);
			QVERIFY(std::find(r.missing.begin(), r.missing.end(), DwellingCircuit::C10)
					!= r.missing.end());
				// ...but it is not elevated electrification
			QCOMPARE(*r.grade, Electrification::Basic);
			d.dryer.reset();
			QCOMPARE(checkDwelling(d).plannedCircuits, CheckOutcome::NotPerformed);
		}

		void compensatedDrop()
		{
				// ITC-BT-19 2.2.2: "inferior a la suma": strict
			DwellingInput in = meeting(DwellingCircuit::C1, 10, 1.5, 10, 0, 16);
			in.dropAtRated = 4.4;
			in.compensatedLimit = 4.5;   // 3 % + 1.5 % of a DI of a single user
			QCOMPARE(checkDwellingCircuit(in).drop, CheckOutcome::Passed);
			in.dropAtRated = 4.5;
			QCOMPARE(checkDwellingCircuit(in).drop, CheckOutcome::NotPassed);
			QCOMPARE(rules::compensatedDrop().id, std::string("REBT-CDT-007"));
		}

		void basicDwelling()
		{
			const DwellingCheck r = checkDwelling(basic());
			QCOMPARE(*r.grade, Electrification::Basic);
			QVERIFY(r.reasons.empty());
			QCOMPARE(*r.minimumSwitch, 25.0);
			QCOMPARE(r.residualNeeded, 1);
			QCOMPARE(r.outcome, CheckOutcome::Passed);
			QVERIFY(r.diagnostics.empty());
			QCOMPARE(r.rules.size(), std::size_t(5));
			QCOMPARE(r.roomPoints, CheckOutcome::Passed);
			QVERIFY(!r.vehicleCircuit);
				// not a dwelling: nothing
			QCOMPARE(checkDwelling(DwellingData()).outcome, CheckOutcome::NotPerformed);
			QVERIFY(checkDwelling(DwellingData()).rules.empty());
		}

		void elevatedConditions_data()
		{
			QTest::addColumn<int>("which");
			QTest::addColumn<ElevatedReason>("reason");
			QTest::addColumn<DwellingCircuit>("required");
			QTest::newRow("area 160.1") << 0 << ElevatedReason::Area << DwellingCircuit::C7;
			QTest::newRow("heating") << 1 << ElevatedReason::Heating << DwellingCircuit::C8;
			QTest::newRow("air conditioning") << 2 << ElevatedReason::AirConditioning
											 << DwellingCircuit::C9;
			QTest::newRow("automation") << 3 << ElevatedReason::Automation
									   << DwellingCircuit::C11;
			QTest::newRow("EV single family") << 4 << ElevatedReason::VehicleCharging
											 << DwellingCircuit::C13;
		}

		void elevatedConditions()
		{
			QFETCH(int, which);
			QFETCH(ElevatedReason, reason);
			QFETCH(DwellingCircuit, required);
			DwellingData d = basic();
			d.generalSwitch = 40;
			d.individualLine = 50;   // IZ of the DI at least the IGA
			switch (which) {
				case 0: d.area = 160.1; break;
				case 1: d.heating = true; break;
				case 2: d.airConditioning = true; break;
				case 3: d.automation = true; break;
				case 4: d.vehicleCharging = true; d.singleFamily = true; break;
			}
			DwellingCheck r = checkDwelling(d);
			QCOMPARE(*r.grade, Electrification::Elevated);
			QCOMPARE(r.reasons, std::vector<ElevatedReason>{reason});
			QCOMPARE(*r.minimumSwitch, 40.0);
				// its circuit is required
			QCOMPARE(r.plannedCircuits, CheckOutcome::NotPassed);
			QVERIFY(std::find(r.missing.begin(), r.missing.end(), required) != r.missing.end());
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
				// with it, and a second RCD for six circuits (C13: its own)
			d.circuits.push_back(required);
			ResidualDevice id2;
			id2.name = "ID2";
			id2.sensitivity = 30;
			id2.rated = 40;
			id2.circuits = {required};
			d.residualDevices.push_back(id2);
			d.residualDevices[0].rated = 40;
			r = checkDwelling(d);
			QCOMPARE(r.plannedCircuits, CheckOutcome::Passed);
			QCOMPARE(r.residualNeeded, 2);
			QCOMPARE(r.outcome, CheckOutcome::Passed);
				// 25 A is not enough any more
			d.generalSwitch = 32;
			r = checkDwelling(d);
			QCOMPARE(r.generalSwitch, CheckOutcome::NotPassed);
		}

		void areaLimitIsStrict()
		{
				// "superior a 160 m2": 160 is still basic
			DwellingData d = basic();
			d.area = 160;
			QCOMPARE(*checkDwelling(d).grade, Electrification::Basic);
		}

		void sharedCarPark()
		{
				// EV charging in a block of flats: not elevated by itself,
				// C13 replaced by the common areas schemes (ITC-BT-52)
			DwellingData d = basic();
			d.vehicleCharging = true;
			d.singleFamily = false;
			DwellingCheck r = checkDwelling(d);
			QCOMPARE(*r.grade, Electrification::Basic);
			QCOMPARE(r.plannedCircuits, CheckOutcome::Passed);
				// not known whether single family: grade and circuits unknown
			d.singleFamily.reset();
			r = checkDwelling(d);
			QVERIFY(!r.grade);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingDwellingData, "singleFamily"));
			QCOMPARE(r.plannedCircuits, CheckOutcome::NotPerformed);
		}

		void extraCircuitMeansElevated()
		{
			using D = DwellingCircuit;
			for (D extra : {D::C6, D::C7, D::C8, D::C9, D::C11, D::C12C3, D::C12C4,
							D::C12C5}) {
				DwellingData d = basic();
				d.circuits.push_back(extra);
				const DwellingCheck r = checkDwelling(d);
				QCOMPARE(*r.grade, Electrification::Elevated);
				QCOMPARE(r.reasons, std::vector<ElevatedReason>{ElevatedReason::ExtraCircuit});
			}
				// a dryer circuit is not a condition of 2.3.2
			DwellingData d = basic();
			d.circuits.push_back(D::C10);
			QCOMPARE(*checkDwelling(d).grade, Electrification::Basic);
		}

		void unknownDataGivesNoGrade()
		{
			DwellingData d = basic();
			d.area.reset();
			d.heating.reset();
			DwellingCheck r = checkDwelling(d);
			QVERIFY(!r.grade);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingDwellingData, "area"));
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingDwellingData, "heating"));
			QCOMPARE(r.plannedCircuits, CheckOutcome::NotPerformed);
				// 25 A: enough for basic, not for elevated: not known
			QCOMPARE(r.generalSwitch, CheckOutcome::NotPerformed);
			d.generalSwitch = 40;
			QCOMPARE(checkDwelling(d).generalSwitch, CheckOutcome::Passed);
			d.generalSwitch = 20;
			QCOMPARE(checkDwelling(d).generalSwitch, CheckOutcome::NotPassed);
				// one known condition is enough for elevated
			d.airConditioning = true;
			QCOMPARE(*checkDwelling(d).grade, Electrification::Elevated);
		}

		void generalSwitchAndIndividualLine()
		{
			DwellingData d = basic();
			d.generalSwitch = 24.9;
			QCOMPARE(checkDwelling(d).generalSwitch, CheckOutcome::NotPassed);
			d = basic();
			d.individualLine = 25;   // equal: enough
			QCOMPARE(checkDwelling(d).individualLine, CheckOutcome::Passed);
			d.individualLine = 24.5;
			QCOMPARE(checkDwelling(d).individualLine, CheckOutcome::NotPassed);
			d.individualLine.reset();
			DwellingCheck r = checkDwelling(d);
			QCOMPARE(r.individualLine, CheckOutcome::NotPerformed);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingIndividualLine, "individualLine"));
			d = basic();
			d.generalSwitch.reset();
			r = checkDwelling(d);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingGeneralSwitch, "generalSwitch"));
			QCOMPARE(r.residualRating, CheckOutcome::NotPerformed);
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
		}

		void residualDevices()
		{
			using D = DwellingCircuit;
				// six circuits on one device: one per five is two
			DwellingData d = basic();
			d.circuits.push_back(D::C10);
			d.residualDevices[0].circuits = d.circuits;
			DwellingCheck r = checkDwelling(d);
			QCOMPARE(r.residualNeeded, 2);
			QCOMPARE(r.residualCount, CheckOutcome::NotPassed);
				// ten circuits, two devices: enough
			d.circuits = {D::C1, D::C2, D::C3, D::C4, D::C5, D::C6, D::C7, D::C8, D::C9, D::C10};
			d.residualDevices[0].circuits = {d.circuits.begin(), d.circuits.begin() + 5};
			ResidualDevice id2 = d.residualDevices[0];
			id2.name = "ID2";
			id2.circuits = {d.circuits.begin() + 5, d.circuits.end()};
			d.residualDevices.push_back(id2);
			QCOMPARE(checkDwelling(d).residualCount, CheckOutcome::Passed);
				// 300 mA, or a device under the IGA
			d = basic();
			d.residualDevices[0].sensitivity = 300;
			QCOMPARE(checkDwelling(d).residualSensitivity, CheckOutcome::NotPassed);
			d = basic();
			d.residualDevices[0].rated = 25;   // equal to the IGA: enough
			QCOMPARE(checkDwelling(d).residualRating, CheckOutcome::Passed);
			d.residualDevices[0].rated = 20;
			QCOMPARE(checkDwelling(d).residualRating, CheckOutcome::NotPassed);
				// an RCBO of one circuit: its rating is not compared
			d.residualDevices[0].combined = true;
			QCOMPARE(checkDwelling(d).residualRating, CheckOutcome::Passed);
				// a circuit with no device stated: not known
			d = basic();
			d.unprotectedCircuits = 1;
			r = checkDwelling(d);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingResidualDevice, "residualDevice"));
			QCOMPARE(r.residualSensitivity, CheckOutcome::NotPerformed);
			d.residualDevices[0].sensitivity.reset();
			r = checkDwelling(d);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingSensitivity, "residualDevice:ID1"));
		}

		void vehicleChargingHasItsOwnDevice()
		{
			using D = DwellingCircuit;
			DwellingData d = basic();
			d.vehicleCharging = true;
			d.singleFamily = true;
			d.generalSwitch = 40;
			d.individualLine = 50;
			d.residualDevices[0].rated = 40;
			d.circuits.push_back(D::C13);
			d.residualDevices[0].circuits = d.circuits;   // shared with C1..C5
			ResidualDevice extra = d.residualDevices[0];
			extra.name = "ID2";
			extra.circuits = {};
			d.residualDevices.push_back(extra);
			DwellingCheck r = checkDwelling(d);
			QVERIFY(r.vehicleCircuit);
			QCOMPARE(r.vehicleExclusive, CheckOutcome::NotPassed);
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
			d.residualDevices[0].circuits.pop_back();
			d.residualDevices[1].circuits = {D::C13};
			r = checkDwelling(d);
			QCOMPARE(r.vehicleExclusive, CheckOutcome::Passed);
			QCOMPARE(r.outcome, CheckOutcome::Passed);
		}

	private slots:
		void table_data()
		{
			QTest::addColumn<DwellingCircuit>("circuit");
			QTest::addColumn<double>("breaker");
			QTest::addColumn<double>("section");
			QTest::addColumn<int>("points");
			QTest::addColumn<double>("power");
			QTest::addColumn<double>("conduit");
			using D = DwellingCircuit;
			QTest::newRow("C1") << D::C1 << 10.0 << 1.5 << 30 << NONE << 16.0;
			QTest::newRow("C2") << D::C2 << 16.0 << 2.5 << 20 << NONE << 20.0;
			QTest::newRow("C3") << D::C3 << 25.0 << 6.0 << 2 << NONE << 25.0;
			QTest::newRow("C4") << D::C4 << 20.0 << 4.0 << 3 << NONE << 20.0;
			QTest::newRow("C4 per appliance") << D::C4Single << 16.0 << NONE << 1 << NONE << NONE;
			QTest::newRow("C5") << D::C5 << 16.0 << 2.5 << 6 << NONE << 20.0;
			QTest::newRow("C6 = C1") << D::C6 << 10.0 << 1.5 << 30 << NONE << 16.0;
			QTest::newRow("C7 = C2") << D::C7 << 16.0 << 2.5 << 20 << NONE << 20.0;
			QTest::newRow("C8") << D::C8 << 25.0 << 6.0 << -1 << 5750.0 << 25.0;
			QTest::newRow("C9") << D::C9 << 25.0 << 6.0 << -1 << 5750.0 << 25.0;
			QTest::newRow("C10") << D::C10 << 16.0 << 2.5 << 1 << NONE << 20.0;
			QTest::newRow("C11") << D::C11 << 10.0 << 1.5 << -1 << 2300.0 << 16.0;
			QTest::newRow("C12 as C3") << D::C12C3 << 25.0 << 6.0 << 2 << NONE << 25.0;
			QTest::newRow("C12 as C4") << D::C12C4 << 20.0 << 4.0 << 3 << NONE << 20.0;
			QTest::newRow("C12 as C5") << D::C12C5 << 16.0 << 2.5 << 6 << NONE << 20.0;
			QTest::newRow("C13") << D::C13 << NONE << 2.5 << 3 << NONE << 20.0;
		}

		void table()
		{
			QFETCH(DwellingCircuit, circuit);
			QFETCH(double, breaker);
			QFETCH(double, section);
			QFETCH(int, points);
			QFETCH(double, power);
			QFETCH(double, conduit);
			const DwellingSpec s = dwellingSpec(circuit);
			QCOMPARE(s.conduit.has_value(), conduit > 0);
			if (conduit > 0) QCOMPARE(*s.conduit, conduit);
			QCOMPARE(s.breaker.has_value(), breaker > 0);
			if (breaker > 0) QCOMPARE(*s.breaker, breaker);
			QCOMPARE(s.section.has_value(), section > 0);
			if (section > 0) QCOMPARE(*s.section, section);
			QCOMPARE(s.points.has_value(), points > 0);
			if (points > 0) QCOMPARE(*s.points, points);
			QCOMPARE(s.power.has_value(), power > 0);
			if (power > 0) QCOMPARE(*s.power, power);

				// exactly at the table: passes
			DwellingInput in = meeting(circuit, breaker, section, points, power, conduit);
			DwellingResult r = checkDwellingCircuit(in);
			QCOMPARE(r.outcome, CheckOutcome::Passed);
			QCOMPARE(r.status, Status::Ok);
				// one step beyond each value: fails
			if (breaker > 0) {
				DwellingInput b = in;
				b.ratedCurrent = breaker + 0.5;
				QCOMPARE(checkDwellingCircuit(b).breaker, CheckOutcome::NotPassed);
				QCOMPARE(checkDwellingCircuit(b).outcome, CheckOutcome::NotPassed);
			}
			if (section > 0) {
				DwellingInput b = in;
				b.section = section - 0.5;
				QCOMPARE(checkDwellingCircuit(b).section, CheckOutcome::NotPassed);
			}
			if (points > 0) {
				DwellingInput b = in;
				b.points = points + 1;
				QCOMPARE(checkDwellingCircuit(b).points, CheckOutcome::NotPassed);
			}
			if (power > 0) {
				DwellingInput b = in;
				b.power = power + 1;
				QCOMPARE(checkDwellingCircuit(b).power, CheckOutcome::NotPassed);
			}
			if (conduit > 0) {
				DwellingInput b = in;
				b.conduit = conduit - 0.5;
				QCOMPARE(checkDwellingCircuit(b).conduit, CheckOutcome::NotPassed);
				b.conduit.reset();
				const auto r = checkDwellingCircuit(b);
				QVERIFY(hasIssue(r.diagnostics, Issue::MissingConduit, "conduit"));
				QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			}
		}

		void lowerBreakerIsNotFlagged()
		{
			DwellingInput in = meeting(DwellingCircuit::C2, 10, 2.5, 5, 0);
			QCOMPARE(checkDwellingCircuit(in).breaker, CheckOutcome::Passed);
		}

		void copperOnly()
		{
			DwellingInput in = meeting(DwellingCircuit::C3, 25, 10, 1, 0);
			in.material = Material::Aluminium;
			const auto r = checkDwellingCircuit(in);
			QCOMPARE(r.copper, CheckOutcome::NotPassed);
			QCOMPARE(r.section, CheckOutcome::Passed);
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
		}

		/// ITC-BT-25 3: "como maximo el 3 %": 3 % passes, above fails
		void dropAtRated()
		{
			DwellingInput in = meeting(DwellingCircuit::C1, 10, 1.5, 10, 0);
			in.dropAtRated = 3.0;
			QCOMPARE(checkDwellingCircuit(in).drop, CheckOutcome::Passed);
			in.dropAtRated = 3.0001;
			auto r = checkDwellingCircuit(in);
			QCOMPARE(r.drop, CheckOutcome::NotPassed);
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
				// unknown drop: not performed, the rest still checked
			in.dropAtRated.reset();
			in.ratedCurrent = 16;
			r = checkDwellingCircuit(in);
			QCOMPARE(r.drop, CheckOutcome::NotPerformed);
			QCOMPARE(r.breaker, CheckOutcome::NotPassed);
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
			in.ratedCurrent = 10;
			r = checkDwellingCircuit(in);
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			QCOMPARE(r.status, Status::NotComputable);
			in.dropAtRated = -1;
			QCOMPARE(checkDwellingCircuit(in).status, Status::InvalidInput);
		}

		void nothingInvented()
		{
				// not a dwelling circuit: nothing checked
			DwellingInput none;
			auto r = checkDwellingCircuit(none);
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			QVERIFY(r.diagnostics.empty());
				// a C2 with nothing known
			DwellingInput c2;
			c2.circuit = DwellingCircuit::C2;
			r = checkDwellingCircuit(c2);
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingRatedCurrent, "ratedCurrent"));
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingSection, "section"));
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingMaterial, "material"));
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingPoints, "points"));
				// C2 has no power limit: its power is not asked for
			QVERIFY(!hasIssue(r.diagnostics, Issue::MissingPower, "power"));
				// C8 has one
			c2.circuit = DwellingCircuit::C8;
			r = checkDwellingCircuit(c2);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingPower, "power"));
			QVERIFY(!hasIssue(r.diagnostics, Issue::MissingPoints, "points"));
		}

		void invalidValues()
		{
			DwellingInput in = meeting(DwellingCircuit::C2, 16, 2.5, 5, 0);
			in.points = 0;
			auto r = checkDwellingCircuit(in);
			QCOMPARE(r.status, Status::InvalidInput);
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			QCOMPARE(r.breaker, CheckOutcome::NotPerformed);
			in = meeting(DwellingCircuit::C2, 16, 2.5, 5, 0);
			in.section = -2.5;
			r = checkDwellingCircuit(in);
			QVERIFY(hasIssue(r.diagnostics, Issue::NotPositive, "section"));
			QCOMPARE(r.status, Status::InvalidInput);
		}

		void minimumCircuits()
		{
			using D = DwellingCircuit;
			auto r = checkMinimumCircuits({});
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			r = checkMinimumCircuits({D::C1, D::C2, D::C3, D::C4, D::C5});
			QCOMPARE(r.outcome, CheckOutcome::Passed);
				// C4 split per appliance, note (8)
			r = checkMinimumCircuits({D::C1, D::C2, D::C3, D::C4Single, D::C4Single, D::C5});
			QCOMPARE(r.outcome, CheckOutcome::Passed);
			r = checkMinimumCircuits({D::C1, D::C2, D::C5, D::C10});
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
			QCOMPARE(r.missing, (std::vector<D>{D::C3, D::C4}));
				// a C6 is not a C1, nor a C12 a C3
			r = checkMinimumCircuits({D::C6, D::C2, D::C12C3, D::C4, D::C5});
			QCOMPARE(r.missing, (std::vector<D>{D::C1, D::C3}));
			QCOMPARE(r.rule.id, std::string("REBT-VIV-003"));
		}

		void ownTransformerLimits()
		{
			const VoltageDropLimit l = rules::ownTransformerLighting();
			QCOMPARE(l.percent, 4.5);
			QVERIFY(!l.strict);   // "maximas admisibles"
			QCOMPARE(l.rule.id, std::string("REBT-CDT-005"));
			const VoltageDropLimit o = rules::ownTransformerOtherUses();
			QCOMPARE(o.percent, 6.5);
			QVERIFY(!o.strict);
			QCOMPARE(o.rule.id, std::string("REBT-CDT-006"));
		}
};

QTEST_GUILESS_MAIN(tst_rebtdwelling)
#include "tst_rebtdwelling.moc"
