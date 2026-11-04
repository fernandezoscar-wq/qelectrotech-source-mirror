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
#include "rebt/rebtshortcircuit.h"

#include <QtTest>

#include <algorithm>

using namespace rebt;

namespace {

bool hasIssue(const std::vector<Diagnostic> &list, Issue issue, const std::string &field)
{
	return std::find(list.begin(), list.end(), Diagnostic{issue, field}) != list.end();
}

bool assumes(const ShortCircuitResult &r, Assumption a)
{
	return std::find(r.assumptions.begin(), r.assumptions.end(), a)
		   != r.assumptions.end();
}

bool cites(const ShortCircuitResult &r, const std::string &id)
{
	return std::any_of(r.rules.begin(), r.rules.end(),
					   [&](const NormRef &n) { return n.id == id; });
}

LineStretch line(double length, double section, std::optional<double> neutral,
				 Insulation insulation, bool interior)
{
	LineStretch l;
	l.length = length;
	l.section = section;
	l.neutralSection = neutral;
	l.material = Material::Copper;
	l.insulation = insulation;
	l.interior = interior;
	return l;
}

/// Case G of docs/rebt/refs_h7.py: LGA, DI and a 2.5 mm2 C16 circuit
ShortCircuitInput caseG()
{
	ShortCircuitInput in;
	in.source.method = SourceMethod::GuideFactor;
	in.phaseVoltage = 230;
	in.feeders.push_back(line(25, 95, 50, Insulation::XLPE, false));
	in.feeders.push_back(line(15, 10, 10, Insulation::XLPE, false));
	in.circuit = line(20, 2.5, std::nullopt, Insulation::PVC, true);
	in.breakingCapacity = 6000;
	in.ratedCurrent = 16;
	in.standard = ProtectionStandard::EN60898;
	in.curve = TripCurve::C;
	return in;
}

/// Case H: one long DI, the protection clears in the instantaneous range
ShortCircuitInput caseH()
{
	ShortCircuitInput in = caseG();
	in.feeders.clear();
	in.feeders.push_back(line(120, 10, 10, Insulation::XLPE, false));
	in.circuit = line(10, 2.5, std::nullopt, Insulation::PVC, true);
	return in;
}

/// Case K: Icc at the origin known
ShortCircuitInput caseK()
{
	ShortCircuitInput in;
	in.source.method = SourceMethod::KnownCurrent;
	in.source.maxCurrent = 10000;
	in.source.minCurrent = 6000;
	in.phaseVoltage = 230;
	LineStretch f = line(30, 16, 16, Insulation::XLPE, true);
	f.reactance = 0.08;
	in.feeders.push_back(f);
	in.circuit = line(25, 4, 4, Insulation::XLPE, true);
	in.circuit.reactance = 0.08;
	in.breakingCapacity = 10000;
	in.ratedCurrent = 32;
	in.standard = ProtectionStandard::EN60898;
	in.curve = TripCurve::C;
	return in;
}

} // namespace

/**
	Reference values from docs/rebt/refs_h7.py, written apart from the
	program from the formulas of the sources.
*/
class tst_rebtshortcircuit : public QObject
{
		Q_OBJECT

	private slots:
		void kFactors()
		{
				// UNE-HD 60364-4-43 table 43A
			QCOMPARE(*kFactor(Material::Copper, Insulation::PVC, 2.5), 115.0);
			QCOMPARE(*kFactor(Material::Copper, Insulation::PVC, 300), 115.0);
			QCOMPARE(*kFactor(Material::Copper, Insulation::PVC, 400), 103.0);
			QCOMPARE(*kFactor(Material::Copper, Insulation::XLPE, 4), 143.0);
			QCOMPARE(*kFactor(Material::Copper, Insulation::EPR, 4), 143.0);
			QCOMPARE(*kFactor(Material::Aluminium, Insulation::PVC, 16), 76.0);
			QCOMPARE(*kFactor(Material::Aluminium, Insulation::PVC, 400), 68.0);
			QCOMPARE(*kFactor(Material::Aluminium, Insulation::XLPE, 16), 94.0);
			QVERIFY(!kFactor(Material::Copper, Insulation::PVC, 0));
			QCOMPARE(finalTemperature(Insulation::PVC, 2.5), 160.0);
			QCOMPARE(finalTemperature(Insulation::PVC, 400), 140.0);
			QCOMPARE(finalTemperature(Insulation::XLPE, 2.5), 250.0);
		}

