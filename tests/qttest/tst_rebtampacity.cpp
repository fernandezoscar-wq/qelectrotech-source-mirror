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
#include "rebt/rebtampacity.h"
#include "rebt/rebtcalc.h"
#include "rebt/rebtrules.h"

#include <QtTest>

#include <algorithm>
#include <cmath>

using namespace rebt;

namespace {

/// Reference values worked out apart (docs/rebt/tablas_h4.md), 0.1 %
bool near(double value, double reference)
{
	return std::fabs(value - reference) <= 1e-3 * std::fabs(reference);
}

bool hasIssue(const std::vector<Diagnostic> &list, Issue issue, const std::string &field)
{
	return std::find(list.begin(), list.end(), Diagnostic{issue, field}) != list.end();
}

bool hasAssumption(const std::vector<Assumption> &list, Assumption a)
{
	return std::find(list.begin(), list.end(), a) != list.end();
}

/// SYNTHETIC values, round on purpose: not a table of any standard
AmpacityLibrary library()
{
	AmpacityLibrary lib;
	AmpacityTable pvc;
	pvc.id = "TEST-PVC2-B1";
	pvc.source = "synthetic test data";
	pvc.material = Material::Copper;
	pvc.insulation = Insulation::PVC;
	pvc.loadedConductors = 2;
	pvc.method = "B1";
	pvc.referenceAmbient = 30.0;
		// given out of order: the library sorts them
	pvc.entries = {{4, 30}, {1.5, 10}, {2.5, 20}, {10, 60}, {6, 40}};
	lib.add(pvc);

	AmpacityTable xlpe;
	xlpe.id = "TEST-XLPE3-E";
	xlpe.source = "synthetic test data";
	xlpe.material = Material::Copper;
	xlpe.insulation = Insulation::XLPE;
	xlpe.loadedConductors = 3;
	xlpe.method = "E";
	xlpe.referenceAmbient = 30.0;
	xlpe.entries = {{2.5, 25}, {4, 35}, {6, 45}};
	lib.add(xlpe);
	return lib;
}

AmpacityInput pvcInput(double section, double ambient)
{
	AmpacityInput in;
	in.system = System::SinglePhase;
	in.material = Material::Copper;
	in.insulation = Insulation::PVC;
	in.method = "B1";
	in.section = section;
	in.ambient = ambient;
	in.groupingFactor = 1.0;
	in.groupingJustification = "single circuit in its conduit";
	return in;
}

CircuitInput circuit16A(double length)
{
	CircuitInput c;
	c.system = System::SinglePhase;
	c.voltage = 230.0;
	c.power = 3680.0;
	c.powerFactor = 1.0;
	c.length = length;
	c.conductivity = 48.0;
	return c;
}

} // namespace

class tst_rebtampacity : public QObject
{
		Q_OBJECT

	private slots:
		void temperatureFactor()
		{
				// the formula gives the published factors once rounded
			QVERIFY(near(*rebt::temperatureFactor(Insulation::PVC, 40, 30), 0.866025));
			QVERIFY(near(*rebt::temperatureFactor(Insulation::XLPE, 40, 30), 0.912871));
			QVERIFY(near(*rebt::temperatureFactor(Insulation::PVC, 25, 30), 1.060660));
			QCOMPARE(*rebt::temperatureFactor(Insulation::EPR, 30, 30), 1.0);
			QVERIFY(!rebt::temperatureFactor(Insulation::PVC, 70, 30));
			QVERIFY(!rebt::temperatureFactor(Insulation::PVC, 75, 30));
			QCOMPARE(maxConductorTemperature(Insulation::PVC), 70.0);
			QCOMPARE(maxConductorTemperature(Insulation::XLPE), 90.0);
			QCOMPARE(maxConductorTemperature(Insulation::EPR), 90.0);
		}

		void libraryFindsByConditions()
		{
			const AmpacityLibrary lib = library();
			QCOMPARE(lib.tables().size(), size_t(2));
			QVERIFY(lib.find(Material::Copper, Insulation::PVC, 2, "B1"));
			QVERIFY(lib.find(Material::Copper, Insulation::PVC, 2, " B1 "));
			QVERIFY(!lib.find(Material::Copper, Insulation::PVC, 3, "B1"));
			QVERIFY(!lib.find(Material::Aluminium, Insulation::PVC, 2, "B1"));
			QVERIFY(!lib.find(Material::Copper, Insulation::XLPE, 2, "B1"));
				// sorted, and a second table for the same conditions refused
			const AmpacityTable *t = lib.find(Material::Copper, Insulation::PVC, 2, "B1");
			QCOMPARE(t->entries.front().section, 1.5);
			QCOMPARE(t->entries.back().section, 10.0);
			AmpacityLibrary copy = lib;
			QVERIFY(!copy.add(*t));
		}

