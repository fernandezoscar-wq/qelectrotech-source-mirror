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
#include "rebt/rebtindirect.h"

#include <QtTest>

#include <algorithm>

using namespace rebt;

namespace {

bool hasIssue(const IndirectContactResult &r, Issue issue, const std::string &field)
{
	return std::find(r.diagnostics.begin(), r.diagnostics.end(), Diagnostic{issue, field})
		   != r.diagnostics.end();
}

bool assumes(const IndirectContactResult &r, Assumption a)
{
	return std::find(r.assumptions.begin(), r.assumptions.end(), a) != r.assumptions.end();
}

bool cites(const IndirectContactResult &r, const std::string &id)
{
	return std::any_of(r.rules.begin(), r.rules.end(),
					   [&](const NormRef &n) { return n.id == id; });
}

/// TT, RA 30 ohm measured, a 30 mA residual current device
IndirectContactInput ttWithRcd()
{
	IndirectContactInput in;
	in.earthing = EarthingSystem::TT;
	in.earthResistance = 30.0;
	in.earthResistanceJustification = "medida";
	in.residualCurrent = 0.030;
	return in;
}

/// Circuit of 25 m 2.5 mm2 Cu PVC, PE 2.5 mm2, C16
LineStretch circuit25()
{
	LineStretch c;
	c.length = 25.0;
	c.section = 2.5;
	c.protectiveSection = 2.5;
	c.material = Material::Copper;
	c.insulation = Insulation::PVC;
	c.interior = true;
	return c;
}

/// TN, Icc min 5 kA at the origin, U0 230 V, C16 breaker
IndirectContactInput tnCase()
{
	IndirectContactInput in;
	in.earthing = EarthingSystem::TN;
	in.phaseVoltage = 230.0;
	in.source.method = SourceMethod::KnownCurrent;
	in.source.maxCurrent = 6000.0;
	in.source.minCurrent = 5000.0;
	in.circuit = circuit25();
	in.ratedCurrent = 16.0;
	in.standard = ProtectionStandard::EN60898;
	in.curve = TripCurve::C;
	return in;
}

/// IT: RA 20 ohm, Id 0.5 A, U0 230 V, C16 breaker
IndirectContactInput itCase()
{
	IndirectContactInput in = tnCase();
	in.earthing = EarthingSystem::IT;
	in.source = SourceData{};
	in.earthResistance = 20.0;
	in.earthResistanceJustification = "medida";
	in.firstFaultCurrent = 0.5;
	in.firstFaultJustification = "fabricante del transformador";
	return in;
}

} // namespace

/**
	Reference values worked out by hand from ITC-BT-24 4.1, with
	rho 1/58, R(theta) = [1 + 0.004 (theta - 20)] R20 (160 degC PVC,
	250 degC XLPE): TT RA x Ia, TN Zs x Ia, IT RA x Id and 2 Zs x Ia.
*/
class tst_rebtindirect : public QObject
{
		Q_OBJECT

	private slots:
		void residualDevice()
		{
			const auto r = checkIndirectContact(ttWithRcd());
			QCOMPARE(r.status, Status::Ok);
			QCOMPARE(*r.operatingCurrent, 0.030);
			QCOMPARE(*r.limitVoltage, 50.0);
				// 30 x 0.030 = 0.9 V; 50 / 0.030 = 1666.666667 ohm
			QVERIFY(qAbs(*r.contactVoltage - 0.9) < 1e-12);
			QVERIFY(qAbs(*r.maxResistance - 1666.666667) < 1e-6);
			QCOMPARE(r.outcome, CheckOutcome::Passed);
			QVERIFY(assumes(r, Assumption::NormalLimitVoltage));
			QVERIFY(assumes(r, Assumption::ResidualOperatingCurrent));
			QVERIFY(assumes(r, Assumption::EarthResistanceGiven));
			QVERIFY(cites(r, "REBT-CIN-001"));
			QVERIFY(r.diagnostics.empty());
		}

