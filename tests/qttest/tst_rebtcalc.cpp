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
#include "rebt/rebtcalc.h"
#include "rebt/rebtrules.h"

#include <QtTest>

#include <algorithm>
#include <cmath>
#include <limits>

using namespace rebt;

Q_DECLARE_METATYPE(rebt::System)
Q_DECLARE_METATYPE(rebt::Issue)

namespace {

/// Reference values are worked out apart (docs/rebt/casos_h1.md)
/// and must match within 0.1 %
bool near(double value, double reference)
{
	return std::fabs(value - reference) <= 1e-3 * std::fabs(reference);
}

bool hasIssue(const std::vector<Diagnostic> &list, Issue issue,
			  const std::string &field)
{
	return std::find(list.begin(), list.end(), Diagnostic{issue, field})
		   != list.end();
}

bool hasAssumption(const std::vector<Assumption> &list, Assumption a)
{
	return std::find(list.begin(), list.end(), a) != list.end();
}

bool hasRule(const std::vector<NormRef> &list, const std::string &id)
{
	return std::any_of(list.begin(), list.end(),
					   [&id](const NormRef &r) { return r.id == id; });
}

CircuitInput currentInput(System system, double power, double voltage,
						  double cos_phi)
{
	CircuitInput input;
	input.system = system;
	input.power = power;
	input.voltage = voltage;
	input.powerFactor = cos_phi;
	return input;
}

/// Single phase, 10 A, cos 1, 2.5 mm2, conductivity 50: dU = 0.16 L
CircuitInput boundaryInput(double length)
{
	CircuitInput input;
	input.system = System::SinglePhase;
	input.voltage = 230.0;
	input.powerFactor = 1.0;
	input.designCurrent = 10.0;
	input.length = length;
	input.section = 2.5;
	input.conductivity = 50.0;
	return input;
}

} // namespace

class tst_rebtcalc : public QObject
{
		Q_OBJECT

	private slots:
		void designCurrent_data()
		{
			QTest::addColumn<System>("system");
			QTest::addColumn<double>("power");
			QTest::addColumn<double>("voltage");
			QTest::addColumn<double>("cosPhi");
			QTest::addColumn<double>("efficiency");   // 0 = not given
			QTest::addColumn<double>("expected");

			QTest::newRow("M1 3680 W 230 V cos 1")
				<< System::SinglePhase << 3680.0 << 230.0 << 1.0 << 0.0 << 16.0;
			QTest::newRow("M2 2300 W 230 V cos 0.8")
				<< System::SinglePhase << 2300.0 << 230.0 << 0.8 << 0.0 << 12.5;
			QTest::newRow("M3 5750 W 230 V cos 0.9")
				<< System::SinglePhase << 5750.0 << 230.0 << 0.9 << 0.0 << 27.777778;
			QTest::newRow("T1 10 kW 400 V cos 0.85")
				<< System::ThreePhase << 10000.0 << 400.0 << 0.85 << 0.0 << 16.980890;
			QTest::newRow("T2 15 kW 400 V cos 0.82 eta 0.9")
				<< System::ThreePhase << 15000.0 << 400.0 << 0.82 << 0.9 << 29.336904;
			QTest::newRow("T3 22 kW 400 V cos 1")
				<< System::ThreePhase << 22000.0 << 400.0 << 1.0 << 0.0 << 31.754265;
		}

		void designCurrent()
		{
			QFETCH(System, system);
			QFETCH(double, power);
			QFETCH(double, voltage);
			QFETCH(double, cosPhi);
			QFETCH(double, efficiency);
			QFETCH(double, expected);

			CircuitInput input = currentInput(system, power, voltage, cosPhi);
			if (efficiency > 0.0)
				input.efficiency = efficiency;

			const CurrentResult r = rebt::designCurrent(input);
			QCOMPARE(r.status, Status::Ok);
			QVERIFY(r.diagnostics.empty());
			QVERIFY(r.current.has_value());
			QVERIFY2(near(*r.current, expected),
					 qPrintable(QStringLiteral("%1 A, expected %2 A")
								.arg(*r.current, 0, 'f', 6).arg(expected, 0, 'f', 6)));
			QVERIFY(hasRule(r.rules, "REBT-I-001"));
			QCOMPARE(hasAssumption(r.assumptions, Assumption::EfficiencyNotApplied),
					 efficiency <= 0.0);
			QCOMPARE(hasAssumption(r.assumptions, Assumption::BalancedLoad),
					 system == System::ThreePhase);
		}

		void designCurrentMissing_data()
		{
			QTest::addColumn<int>("drop");   // which field is left empty
			QTest::addColumn<Issue>("issue");
			QTest::addColumn<QString>("field");

			QTest::newRow("no system") << 0 << Issue::MissingSystem << "system";
			QTest::newRow("no voltage") << 1 << Issue::MissingVoltage << "voltage";
			QTest::newRow("no power") << 2 << Issue::MissingPower << "power";
			QTest::newRow("no power factor")
				<< 3 << Issue::MissingPowerFactor << "powerFactor";
		}