		void admissibleWithComputedTemperature()
		{
				// case A
			const AmpacityResult r = admissibleCurrent(pvcInput(2.5, 40), library());
			QCOMPARE(r.status, Status::Ok);
			QCOMPARE(*r.tableCurrent, 20.0);
			QVERIFY(near(*r.admissible, 17.320508));
			QCOMPARE(r.tableId, std::string("TEST-PVC2-B1"));
			QCOMPARE(r.factors.size(), size_t(2));
			QVERIFY(r.factors.at(0).computed);
			QVERIFY(hasAssumption(r.assumptions, Assumption::TemperatureFactorComputed));
			QVERIFY(hasAssumption(r.assumptions, Assumption::LoadedConductorsFromSystem));
			QCOMPARE(r.rules.at(0).id, std::string("REBT-IZ-001"));
			QCOMPARE(r.rules.at(0).section, std::string("2.2.3"));
			QCOMPARE(r.rules.at(1).id, std::string("REBT-IZ-002"));
		}

		void admissibleWithGivenFactors()
		{
				// case C: 20 x 0.87 x 0.8
			AmpacityInput in = pvcInput(2.5, 40);
			in.temperatureFactor = 0.87;
			in.temperatureJustification = "table B.52.14, 40 degC";
			in.groupingFactor = 0.8;
			in.groupingJustification = "two circuits in one conduit";
			AmpacityResult r = admissibleCurrent(in, library());
			QCOMPARE(r.status, Status::Ok);
			QVERIFY(near(*r.admissible, 13.92));
			QVERIFY(!r.factors.at(0).computed);
			QVERIFY(!hasAssumption(r.assumptions, Assumption::TemperatureFactorComputed));
			QCOMPARE(r.factors.at(1).justification, std::string("two circuits in one conduit"));

				// case E: ambient at the reference, grouping 0.8, other 0.9
			in = pvcInput(2.5, 30);
			in.groupingFactor = 0.8;
			in.otherFactor = 0.9;
			in.otherJustification = "thermal insulation nearby";
			r = admissibleCurrent(in, library());
			QVERIFY(near(*r.admissible, 14.4));
			QCOMPARE(r.factors.size(), size_t(3));
		}

		void threePhaseTable()
		{
				// case D
			AmpacityInput in = pvcInput(4, 40);
			in.system = System::ThreePhase;
			in.insulation = Insulation::XLPE;
			in.method = "E";
			const AmpacityResult r = admissibleCurrent(in, library());
			QCOMPARE(r.status, Status::Ok);
			QVERIFY(near(*r.admissible, 31.950483));
				// loaded conductors given win over the system
			in.loadedConductors = 2;
			QVERIFY(hasIssue(admissibleCurrent(in, library()).diagnostics,
							 Issue::NoTable, "table"));
		}

		void missingData()
		{
			const AmpacityResult r = admissibleCurrent(AmpacityInput(), library());
			QCOMPARE(r.status, Status::NotComputable);
			QVERIFY(!r.admissible);
			for (const auto &e : std::vector<std::pair<Issue, std::string>>{
					 {Issue::MissingMaterial, "material"},
					 {Issue::MissingInsulation, "insulation"},
					 {Issue::MissingMethod, "installationMethod"},
					 {Issue::MissingLoadedConductors, "loadedConductors"},
					 {Issue::MissingAmbient, "ambientTemperature"},
					 {Issue::MissingGroupingFactor, "groupingFactor"},
					 {Issue::MissingSection, "section"}})
				QVERIFY2(hasIssue(r.diagnostics, e.first, e.second), e.second.c_str());
			QCOMPARE(r.diagnostics.size(), size_t(7));
		}