		void boundary()
		{
			IndirectContactInput in = ttWithRcd();
				// 1666.666667 x 0.030 = 50.00000001: over 50 by less than
				// the tolerance of the program
			in.earthResistance = 50.0 / 0.030;
			QCOMPARE(checkIndirectContact(in).outcome, CheckOutcome::Passed);
				// 1667 x 0.030 = 50.01 V
			in.earthResistance = 1667.0;
			const auto r = checkIndirectContact(in);
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
			QVERIFY(qAbs(*r.contactVoltage - 50.01) < 1e-9);
				// 300 mA: 167 x 0.3 = 50.1 V, 166 x 0.3 = 49.8 V
			in.residualCurrent = 0.3;
			in.earthResistance = 167.0;
			QCOMPARE(checkIndirectContact(in).outcome, CheckOutcome::NotPassed);
			in.earthResistance = 166.0;
			QCOMPARE(checkIndirectContact(in).outcome, CheckOutcome::Passed);
		}

		void lowerLimitVoltage()
		{
			IndirectContactInput in = ttWithRcd();
			in.limitVoltage = 24.0;
			in.limitVoltageJustification = "local mojado";
				// 800 x 0.030 = 24 V; 801 x 0.030 = 24.03 V
			in.earthResistance = 800.0;
			auto r = checkIndirectContact(in);
			QCOMPARE(r.outcome, CheckOutcome::Passed);
			QCOMPARE(*r.limitVoltage, 24.0);
			QVERIFY(!assumes(r, Assumption::NormalLimitVoltage));
			QVERIFY(qAbs(*r.maxResistance - 800.0) < 1e-9);
			in.earthResistance = 801.0;
			QCOMPARE(checkIndirectContact(in).outcome, CheckOutcome::NotPassed);
				// with no reason it is not used, and 50 V is not taken instead
			in.limitVoltageJustification = " ";
			r = checkIndirectContact(in);
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			QVERIFY(!r.limitVoltage);
			QVERIFY(hasIssue(r, Issue::MissingJustification, "limitVoltage"));
				// over 50 V: not the normal conditions of the instruction
			in.limitVoltage = 60.0;
			in.limitVoltageJustification = "x";
			r = checkIndirectContact(in);
			QCOMPARE(r.status, Status::InvalidInput);
			QVERIFY(hasIssue(r, Issue::LimitAboveNormal, "limitVoltage"));
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
				// 50 V given is allowed: 24.03 V passes
			in.limitVoltage = 50.0;
			QCOMPARE(checkIndirectContact(in).outcome, CheckOutcome::Passed);
		}

		void overcurrentDevice()
		{
			IndirectContactInput in = ttWithRcd();
			in.residualCurrent.reset();
			in.ratedCurrent = 16.0;
			in.standard = ProtectionStandard::EN60898;
			in.curve = TripCurve::C;
				// Ia = 10 x 16 = 160 A; 50 / 160 = 0.3125 ohm
			in.earthResistance = 0.3125;
			auto r = checkIndirectContact(in);
			QCOMPARE(*r.operatingCurrent, 160.0);
			QCOMPARE(r.outcome, CheckOutcome::Passed);
			QVERIFY(assumes(r, Assumption::TripCurrentFromCurve));
			QVERIFY(!assumes(r, Assumption::ResidualOperatingCurrent));
			QVERIFY(cites(r, "REBT-ICC-004"));
				// 30 ohm: 4800 V
			in.earthResistance = 30.0;
			r = checkIndirectContact(in);
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
			QVERIFY(qAbs(*r.contactVoltage - 4800.0) < 1e-9);
				// curve B: 5 x 16 = 80 A
			in.curve = TripCurve::B;
			QCOMPARE(*checkIndirectContact(in).operatingCurrent, 80.0);
				// a given instantaneous trip goes before the curve
			in.tripCurrent = 200.0;
			in.tripCurrentJustification = "catálogo";
			QCOMPARE(*checkIndirectContact(in).operatingCurrent, 200.0);
			in.tripCurrentJustification.clear();
			r = checkIndirectContact(in);
			QVERIFY(!r.operatingCurrent);
			QVERIFY(hasIssue(r, Issue::MissingJustification, "tripCurrent"));
				// UNE-EN 60947-2: no curve of the standard
			in.tripCurrent.reset();
			in.standard = ProtectionStandard::EN60947_2;
			r = checkIndirectContact(in);
			QVERIFY(!r.operatingCurrent);
			QVERIFY(hasIssue(r, Issue::MissingOperatingCurrent, "operatingCurrent"));
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
				// the residual current device goes before the breaker
			in.residualCurrent = 0.030;
			QCOMPARE(*checkIndirectContact(in).operatingCurrent, 0.030);
		}

