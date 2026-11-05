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
#include "rebt/rebtpv.h"

#include <QtTest>

#include <algorithm>

using namespace rebt;

namespace {

bool hasIssue(const PvStringResult &r, Issue issue, const std::string &field)
{
	return std::find(r.diagnostics.begin(), r.diagnostics.end(), Diagnostic{issue, field})
		   != r.diagnostics.end();
}

bool assumes(const PvStringResult &r, Assumption a)
{
	return std::find(r.assumptions.begin(), r.assumptions.end(), a) != r.assumptions.end();
}

bool cites(const PvStringResult &r, const std::string &id)
{
	return std::any_of(r.rules.begin(), r.rules.end(),
					   [&](const NormRef &n) { return n.id == id; });
}

/**
	The module of the 5 kW example of Prysmian Club (Voc 42.5 V, Vmpp 37.0 V,
	Isc 9.18 A, Impp 8.64 A, series fuse 15 A) with test coefficients
	-0.29 / -0.37 %/degC and 1000 V; a test inverter of 600 V and MPPT 90-550 V.
	Two strings of 8, 30 m of 4 mm2 copper at 90 degC, -10 / 70 degC.
*/
PvStringInput example()
{
	PvStringInput in;
	in.module.openCircuitVoltage = 42.5;
	in.module.mppVoltage = 37.0;
	in.module.shortCircuitCurrent = 9.18;
	in.module.mppCurrent = 8.64;
	in.module.vocCoefficient = -0.29;
	in.module.vmppCoefficient = -0.37;
	in.module.maxSystemVoltage = 1000.0;
	in.module.maxSeriesFuse = 15.0;
	in.module.source = "test datasheet";
	in.inverter.maxDcVoltage = 600.0;
	in.inverter.mpptMinVoltage = 90.0;
	in.inverter.mpptMaxVoltage = 550.0;
	in.inverter.maxInputCurrent = 22.0;
	in.inverter.maxShortCircuitCurrent = 27.5;
	in.inverter.source = "test datasheet";
	in.modulesPerString = 8;
	in.parallelStrings = 2;
	in.minCellTemperature = -10.0;
	in.maxCellTemperature = 70.0;
	in.temperatureJustification = "test site";
	in.cableLength = 30.0;
	in.cableSection = 4.0;
	in.cableMaterial = Material::Copper;
	in.conductorTemperature = 90.0;
	in.cableAdmissible = 20.0;
	return in;
}

} // namespace

/**
	Reference values worked out apart with the formulas of the header:
	UOC MAX 8 x 42.5 x (1 + 0.0029 x 35) = 374.51 V; UMPP 296 x (1 - 0.0037 x 45)
	= 246.716 V and 296 x (1 + 0.0037 x 35) = 334.332 V; drop 2 x 30 x 8.64 x
	rho(90) / 4 over 296 V, rho = (1 + 0.00393 x 70) / 58: 0.962564 %.
*/
class tst_rebtpv : public QObject
{
		Q_OBJECT

	private slots:
		void example2x8()
		{
			const auto r = checkPvString(example());
			QCOMPARE(r.status, Status::Ok);
			QVERIFY(qAbs(*r.openCircuitMax - 374.51) < 1e-9);
			QVERIFY(qAbs(*r.mppMin - 246.716) < 1e-9);
			QVERIFY(qAbs(*r.mppMax - 334.332) < 1e-9);
			QVERIFY(qAbs(*r.inputShortCircuit - 18.36) < 1e-12);
			QVERIFY(qAbs(*r.inputMppCurrent - 17.28) < 1e-12);
			QVERIFY(qAbs(*r.dropPercent - 0.962564) < 1e-6);
				// (2 - 1) x 1.25 x 9.18 = 11.475 A <= 15 A: no string fuses
			QCOMPARE(*r.protectionRequired, false);
			QVERIFY(qAbs(*r.cableRequired - 11.475) < 1e-12);
			QCOMPARE(r.voltage, CheckOutcome::Passed);
			QCOMPARE(r.mppt, CheckOutcome::Passed);
			QCOMPARE(r.current, CheckOutcome::Passed);
			QCOMPARE(r.protection, CheckOutcome::Passed);
			QCOMPARE(r.cable, CheckOutcome::Passed);
			QCOMPARE(r.drop, CheckOutcome::Passed);
			QCOMPARE(r.outcome, CheckOutcome::Passed);
			for (const char *id : {"REBT-FV-001", "REBT-FV-002", "REBT-FV-003", "REBT-FV-004",
								   "REBT-FV-005", "REBT-FV-006"})
				QVERIFY2(cites(r, id), id);
			QVERIFY(assumes(r, Assumption::IscMaxFactor));
			QVERIFY(assumes(r, Assumption::CurrentsAtStc));
			QVERIFY(assumes(r, Assumption::SingleStringCable));
			QVERIFY(r.diagnostics.empty());
		}