		void designCurrentMissing()
		{
			QFETCH(int, drop);
			QFETCH(Issue, issue);
			QFETCH(QString, field);

			CircuitInput input = currentInput(System::SinglePhase, 3680.0, 230.0, 1.0);
			switch (drop) {
				case 0: input.system.reset(); break;
				case 1: input.voltage.reset(); break;
				case 2: input.power.reset(); break;
				case 3: input.powerFactor.reset(); break;
			}
			const CurrentResult r = rebt::designCurrent(input);
			QCOMPARE(r.status, Status::NotComputable);
			QVERIFY(!r.current.has_value());
			QCOMPARE(r.diagnostics.size(), size_t(1));
			QVERIFY(hasIssue(r.diagnostics, issue, field.toStdString()));
		}

		void designCurrentNothingKnown()
		{
			const CurrentResult r = rebt::designCurrent(CircuitInput());
			QCOMPARE(r.status, Status::NotComputable);
			QVERIFY(!r.current.has_value());
			QCOMPARE(r.diagnostics.size(), size_t(4));
		}

		void designCurrentInvalid_data()
		{
			QTest::addColumn<double>("power");
			QTest::addColumn<double>("voltage");
			QTest::addColumn<double>("cosPhi");
			QTest::addColumn<double>("efficiency");   // 0 = not given
			QTest::addColumn<Issue>("issue");
			QTest::addColumn<QString>("field");

			const double nan = std::numeric_limits<double>::quiet_NaN();
			const double inf = std::numeric_limits<double>::infinity();
			QTest::newRow("cos 0") << 1000.0 << 230.0 << 0.0 << 0.0
								   << Issue::OutOfRange << "powerFactor";
			QTest::newRow("cos 1.01") << 1000.0 << 230.0 << 1.01 << 0.0
									  << Issue::OutOfRange << "powerFactor";
			QTest::newRow("cos -0.8") << 1000.0 << 230.0 << -0.8 << 0.0
									  << Issue::OutOfRange << "powerFactor";
			QTest::newRow("eta 1.2") << 1000.0 << 230.0 << 0.9 << 1.2
									 << Issue::OutOfRange << "efficiency";
			QTest::newRow("eta -0.5") << 1000.0 << 230.0 << 0.9 << -0.5
									  << Issue::OutOfRange << "efficiency";
			QTest::newRow("power 0") << 0.0 << 230.0 << 0.9 << 0.0
									 << Issue::NotPositive << "power";
			QTest::newRow("power negative") << -100.0 << 230.0 << 0.9 << 0.0
											<< Issue::NotPositive << "power";
			QTest::newRow("voltage 0") << 1000.0 << 0.0 << 0.9 << 0.0
									   << Issue::NotPositive << "voltage";
			QTest::newRow("power NaN") << nan << 230.0 << 0.9 << 0.0
									   << Issue::NotFinite << "power";
			QTest::newRow("voltage inf") << 1000.0 << inf << 0.9 << 0.0
										 << Issue::NotFinite << "voltage";
		}

		void designCurrentInvalid()
		{
			QFETCH(double, power);
			QFETCH(double, voltage);
			QFETCH(double, cosPhi);
			QFETCH(double, efficiency);
			QFETCH(Issue, issue);
			QFETCH(QString, field);

			CircuitInput input = currentInput(System::SinglePhase, power, voltage, cosPhi);
			if (efficiency != 0.0)
				input.efficiency = efficiency;
			const CurrentResult r = rebt::designCurrent(input);
			QCOMPARE(r.status, Status::InvalidInput);
			QVERIFY(!r.current.has_value());
			QVERIFY(hasIssue(r.diagnostics, issue, field.toStdString()));
		}