		void missingData()
		{
			IndirectContactInput in = ttWithRcd();
			in.earthResistance.reset();
			auto r = checkIndirectContact(in);
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			QCOMPARE(r.status, Status::NotComputable);
			QVERIFY(hasIssue(r, Issue::MissingEarthResistance, "earthResistance"));
				// what RA may be is still said
			QVERIFY(qAbs(*r.maxResistance - 1666.666667) < 1e-6);
			in.earthResistance = 30.0;
			in.earthResistanceJustification.clear();
			r = checkIndirectContact(in);
			QVERIFY(hasIssue(r, Issue::MissingJustification, "earthResistance"));
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			in = ttWithRcd();
			in.earthResistance = -1.0;
			r = checkIndirectContact(in);
			QCOMPARE(r.status, Status::InvalidInput);
			QVERIFY(hasIssue(r, Issue::NotPositive, "earthResistance"));
			QVERIFY(!r.maxResistance);
			QVERIFY(!r.operatingCurrent);
			QVERIFY(!r.limitVoltage);
			in = ttWithRcd();
			in.residualCurrent = 0.0;
			r = checkIndirectContact(in);
			QCOMPARE(r.status, Status::InvalidInput);
			QVERIFY(hasIssue(r, Issue::NotPositive, "residualCurrent"));
			QVERIFY(!hasIssue(r, Issue::MissingOperatingCurrent, "operatingCurrent"));
				// a breaker of IN not valid: said once, not as missing
			in = ttWithRcd();
			in.residualCurrent.reset();
			in.standard = ProtectionStandard::EN60898;
			in.curve = TripCurve::C;
			in.ratedCurrent = -16.0;
			r = checkIndirectContact(in);
			QCOMPARE(r.status, Status::InvalidInput);
			QVERIFY(hasIssue(r, Issue::NotPositive, "ratedCurrent"));
			QVERIFY(!hasIssue(r, Issue::MissingOperatingCurrent, "operatingCurrent"));
		}

		void missingEarthing()
		{
			IndirectContactInput in = ttWithRcd();
			in.earthing.reset();
			const auto r = checkIndirectContact(in);
			QVERIFY(hasIssue(r, Issue::MissingEarthing, "earthing"));
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			QVERIFY(r.rules.empty());
		}