		void factorsNeedTheirReason()
		{
			AmpacityInput in = pvcInput(2.5, 40);
			in.groupingJustification = "  ";
			AmpacityResult r = admissibleCurrent(in, library());
			QCOMPARE(r.status, Status::NotComputable);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingJustification, "groupingFactor"));
			QVERIFY(!r.admissible);

			in = pvcInput(2.5, 40);
			in.temperatureFactor = 0.9;
			r = admissibleCurrent(in, library());
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingJustification, "temperatureFactor"));
		}

		void invalidData()
		{
			AmpacityInput in = pvcInput(2.5, 70);   // ambient at theta max
			QVERIFY(hasIssue(admissibleCurrent(in, library()).diagnostics,
							 Issue::OutOfRange, "ambientTemperature"));
			in = pvcInput(2.5, 40);
			in.groupingFactor = 0.0;
			QCOMPARE(admissibleCurrent(in, library()).status, Status::InvalidInput);
			in.groupingFactor = 2.5;
			QVERIFY(hasIssue(admissibleCurrent(in, library()).diagnostics,
							 Issue::OutOfRange, "groupingFactor"));
			in = pvcInput(2.5, 40);
			in.loadedConductors = 4;
			QVERIFY(hasIssue(admissibleCurrent(in, library()).diagnostics,
							 Issue::OutOfRange, "loadedConductors"));
			in = pvcInput(-2.5, 40);
			QCOMPARE(admissibleCurrent(in, library()).status, Status::InvalidInput);
		}

		void tableProblems()
		{
			AmpacityInput in = pvcInput(3.0, 40);   // not a row
			AmpacityResult r = admissibleCurrent(in, library());
			QCOMPARE(r.status, Status::NotComputable);
			QVERIFY(hasIssue(r.diagnostics, Issue::SectionNotInTable, "section"));
				// no table loaded at all
			r = admissibleCurrent(pvcInput(2.5, 40), AmpacityLibrary());
			QVERIFY(hasIssue(r.diagnostics, Issue::NoTable, "table"));
			QVERIFY(!r.admissible);
		}

		void checkIbIz()
		{
			const AmpacityLibrary lib = library();
				// 16 A in 17.32 A: passed
			CheckResult c = checkAmpacity(16.0, admissibleCurrent(pvcInput(2.5, 40), lib));
			QCOMPARE(c.outcome, CheckOutcome::Passed);
			QVERIFY(near(c.limit, 17.320508));
			QCOMPARE(c.rule.id, std::string("REBT-IZ-001"));
				// 16 A in 13.92 A: not passed
			AmpacityInput in = pvcInput(2.5, 40);
			in.temperatureFactor = 0.87;
			in.temperatureJustification = "table";
			in.groupingFactor = 0.8;
			c = checkAmpacity(16.0, admissibleCurrent(in, lib));
			QCOMPARE(c.outcome, CheckOutcome::NotPassed);
				// IB = IZ = 16 A (20 x 1 x 0.8): passes
			in = pvcInput(2.5, 30);
			in.groupingFactor = 0.8;
			c = checkAmpacity(16.0, admissibleCurrent(in, lib));
			QCOMPARE(c.outcome, CheckOutcome::Passed);
				// unknown current or IZ: not performed
			QCOMPARE(checkAmpacity(std::nullopt, admissibleCurrent(pvcInput(2.5, 40), lib)).outcome,
					 CheckOutcome::NotPerformed);
			QCOMPARE(checkAmpacity(16.0, admissibleCurrent(AmpacityInput(), lib)).outcome,
					 CheckOutcome::NotPerformed);
		}

		void suggestByAmpacityOnly()
		{
			const SectionSuggestion s = suggestSection(circuit16A(25), pvcInput(0, 40),
													   library(), std::nullopt);
			QCOMPARE(s.status, Status::Ok);
			QCOMPARE(*s.section, 2.5);
			QVERIFY(near(*s.admissible, 17.320508));
			QVERIFY(!s.dropConsidered);
		}

		void suggestWithDropLimit()
		{
				// 25 m: 2.5 mm2 gives 2.90 %, within 3 %
			SectionSuggestion s = suggestSection(circuit16A(25), pvcInput(0, 40), library(),
												 rules::dwellingInterior());
			QCOMPARE(*s.section, 2.5);
			QVERIFY(s.dropConsidered);
			QVERIFY(near(*s.dropPercent, 2.898551));
				// 40 m: 2.5 mm2 gives 4.64 %, 4 mm2 2.90 %
			s = suggestSection(circuit16A(40), pvcInput(0, 40), library(),
							   rules::dwellingInterior());
			QCOMPARE(*s.section, 4.0);
			QVERIFY(near(*s.admissible, 25.980762));
			QVERIFY(near(*s.dropPercent, 2.898551));
				// the drop cannot be worked out: by ampacity, and said so
			CircuitInput no_length = circuit16A(25);
			no_length.length.reset();
			s = suggestSection(no_length, pvcInput(0, 40), library(),
							   rules::dwellingInterior());
			QCOMPARE(*s.section, 2.5);
			QVERIFY(!s.dropConsidered);
		}

		void suggestNoneEnough()
		{
			CircuitInput big = circuit16A(25);
			big.power = 23000.0;   // 100 A
			const SectionSuggestion s = suggestSection(big, pvcInput(0, 40), library(),
													   std::nullopt);
			QCOMPARE(s.status, Status::Ok);
			QVERIFY(s.noneEnough);
			QVERIFY(!s.section);
		}

		void suggestNeedsItsData()
		{
			CircuitInput c = circuit16A(25);
			c.power.reset();
			SectionSuggestion s = suggestSection(c, pvcInput(0, 40), library(), std::nullopt);
			QCOMPARE(s.status, Status::NotComputable);
			QVERIFY(hasIssue(s.diagnostics, Issue::MissingPower, "power"));
			QVERIFY(!s.section);
			s = suggestSection(circuit16A(25), pvcInput(0, 40), AmpacityLibrary(), std::nullopt);
			QVERIFY(hasIssue(s.diagnostics, Issue::NoTable, "table"));
		}
};

QTEST_GUILESS_MAIN(tst_rebtampacity)
#include "tst_rebtampacity.moc"