		void invalidWinsOverMissing()
		{
			CircuitInput input;
			input.system = System::SinglePhase;
			input.power = -1.0;
			const CurrentResult r = rebt::designCurrent(input);
			QCOMPARE(r.status, Status::InvalidInput);
			QVERIFY(hasIssue(r.diagnostics, Issue::NotPositive, "power"));
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingVoltage, "voltage"));
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingPowerFactor, "powerFactor"));
		}

		void resistivity()
		{
			QVERIFY(near(rebt::resistivity(Material::Copper, 20.0), 1.0 / 58.0));
			QVERIFY(near(rebt::resistivity(Material::Copper, 70.0), 0.02062931));
			QVERIFY(near(rebt::resistivity(Material::Aluminium, 20.0), 0.028264));
			QVERIFY(near(rebt::resistivity(Material::Aluminium, 90.0), 0.03623727));
		}

		void voltageDropM1()
		{
			// single phase, 16 A given, 25 m, 2.5 mm2, conductivity 48, cos 1
			CircuitInput input;
			input.system = System::SinglePhase;
			input.voltage = 230.0;
			input.powerFactor = 1.0;
			input.designCurrent = 16.0;
			input.length = 25.0;
			input.section = 2.5;
			input.conductivity = 48.0;

			const VoltageDropResult r = rebt::voltageDrop(input);
			QCOMPARE(r.status, Status::Ok);
			QVERIFY(near(*r.drop, 6.666667));
			QVERIFY(near(*r.percent, 2.898551));
			QVERIFY(hasAssumption(r.assumptions, Assumption::CurrentGiven));
			QVERIFY(hasAssumption(r.assumptions, Assumption::ConductivityGiven));
			QVERIFY(hasAssumption(r.assumptions, Assumption::ReactanceNeglected));
			QVERIFY(hasAssumption(r.assumptions, Assumption::LoadAtEnd));
			QVERIFY(hasRule(r.rules, "REBT-CDT-001"));
			QVERIFY(!hasRule(r.rules, "REBT-I-001"));
		}

		void voltageDropT1()
		{
			// three phase, current from 10 kW 400 V cos 0.85, 50 m, 6 mm2,
			// conductivity 44, X 0.08 ohm/km
			CircuitInput input = currentInput(System::ThreePhase, 10000.0, 400.0, 0.85);
			input.length = 50.0;
			input.section = 6.0;
			input.conductivity = 44.0;
			input.reactance = 0.08;

			const VoltageDropResult r = rebt::voltageDrop(input);
			QCOMPARE(r.status, Status::Ok);
			QVERIFY(near(*r.current, 16.980890));
			QVERIFY(near(*r.drop, 4.796823));
			QVERIFY(near(*r.percent, 1.199206));
			QVERIFY(!hasAssumption(r.assumptions, Assumption::ReactanceNeglected));
			QVERIFY(hasAssumption(r.assumptions, Assumption::BalancedLoad));
			QVERIFY(hasRule(r.rules, "REBT-I-001"));
		}

		void voltageDropM2FromMaterial()
		{
			// single phase, 10 A, 30 m, 1.5 mm2, copper at 70 degC, cos 0.9
			CircuitInput input;
			input.system = System::SinglePhase;
			input.voltage = 230.0;
			input.powerFactor = 0.9;
			input.designCurrent = 10.0;
			input.length = 30.0;
			input.section = 1.5;
			input.material = Material::Copper;
			input.conductorTemperature = 70.0;

			const VoltageDropResult r = rebt::voltageDrop(input);
			QCOMPARE(r.status, Status::Ok);
			QVERIFY(near(*r.drop, 7.426552));
			QVERIFY(near(*r.percent, 3.228936));
			QVERIFY(hasAssumption(r.assumptions, Assumption::ResistivityFromMaterial));
			QVERIFY(hasRule(r.rules, "REBT-RHO-001"));
		}

		void voltageDropT2Aluminium()
		{
			// three phase, 100 A, 80 m, 50 mm2, aluminium at 90 degC, cos 0.9
			CircuitInput input;
			input.system = System::ThreePhase;
			input.voltage = 400.0;
			input.powerFactor = 0.9;
			input.designCurrent = 100.0;
			input.length = 80.0;
			input.section = 50.0;
			input.material = Material::Aluminium;
			input.conductorTemperature = 90.0;

			const VoltageDropResult r = rebt::voltageDrop(input);
			QCOMPARE(r.status, Status::Ok);
			QVERIFY(near(*r.drop, 9.038131));
			QVERIFY(near(*r.percent, 2.259533));
		}

		void voltageDropMissing()
		{
			// nothing but the system: everything else is reported, no value
			CircuitInput input;
			input.system = System::SinglePhase;
			const VoltageDropResult r = rebt::voltageDrop(input);
			QCOMPARE(r.status, Status::NotComputable);
			QVERIFY(!r.drop.has_value());
			QVERIFY(!r.percent.has_value());
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingVoltage, "voltage"));
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingPowerFactor, "powerFactor"));
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingLength, "length"));
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingSection, "section"));
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingMaterial, "material"));
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingTemperature,
							 "conductorTemperature"));
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingPower, "power"));
			// each problem is listed once
			QCOMPARE(r.diagnostics.size(), size_t(7));
		}

		void voltageDropMaterialWithoutTemperature()
		{
			CircuitInput input = boundaryInput(20.0);
			input.conductivity.reset();
			input.material = Material::Copper;
			const VoltageDropResult r = rebt::voltageDrop(input);
			QCOMPARE(r.status, Status::NotComputable);
			QVERIFY(!r.percent.has_value());
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingTemperature,
							 "conductorTemperature"));
			QCOMPARE(r.diagnostics.size(), size_t(1));
		}

		void voltageDropInvalid_data()
		{
			QTest::addColumn<int>("field");
			QTest::addColumn<double>("value");
			QTest::addColumn<Issue>("issue");
			QTest::addColumn<QString>("name");

			QTest::newRow("length 0") << 0 << 0.0 << Issue::NotPositive << "length";
			QTest::newRow("section -1") << 1 << -1.0 << Issue::NotPositive << "section";
			QTest::newRow("reactance -0.1") << 2 << -0.1 << Issue::Negative << "reactance";
			QTest::newRow("conductivity 0") << 3 << 0.0 << Issue::NotPositive << "conductivity";
			QTest::newRow("current 0") << 4 << 0.0 << Issue::NotPositive << "designCurrent";
		}

		void voltageDropInvalid()
		{
			QFETCH(int, field);
			QFETCH(double, value);
			QFETCH(Issue, issue);
			QFETCH(QString, name);

			CircuitInput input = boundaryInput(20.0);
			switch (field) {
				case 0: input.length = value; break;
				case 1: input.section = value; break;
				case 2: input.reactance = value; break;
				case 3: input.conductivity = value; break;
				case 4: input.designCurrent = value; break;
			}
			const VoltageDropResult r = rebt::voltageDrop(input);
			QCOMPARE(r.status, Status::InvalidInput);
			QVERIFY(!r.percent.has_value());
			QVERIFY(hasIssue(r.diagnostics, issue, name.toStdString()));
		}

		void reactanceZeroIsNotNeglected()
		{
			CircuitInput input = boundaryInput(20.0);
			input.reactance = 0.0;
			const VoltageDropResult r = rebt::voltageDrop(input);
			QCOMPARE(r.status, Status::Ok);
			QVERIFY(!hasAssumption(r.assumptions, Assumption::ReactanceNeglected));
		}

		void limitBoundary_data()
		{
			QTest::addColumn<double>("length");
			QTest::addColumn<bool>("strict");
			QTest::addColumn<int>("outcome");
			QTest::addColumn<double>("percent");

			// 3 % of 230 V is 6.9 V, reached at 43.125 m
			QTest::newRow("equal, strict") << 43.125 << true
				<< int(CheckOutcome::NotPassed) << 3.0;
			QTest::newRow("equal, not strict") << 43.125 << false
				<< int(CheckOutcome::Passed) << 3.0;
			QTest::newRow("just below") << 43.0 << true
				<< int(CheckOutcome::Passed) << 2.991304348;
			QTest::newRow("just above") << 43.25 << false
				<< int(CheckOutcome::NotPassed) << 3.008695652;
		}

		void limitBoundary()
		{
			QFETCH(double, length);
			QFETCH(bool, strict);
			QFETCH(int, outcome);
			QFETCH(double, percent);

			VoltageDropLimit limit = rules::dwellingInterior();
			QCOMPARE(limit.percent, 3.0);
			limit.strict = strict;

			const VoltageDropResult drop = rebt::voltageDrop(boundaryInput(length));
			QCOMPARE(drop.status, Status::Ok);
			QVERIFY(near(*drop.percent, percent));
			const CheckResult check = rebt::checkVoltageDrop(drop, limit);
			QCOMPARE(int(check.outcome), outcome);
			QCOMPARE(check.limit, 3.0);
			QCOMPARE(check.rule.id, std::string("REBT-CDT-002"));
		}

		void limitNotPerformed()
		{
			const VoltageDropResult drop = rebt::voltageDrop(CircuitInput());
			const CheckResult check = rebt::checkVoltageDrop(
				drop, rules::otherInteriorOtherUses());
			QCOMPARE(check.outcome, CheckOutcome::NotPerformed);
			QVERIFY(!check.value.has_value());
		}

		void limitsCarryTheirSource()
		{
			const VoltageDropLimit limits[] = {
				rules::dwellingInterior(),
				rules::otherInteriorLighting(),
				rules::otherInteriorOtherUses()
			};
			const double expected[] = {3.0, 3.0, 5.0};
			for (int i = 0; i < 3; ++i) {
				QCOMPARE(limits[i].percent, expected[i]);
				QVERIFY(limits[i].strict);
				QCOMPARE(limits[i].rule.source, std::string("ITC-BT-19"));
				QCOMPARE(limits[i].rule.section, std::string("2.2.2"));
				QVERIFY(!limits[i].rule.version.empty());
				QVERIFY(limits[i].rule.limitations.find("partial")
						!= std::string::npos);
			}
		}
};

QTEST_GUILESS_MAIN(tst_rebtcalc)
#include "tst_rebtcalc.moc"