		void tnKnownSource()
		{
				// Zs = 230 / 5000 + 0.537931 = 0.583931 ohm; x 160 A = 93.43 V
			IndirectContactInput in = tnCase();
			auto r = checkIndirectContact(in);
			QCOMPARE(r.status, Status::Ok);
			QVERIFY(qAbs(*r.loopImpedance - 0.583931034) < 1e-8);
			QCOMPARE(*r.operatingCurrent, 160.0);
			QVERIFY(qAbs(*r.loopVoltage - 93.428966) < 1e-5);
			QCOMPARE(*r.loopLimit, 230.0);
			QVERIFY(qAbs(*r.maxImpedance - 1.4375) < 1e-12);
			QCOMPARE(r.outcome, CheckOutcome::Passed);
			QVERIFY(cites(r, "REBT-CIN-002"));
			QVERIFY(!cites(r, "REBT-CIN-001"));
			QVERIFY(assumes(r, Assumption::SourceCurrentGiven));
			QVERIFY(assumes(r, Assumption::ResistanceAtFinalTemperature));
			QVERIFY(assumes(r, Assumption::ReactanceNeglected));
			QVERIFY(assumes(r, Assumption::InstantaneousWithinTable));
				// no RA is asked for in TN
			QVERIFY(!hasIssue(r, Issue::MissingEarthResistance, "earthResistance"));
				// 70 m: 0.046 + 1.506207 = 1.552207 ohm, 248.35 V
			in.circuit.length = 70.0;
			r = checkIndirectContact(in);
			QVERIFY(qAbs(*r.loopVoltage - 248.353103) < 1e-5);
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
				// a 30 mA residual current device: 230 / 0.03 = 7666.67 ohm
			in.residualCurrent = 0.030;
			r = checkIndirectContact(in);
			QCOMPARE(r.outcome, CheckOutcome::Passed);
			QVERIFY(qAbs(*r.maxImpedance - 7666.666667) < 1e-5);
			QVERIFY(!assumes(r, Assumption::InstantaneousWithinTable));
		}

		void tnGuideFactor()
		{
				// feeder 15 m 10/10 XLPE 0.099310 + circuit 0.537931,
				// / 0.8 = 0.796552 ohm; x 160 A = 127.45 V
			IndirectContactInput in = tnCase();
			in.source = SourceData{};
			in.source.method = SourceMethod::GuideFactor;
			LineStretch f;
			f.length = 15.0;
			f.section = 10.0;
			f.protectiveSection = 10.0;
			f.material = Material::Copper;
			f.insulation = Insulation::XLPE;
			in.feeders.push_back(f);
			auto r = checkIndirectContact(in);
			QVERIFY(qAbs(*r.loopImpedance - 0.796551724) < 1e-8);
			QVERIFY(qAbs(*r.loopVoltage - 127.448276) < 1e-5);
			QCOMPARE(r.outcome, CheckOutcome::Passed);
			QVERIFY(assumes(r, Assumption::GuideLoop));
			QVERIFY(cites(r, "REBT-ICC-001"));
				// the PE of the feeder is needed, never assumed
			in.feeders[0].protectiveSection.reset();
			r = checkIndirectContact(in);
			QVERIFY(!r.loopImpedance);
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			QVERIFY(hasIssue(r, Issue::MissingProtectiveSection, "feeder[0]/protectiveSection"));
				// said once, not also as a missing neutral
			QVERIFY(!hasIssue(r, Issue::MissingNeutralSection, "feeder[0]/protectiveSection"));
			QVERIFY(!hasIssue(r, Issue::MissingNeutralSection, "feeder[0]/neutralSection"));
				// a PE of zero is not a value, and is named as the PE
			in.feeders[0].protectiveSection = 0.0;
			r = checkIndirectContact(in);
			QCOMPARE(r.status, Status::InvalidInput);
			QVERIFY(hasIssue(r, Issue::NotPositive, "feeder[0]/protectiveSection"));
		}