		void tripFactors()
		{
			QCOMPARE(*tripFactor(ProtectionStandard::EN60898, TripCurve::B), 5.0);
			QCOMPARE(*tripFactor(ProtectionStandard::EN60898, TripCurve::C), 10.0);
			QCOMPARE(*tripFactor(ProtectionStandard::EN61009, TripCurve::D), 20.0);
				// no range set by these standards
			QVERIFY(!tripFactor(ProtectionStandard::EN60947_2, TripCurve::C));
			QVERIFY(!tripFactor(ProtectionStandard::FuseGG, TripCurve::C));
		}

		/// The guide's own example, at its rounded resistivity 0.018
		void guideExample()
		{
			ShortCircuitInput in;
			in.source.method = SourceMethod::GuideFactor;
			in.phaseVoltage = 230;
				// a conductor whose resistivity at 20 degC is 0.018 has
				// the section of copper scaled by (1/58) / 0.018
			const double scale = (1.0 / 58.0) / 0.018;
			in.feeders.push_back(line(25, 95 * scale, std::nullopt,
									  Insulation::XLPE, false));
			in.feeders.push_back(line(15, 10 * scale, std::nullopt,
									  Insulation::XLPE, false));
			in.circuit = line(1, 2.5, std::nullopt, Insulation::PVC, true);
			const auto r = checkShortCircuit(in);
			QVERIFY(r.maxCurrent);
				// Guia-BT Anexo 3 prints 2898 A (R rounded to 0.0635 ohm)
			QVERIFY(qAbs(*r.maxCurrent - 2898.839) < 0.01);
			QVERIFY(assumes(r, Assumption::NeutralAsPhase));
		}