		void voltageLimits()
		{
			PvStringInput in = example();
				// 14 modules: 655.39 V over the 600 V of the inverter
			in.modulesPerString = 14;
			auto r = checkPvString(in);
			QVERIFY(qAbs(*r.openCircuitMax - 655.3925) < 1e-9);
			QCOMPARE(r.voltage, CheckOutcome::NotPassed);
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
				// equal to the limit: rated at least UOC MAX
			in.modulesPerString = 8;
			in.inverter.maxDcVoltage = 374.51;
			QCOMPARE(checkPvString(in).voltage, CheckOutcome::Passed);
			in.inverter.maxDcVoltage = 374.5;
			QCOMPARE(checkPvString(in).voltage, CheckOutcome::NotPassed);
				// the module's own limit too
			in.inverter.maxDcVoltage = 600.0;
			in.module.maxSystemVoltage = 360.0;
			QCOMPARE(checkPvString(in).voltage, CheckOutcome::NotPassed);
		}

		void mpptWindow()
		{
			PvStringInput in = example();
				// UMPP min 246.716 V under a 250 V start
			in.inverter.mpptMinVoltage = 250.0;
			auto r = checkPvString(in);
			QCOMPARE(r.mppt, CheckOutcome::NotPassed);
				// UMPP max 334.332 V over a 330 V top
			in.inverter.mpptMinVoltage = 90.0;
			in.inverter.mpptMaxVoltage = 330.0;
			QCOMPARE(checkPvString(in).mppt, CheckOutcome::NotPassed);
			in.inverter.mpptMaxVoltage = 334.332;
			QCOMPARE(checkPvString(in).mppt, CheckOutcome::Passed);
				// MPPT window upside down: not a value
			in.inverter.mpptMinVoltage = 400.0;
			r = checkPvString(in);
			QCOMPARE(r.status, Status::InvalidInput);
			QVERIFY(hasIssue(r, Issue::OutOfRange, "mpptMinVoltage"));
		}

		void inputCurrents()
		{
			PvStringInput in = example();
			in.inverter.maxShortCircuitCurrent = 18.0;   // 18.36 A in
			QCOMPARE(checkPvString(in).current, CheckOutcome::NotPassed);
			in.inverter.maxShortCircuitCurrent = 27.5;
			in.inverter.maxInputCurrent = 17.0;          // 17.28 A in
			QCOMPARE(checkPvString(in).current, CheckOutcome::NotPassed);
			in.inverter.maxInputCurrent = 17.28;
			QCOMPARE(checkPvString(in).current, CheckOutcome::Passed);
		}

		void stringProtection()
		{
				// 3 strings: 2 x 11.475 = 22.95 A > 15 A, fuses needed
			PvStringInput in = example();
			in.parallelStrings = 3;
			auto r = checkPvString(in);
			QCOMPARE(*r.protectionRequired, true);
			QCOMPARE(r.protection, CheckOutcome::NotPassed);
			QVERIFY(hasIssue(r, Issue::MissingStringProtection, "stringFuse"));
			QVERIFY(qAbs(*r.fuseMin - 13.77) < 1e-12);
			QVERIFY(qAbs(*r.fuseMax - 15.0) < 1e-12);   // min(22.032, 15)
				// 15 A: 13.77 < 15 < 22.032 and <= 15
			in.stringFuse = 15.0;
			r = checkPvString(in);
			QCOMPARE(r.protection, CheckOutcome::Passed);
			QVERIFY(!hasIssue(r, Issue::MissingStringProtection, "stringFuse"));
				// the cable then needs the In of the fuse
			QCOMPARE(*r.cableRequired, 15.0);
				// 13.77 A itself is not over 1.5 Isc
			in.stringFuse = 13.77;
			QCOMPARE(checkPvString(in).protection, CheckOutcome::NotPassed);
			in.stringFuse = 16.0;   // over the module's 15 A
			QCOMPARE(checkPvString(in).protection, CheckOutcome::NotPassed);
			in.module.maxSeriesFuse = 25.0;
			in.stringFuse = 22.032;   // 2.4 Isc itself is not under it
			QCOMPARE(checkPvString(in).protection, CheckOutcome::NotPassed);
			in.stringFuse = 20.0;
			QCOMPARE(checkPvString(in).protection, CheckOutcome::Passed);
		}