		void tnMissingData()
		{
			IndirectContactInput in = tnCase();
			in.source = SourceData{};
			auto r = checkIndirectContact(in);
			QVERIFY(hasIssue(r, Issue::MissingSource, "source"));
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
				// the largest Zs is still said
			QVERIFY(qAbs(*r.maxImpedance - 1.4375) < 1e-12);
			in = tnCase();
			in.source.minCurrent.reset();
			r = checkIndirectContact(in);
			QVERIFY(hasIssue(r, Issue::MissingSourceCurrent, "sourceMinCurrent"));
			in = tnCase();
			in.phaseVoltage.reset();
			r = checkIndirectContact(in);
			QVERIFY(hasIssue(r, Issue::MissingVoltage, "voltage"));
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			in = tnCase();
			in.circuit.protectiveSection.reset();
			r = checkIndirectContact(in);
			QVERIFY(hasIssue(r, Issue::MissingProtectiveSection, "protectiveSection"));
				// the chain was not followed: nothing computed
			in = tnCase();
			in.pathKnown = false;
			r = checkIndirectContact(in);
			QVERIFY(!r.loopImpedance);
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
				// and the origin is not asked for: the caller says why
			in.source = SourceData{};
			r = checkIndirectContact(in);
			QVERIFY(!hasIssue(r, Issue::MissingSource, "source"));
		}

		void tncWithoutResidualDevices()
		{
			IndirectContactInput in = tnCase();
			in.earthing = EarthingSystem::TNC;
				// TN-C with its breaker: as TN
			QCOMPARE(checkIndirectContact(in).outcome, CheckOutcome::Passed);
				// with a residual current device: not allowed
			in.residualCurrent = 0.030;
			const auto r = checkIndirectContact(in);
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
			QVERIFY(hasIssue(r, Issue::ResidualInTNC, "residualDevice"));
			QVERIFY(cites(r, "REBT-CIN-002"));
		}

		void itFirstFault()
		{
				// RA 20 ohm, Id 0.5 A: 10 V; Id 3 A: 60 V
			IndirectContactInput in = itCase();
			auto r = checkIndirectContact(in);
			QVERIFY(qAbs(*r.firstFaultVoltage - 10.0) < 1e-12);
			QCOMPARE(r.firstFault, CheckOutcome::Passed);
			QVERIFY(cites(r, "REBT-CIN-003"));
			QVERIFY(cites(r, "REBT-CIN-004"));
			in.firstFaultCurrent = 3.0;
			r = checkIndirectContact(in);
			QCOMPARE(r.firstFault, CheckOutcome::NotPassed);
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
				// 2.5 A: 50 V, the limit
			in.firstFaultCurrent = 2.5;
			QCOMPARE(checkIndirectContact(in).firstFault, CheckOutcome::Passed);
				// Id with no source is not used
			in.firstFaultJustification.clear();
			r = checkIndirectContact(in);
			QCOMPARE(r.firstFault, CheckOutcome::NotPerformed);
			QVERIFY(hasIssue(r, Issue::MissingJustification, "firstFaultCurrent"));
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			in.firstFaultCurrent.reset();
			r = checkIndirectContact(in);
			QVERIFY(hasIssue(r, Issue::MissingFirstFault, "firstFaultCurrent"));
		}

		void itMassesApart()
		{
				// second fault as TT: 20 x 0.030 = 0.6 V
			IndirectContactInput in = itCase();
			in.massesInterconnected = false;
			in.residualCurrent = 0.030;
			auto r = checkIndirectContact(in);
			QVERIFY(qAbs(*r.contactVoltage - 0.6) < 1e-12);
			QCOMPARE(r.secondFault, CheckOutcome::Passed);
			QCOMPARE(r.outcome, CheckOutcome::Passed);
			QCOMPARE(r.status, Status::Ok);
				// with the C16 breaker: 20 x 160 = 3200 V
			in.residualCurrent.reset();
			r = checkIndirectContact(in);
			QCOMPARE(r.secondFault, CheckOutcome::NotPassed);
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
				// second fault passed, first not known: not passed as a whole
			in.residualCurrent = 0.030;
			in.firstFaultCurrent.reset();
			r = checkIndirectContact(in);
			QCOMPARE(r.secondFault, CheckOutcome::Passed);
			QCOMPARE(r.firstFault, CheckOutcome::NotPerformed);
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			in.massesInterconnected.reset();
			r = checkIndirectContact(in);
			QVERIFY(hasIssue(r, Issue::MissingMassesArrangement, "massesInterconnected"));
			QCOMPARE(r.secondFault, CheckOutcome::NotPerformed);
		}