		void caseGuide()
		{
			const auto r = checkShortCircuit(caseG());
			QVERIFY(qAbs(*r.maxCurrent - 2835.916084) < 1e-5);
			QVERIFY(qAbs(*r.minCurrent - 331.580324) < 1e-5);
			QCOMPARE(*r.tripCurrent, 160.0);
			QCOMPARE(*r.k, 115.0);
			QVERIFY(qAbs(*r.allowedTime - 0.010278) < 1e-6);
			QVERIFY(qAbs(*r.conductorEnergy - 82656.25) < 1e-6);
			QCOMPARE(r.breaking, CheckOutcome::Passed);
			QCOMPARE(r.trip, CheckOutcome::Passed);
				// (k S / Imax)^2 under 0.1 s: the I2t is needed
			QCOMPARE(r.thermal, CheckOutcome::NotPerformed);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingLetThrough, "letThrough"));
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			QVERIFY(cites(r, "REBT-ICC-001"));
			QVERIFY(cites(r, "REBT-ICC-003"));
			QVERIFY(cites(r, "REBT-ICC-004"));
			QVERIFY(cites(r, "REBT-ICC-005"));
			QVERIFY(assumes(r, Assumption::GuideSourceFactor));
			QVERIFY(assumes(r, Assumption::ResistanceAtFinalTemperature));
				// circuit neutral: interior, ITC-BT-19 2.2.2
			QVERIFY(assumes(r, Assumption::NeutralAsPhase));
			QVERIFY(assumes(r, Assumption::ReactanceNeglected));
		}

		void letThroughDecides()
		{
			ShortCircuitInput in = caseG();
			in.letThrough = 20000;
			in.letThroughJustification = "catalogue curve, 3 kA";
			auto r = checkShortCircuit(in);
			QCOMPARE(r.thermal, CheckOutcome::Passed);
			QCOMPARE(*r.letThrough, 20000.0);
			QCOMPARE(r.outcome, CheckOutcome::Passed);
			QCOMPARE(r.status, Status::Ok);
				// equal to k2 S2 still passes
			in.letThrough = 82656.25;
			QCOMPARE(checkShortCircuit(in).thermal, CheckOutcome::Passed);
			in.letThrough = 82656.25 * 1.0001;
			r = checkShortCircuit(in);
			QCOMPARE(r.thermal, CheckOutcome::NotPassed);
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
				// a value with no source is not used
			in.letThroughJustification = " ";
			r = checkShortCircuit(in);
			QCOMPARE(r.thermal, CheckOutcome::NotPerformed);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingJustification, "letThrough"));
		}

		void letThroughAtTestCurrent()
		{
				// Icc max of case G 2835.916084 A (refs_h7.py)
			ShortCircuitInput in = caseG();
			in.letThrough = 20000;
			in.letThroughJustification = "catalogue curve";
				// given at 3 kA, over the Icc max: used
			in.letThroughAt = 3000;
			auto r = checkShortCircuit(in);
			QCOMPARE(r.thermal, CheckOutcome::Passed);
			QCOMPARE(*r.letThrough, 20000.0);
			QVERIFY(!hasIssue(r.diagnostics, Issue::LetThroughBelowFault, "letThrough"));
				// given at the Icc max itself: used
			in.letThroughAt = 2835.916084;
			QCOMPARE(checkShortCircuit(in).thermal, CheckOutcome::Passed);
				// given at 2.8 kA, under the Icc max: not valid at that fault
			in.letThroughAt = 2800;
			r = checkShortCircuit(in);
			QCOMPARE(r.thermal, CheckOutcome::NotPerformed);
			QVERIFY(!r.letThrough);
			QVERIFY(hasIssue(r.diagnostics, Issue::LetThroughBelowFault, "letThrough"));
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
				// a test current of zero is not a value
			in.letThroughAt = 0;
			r = checkShortCircuit(in);
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			QVERIFY(!r.letThrough);
			QVERIFY(hasIssue(r.diagnostics, Issue::NotPositive, "letThroughAt"));
			QVERIFY(!hasIssue(r.diagnostics, Issue::LetThroughBelowFault, "letThrough"));
				// Icc max not known: the I2t cannot be compared, not used
			in.letThroughAt = 3000;
			in.source.method.reset();
			r = checkShortCircuit(in);
			QVERIFY(!r.maxCurrent);
			QVERIFY(!r.letThrough);
			QVERIFY(!hasIssue(r.diagnostics, Issue::LetThroughBelowFault, "letThrough"));
		}

		void hotLoopNeverAssumes()
		{
				// 25 m 2.5/2.5 mm2 Cu PVC: 25 x 0.8 x 1.56 / 58 = 0.537931 ohm
			LineStretch l = line(25, 2.5, 2.5, Insulation::PVC, true);
			LoopImpedanceResult z = hotLoopImpedance({l});
			QVERIFY(qAbs(*z.impedance - 0.537931034) < 1e-8);
				// the second conductor is not taken as the phase, even interior
			l.neutralSection.reset();
			z = hotLoopImpedance({l});
			QVERIFY(!z.impedance);
			QVERIFY(hasIssue(z.diagnostics, Issue::MissingNeutralSection, "neutralSection"));
		}

		void instantaneousRange()
		{
			const auto r = checkShortCircuit(caseH());
			QVERIFY(qAbs(*r.maxCurrent - 444.666667) < 1e-5);
			QVERIFY(qAbs(*r.minCurrent - 182.240437) < 1e-5);
			QVERIFY(qAbs(*r.allowedTime - 0.418029) < 1e-6);
			QCOMPARE(r.trip, CheckOutcome::Passed);
			QCOMPARE(r.thermal, CheckOutcome::Passed);
			QVERIFY(assumes(r, Assumption::InstantaneousTripTime));
			QCOMPARE(r.outcome, CheckOutcome::Passed);
			QCOMPARE(r.status, Status::Ok);
		}

		void tripNotReached()
		{
			ShortCircuitInput in = caseH();
				// C20: 200 A > 182.2 A
			in.ratedCurrent = 20;
			auto r = checkShortCircuit(in);
			QCOMPARE(r.trip, CheckOutcome::NotPassed);
			QCOMPARE(r.thermal, CheckOutcome::NotPerformed);
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
				// B: 5 x 20 = 100 A
			in.curve = TripCurve::B;
			r = checkShortCircuit(in);
			QCOMPARE(*r.tripCurrent, 100.0);
			QCOMPARE(r.trip, CheckOutcome::Passed);
		}

		void breakingTooLow()
		{
			ShortCircuitInput in = caseG();
			in.breakingCapacity = 2835.0;
			auto r = checkShortCircuit(in);
			QCOMPARE(r.breaking, CheckOutcome::NotPassed);
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);
			in.breakingCapacity = 2836.0;
			QCOMPARE(checkShortCircuit(in).breaking, CheckOutcome::Passed);
		}

		/// "mayor o igual": equal values pass both checks
		void equalValuesPass()
		{
			ShortCircuitInput in = caseH();
			const auto first = checkShortCircuit(in);
			in.breakingCapacity = *first.maxCurrent;
			in.standard = ProtectionStandard::EN60947_2;
			in.tripCurrent = *first.minCurrent;
			in.tripCurrentJustification = "setting equal to the fault";
			const auto r = checkShortCircuit(in);
			QCOMPARE(r.breaking, CheckOutcome::Passed);
			QCOMPARE(r.trip, CheckOutcome::Passed);
		}

		/// No minimum current: the trip is unknown, so is the thermal check
		void noMinimumNoThermalPass()
		{
			ShortCircuitInput in = caseH();
			in.feeders[0].neutralSection.reset();
			const auto r = checkShortCircuit(in);
			QVERIFY(r.maxCurrent);
			QVERIFY(r.allowedTime && *r.allowedTime > 0.1);
			QVERIFY(!r.minCurrent);
			QCOMPARE(r.trip, CheckOutcome::NotPerformed);
			QCOMPARE(r.thermal, CheckOutcome::NotPerformed);
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
		}

		void knownSource()
		{
			const auto r = checkShortCircuit(caseK());
			QVERIFY(qAbs(*r.maxCurrent - 5797.161839) < 1e-5);
			QVERIFY(qAbs(*r.minCurrent - 399.072526) < 1e-5);
			QVERIFY(qAbs(*r.allowedTime - 0.009736) < 1e-6);
			QCOMPARE(*r.k, 143.0);
			QCOMPARE(r.breaking, CheckOutcome::Passed);
			QCOMPARE(r.trip, CheckOutcome::Passed);   // 399 >= 320
			QVERIFY(cites(r, "REBT-ICC-002"));
			QVERIFY(!cites(r, "REBT-ICC-001"));
			QVERIFY(assumes(r, Assumption::SourceCurrentGiven));
			QVERIFY(!assumes(r, Assumption::ReactanceNeglected));
			QVERIFY(!assumes(r, Assumption::NeutralAsPhase));
		}

		void knownSourceNeedsBothCurrents()
		{
			ShortCircuitInput in = caseK();
			in.source.minCurrent.reset();
			auto r = checkShortCircuit(in);
			QVERIFY(r.maxCurrent);
			QVERIFY(!r.minCurrent);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingSourceCurrent,
							 "sourceMinCurrent"));
			QCOMPARE(r.trip, CheckOutcome::NotPerformed);
				// minimum above maximum
			in.source.minCurrent = 12000;
			r = checkShortCircuit(in);
			QCOMPARE(r.status, Status::InvalidInput);
			QVERIFY(hasIssue(r.diagnostics, Issue::OutOfRange, "sourceMinCurrent"));
			QCOMPARE(r.breaking, CheckOutcome::NotPerformed);
		}

		void nothingInvented()
		{
			ShortCircuitInput in;
			const auto r = checkShortCircuit(in);
			QVERIFY(!r.maxCurrent);
			QVERIFY(!r.minCurrent);
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingSource, "source"));
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingVoltage, "voltage"));
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingBreakingCapacity,
							 "breakingCapacity"));
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingProtectionStandard,
							 "protectionStandard"));
		}

		void guideNeedsALine()
		{
			ShortCircuitInput in = caseG();
			in.feeders.clear();
			const auto r = checkShortCircuit(in);
			QVERIFY(!r.maxCurrent);
			QVERIFY(hasIssue(r.diagnostics, Issue::NoSourceImpedance, "feeders"));
			QCOMPARE(r.breaking, CheckOutcome::NotPerformed);
				// the minimum only needs the circuit
			QVERIFY(r.minCurrent);
		}

		void neutralOutsideInterior()
		{
			ShortCircuitInput in = caseG();
			in.feeders[1].neutralSection.reset();
				// maximum: equal neutral is the upper bound
			const auto r = checkShortCircuit(in);
			QVERIFY(qAbs(*r.maxCurrent - 2835.916084) < 1e-5);
				// minimum: a DI neutral is not set by ITC-BT-19
			QVERIFY(!r.minCurrent);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingNeutralSection,
							 "feeder[1]/neutralSection"));
		}

		void otherDevicesNeedTheirTripCurrent()
		{
			ShortCircuitInput in = caseH();
			in.standard = ProtectionStandard::EN60947_2;
			auto r = checkShortCircuit(in);
			QCOMPARE(r.trip, CheckOutcome::NotPerformed);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingTripCurrent, "tripCurrent"));
			in.tripCurrent = 150;
			in.tripCurrentJustification = "Im = 150 A, setting of the breaker";
			r = checkShortCircuit(in);
			QCOMPARE(r.trip, CheckOutcome::Passed);
				// no standard time: the I2t is needed even with t >= 0.1 s
			QCOMPARE(r.thermal, CheckOutcome::NotPerformed);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingLetThrough, "letThrough"));
			in.curve.reset();
			in.standard = ProtectionStandard::EN60898;
			in.tripCurrent.reset();
			r = checkShortCircuit(in);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingTripCurrent, "curve"));
		}

		void notAShortCircuitDevice()
		{
			ShortCircuitInput in = caseG();
			in.protectsAgainstShortCircuit = false;
			const auto r = checkShortCircuit(in);
			QVERIFY(hasIssue(r.diagnostics, Issue::NoShortCircuitProtection,
							 "protectionType"));
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
		}

		void invalidValues()
		{
			ShortCircuitInput in = caseG();
			in.circuit.length = -3;
			auto r = checkShortCircuit(in);
			QCOMPARE(r.status, Status::InvalidInput);
			QVERIFY(!r.minCurrent);
			QVERIFY(hasIssue(r.diagnostics, Issue::NotPositive, "length"));
			in = caseG();
			in.feeders[0].reactance = -0.1;
			r = checkShortCircuit(in);
			QCOMPARE(r.status, Status::InvalidInput);
			QVERIFY(hasIssue(r.diagnostics, Issue::Negative, "feeder[0]/reactance"));
			in = caseG();
			in.breakingCapacity = std::numeric_limits<double>::quiet_NaN();
			r = checkShortCircuit(in);
			QVERIFY(hasIssue(r.diagnostics, Issue::NotFinite, "breakingCapacity"));
		}

		void missingInsulation()
		{
			ShortCircuitInput in = caseG();
			in.circuit.insulation.reset();
			const auto r = checkShortCircuit(in);
			QVERIFY(r.maxCurrent);
			QVERIFY(!r.minCurrent);
			QVERIFY(!r.k);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingInsulation, "insulation"));
		}
};

QTEST_GUILESS_MAIN(tst_rebtshortcircuit)
#include "tst_rebtshortcircuit.moc"