		void stringCable()
		{
				// 4 strings, no fuse: 1.25 x 9.18 x 3 = 34.425 A
			PvStringInput in = example();
			in.parallelStrings = 4;
			in.module.maxSeriesFuse = 40.0;   // fuses not needed
			in.inverter.maxShortCircuitCurrent = 40.0;
			in.inverter.maxInputCurrent = 40.0;
			auto r = checkPvString(in);
			QVERIFY(qAbs(*r.cableRequired - 34.425) < 1e-12);
			QCOMPARE(r.cable, CheckOutcome::NotPassed);
			QVERIFY(!assumes(r, Assumption::SingleStringCable));
			in.cableAdmissible = 34.425;
			QCOMPARE(checkPvString(in).cable, CheckOutcome::Passed);
				// one string: 1.25 Isc
			in = example();
			in.parallelStrings = 1;
			r = checkPvString(in);
			QVERIFY(qAbs(*r.cableRequired - 11.475) < 1e-12);
			QCOMPARE(*r.protectionRequired, false);
		}

		void dcVoltageDrop()
		{
				// 2.5 mm2: 1.540102 %, not under 1.5 %
			PvStringInput in = example();
			in.cableSection = 2.5;
			auto r = checkPvString(in);
			QVERIFY(qAbs(*r.dropPercent - 1.540102) < 1e-6);
			QCOMPARE(r.drop, CheckOutcome::NotPassed);
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
				// no conductor temperature: no drop
			in = example();
			in.conductorTemperature.reset();
			r = checkPvString(in);
			QVERIFY(!r.dropPercent);
			QVERIFY(hasIssue(r, Issue::MissingTemperature, "conductorTemperature"));
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
		}

		void sourcesAndMissingData()
		{
				// a datasheet with no source is not used
			PvStringInput in = example();
			in.module.source = " ";
			auto r = checkPvString(in);
			QVERIFY(hasIssue(r, Issue::MissingJustification, "module"));
			QVERIFY(!r.openCircuitMax);
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			in = example();
			in.inverter.source.clear();
			r = checkPvString(in);
			QVERIFY(hasIssue(r, Issue::MissingJustification, "inverter"));
			QCOMPARE(r.voltage, CheckOutcome::NotPerformed);
				// temperatures need their source
			in = example();
			in.temperatureJustification.clear();
			r = checkPvString(in);
			QVERIFY(hasIssue(r, Issue::MissingJustification, "cellTemperature"));
			QVERIFY(!r.openCircuitMax && !r.mppMin);
			in = example();
			in.module.vmppCoefficient.reset();
			r = checkPvString(in);
			QVERIFY(hasIssue(r, Issue::MissingModuleData, "vmppCoefficient"));
			QVERIFY(!r.mppMin);
			QVERIFY(r.openCircuitMax);
			in = example();
			in.parallelStrings.reset();
			r = checkPvString(in);
			QVERIFY(hasIssue(r, Issue::MissingStringLayout, "parallelStrings"));
			in = example();
			in.modulesPerString = 0;
			r = checkPvString(in);
			QCOMPARE(r.status, Status::InvalidInput);
			QVERIFY(hasIssue(r, Issue::NotPositive, "modulesPerString"));
			in = example();
			in.minCellTemperature = 80.0;
			r = checkPvString(in);
			QCOMPARE(r.status, Status::InvalidInput);
			QVERIFY(hasIssue(r, Issue::OutOfRange, "minCellTemperature"));
				// a negative coefficient is a value; a positive current is needed
			in = example();
			in.module.shortCircuitCurrent = -9.18;
			r = checkPvString(in);
			QVERIFY(hasIssue(r, Issue::NotPositive, "shortCircuitCurrent"));
			in = example();
			in.cableAdmissible.reset();
			r = checkPvString(in);
			QVERIFY(hasIssue(r, Issue::MissingAdmissibleCurrent, "cableAdmissible"));
			QCOMPARE(r.cable, CheckOutcome::NotPerformed);
		}

		void rulesAreCited()
		{
			QCOMPARE(rules::pvAcLine().id, std::string("REBT-FV-007"));
			QCOMPARE(rules::pvAcLine().source, std::string("ITC-BT-40"));
			QVERIFY(rules::pvStringProtection().limitations.find("not binding")
					!= std::string::npos);
			QCOMPARE(rules::pvDcVoltageDrop().section, std::string("5.5.2"));
		}
};

QTEST_GUILESS_MAIN(tst_rebtpv)
#include "tst_rebtpv.moc"