		void itMassesInterconnected()
		{
			IndirectContactInput in = itCase();
			in.massesInterconnected = true;
			auto r = checkIndirectContact(in);
			QVERIFY(hasIssue(r, Issue::MissingNeutralArrangement, "neutralDistributed"));
				// neutral not distributed: 2 x 0.537931 x 160 = 172.14 V
				// <= sqrt(3) x 230 = 398.37 V; largest Zs 398.37 / 320
			in.neutralDistributed = false;
			r = checkIndirectContact(in);
			QVERIFY(qAbs(*r.loopImpedance - 0.537931034) < 1e-8);
			QVERIFY(qAbs(*r.loopVoltage - 172.137931) < 1e-5);
			QVERIFY(qAbs(*r.loopLimit - 398.371686) < 1e-5);
			QVERIFY(qAbs(*r.maxImpedance - 1.244912) < 1e-5);
			QCOMPARE(r.secondFault, CheckOutcome::Passed);
			QCOMPARE(r.outcome, CheckOutcome::Passed);
			QVERIFY(assumes(r, Assumption::LoopFromItOrigin));
				// no source in the loop of IT
			QVERIFY(!assumes(r, Assumption::SourceCurrentGiven));
				// distributed, neutral as the phase (interior): limit U0 = 230 V
			in.neutralDistributed = true;
			r = checkIndirectContact(in);
			QCOMPARE(*r.loopLimit, 230.0);
			QCOMPARE(r.secondFault, CheckOutcome::Passed);
			QVERIFY(assumes(r, Assumption::NeutralAsPhase));
				// 50 m: 2 x 1.075862 x 160 = 344.28 V > 230 V
			in.circuit.length = 50.0;
			r = checkIndirectContact(in);
			QVERIFY(qAbs(*r.loopVoltage - 344.275862) < 1e-5);
			QCOMPARE(r.secondFault, CheckOutcome::NotPassed);
				// a neutral of 1.5 mm2: Z's 25 m (1/1.5 + 1/2.5) 1.56 / 58 = 0.717241,
				// 2 Z's x 160 = 229.52 V <= 230 V
			in.circuit.length = 25.0;
			in.circuit.neutralSection = 1.5;
			r = checkIndirectContact(in);
			QVERIFY(qAbs(*r.loopImpedance - 0.717241379) < 1e-8);
			QVERIFY(qAbs(*r.loopVoltage - 229.517241) < 1e-5);
			QCOMPARE(r.secondFault, CheckOutcome::Passed);
			in.circuit.neutralSection.reset();
			in.circuit.length = 50.0;
				// out of the interior installation the neutral is needed
			in.circuit.interior = false;
			r = checkIndirectContact(in);
			QVERIFY(hasIssue(r, Issue::MissingNeutralSection, "neutralSection"));
			QCOMPARE(r.secondFault, CheckOutcome::NotPerformed);
		}

		void rulesAreCited()
		{
			const NormRef tt = rules::indirectContactTT();
			QCOMPARE(tt.id, std::string("REBT-CIN-001"));
			QCOMPARE(tt.section, std::string("4.1, 4.1.2"));
			const NormRef tn = rules::indirectContactTN();
			QCOMPARE(tn.id, std::string("REBT-CIN-002"));
			QCOMPARE(tn.source, std::string("ITC-BT-24"));
			QVERIFY(tn.limitations.find("instantaneous trip") != std::string::npos);
			QCOMPARE(rules::indirectContactITFirst().id, std::string("REBT-CIN-003"));
			QCOMPARE(rules::indirectContactITSecond().id, std::string("REBT-CIN-004"));
			QVERIFY(rules::indirectContactITFirst().limitations.find("not worked out")
					!= std::string::npos);
		}
};

QTEST_GUILESS_MAIN(tst_rebtindirect)
#include "tst_rebtindirect.moc"
