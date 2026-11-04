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
#include "rebt/rebtprotection.h"

#include <QtTest>

#include <algorithm>

using namespace rebt;

Q_DECLARE_METATYPE(rebt::ProtectionStandard)
Q_DECLARE_METATYPE(rebt::CheckOutcome)

namespace {

bool hasIssue(const std::vector<Diagnostic> &list, Issue issue, const std::string &field)
{
	return std::find(list.begin(), list.end(), Diagnostic{issue, field}) != list.end();
}

CoordinationInput input(double ib, double in, double iz, ProtectionStandard standard)
{
	CoordinationInput c;
	c.designCurrent = ib;
	c.ratedCurrent = in;
	c.admissible = iz;
	c.standard = standard;
	return c;
}

} // namespace

/**
	Reference cases worked out apart in docs/rebt/coordinacion_h5.md
*/
class tst_rebtprotection : public QObject
{
		Q_OBJECT

	private slots:
		void conventionalFactors()
		{
			QCOMPARE(*conventionalFactor(ProtectionStandard::EN60898, 16), 1.45);
			QCOMPARE(*conventionalFactor(ProtectionStandard::EN61009, 40), 1.45);
			QCOMPARE(*conventionalFactor(ProtectionStandard::EN60947_2, 100), 1.30);
			QCOMPARE(*conventionalFactor(ProtectionStandard::FuseGG, 16), 1.6);
			QCOMPARE(*conventionalFactor(ProtectionStandard::FuseGG, 63), 1.6);
				// under 16 A the standard alone does not say: never guessed
			QVERIFY(!conventionalFactor(ProtectionStandard::FuseGG, 10));
			QVERIFY(!conventionalFactor(ProtectionStandard::FuseGG, 2));
		}

		void cases_data()
		{
			QTest::addColumn<double>("ib");
			QTest::addColumn<double>("in");
			QTest::addColumn<double>("iz");
			QTest::addColumn<ProtectionStandard>("standard");
			QTest::addColumn<double>("i2");
			QTest::addColumn<CheckOutcome>("ibIn");
			QTest::addColumn<CheckOutcome>("inIz");
			QTest::addColumn<CheckOutcome>("i2Iz");
			QTest::addColumn<CheckOutcome>("outcome");
			const auto P = CheckOutcome::Passed;
			const auto N = CheckOutcome::NotPassed;

			QTest::newRow("P1 16/16/17.32 EN 60898")
				<< 16.0 << 16.0 << 17.320508 << ProtectionStandard::EN60898 << 23.2
				<< P << P << P << P;
			QTest::newRow("P2 IN above IZ")
				<< 16.0 << 20.0 << 17.320508 << ProtectionStandard::EN60898 << 29.0
				<< P << N << N << N;
			QTest::newRow("P3 IN above IZ")
				<< 16.0 << 16.0 << 15.0 << ProtectionStandard::EN60898 << 23.2
				<< P << N << N << N;
			QTest::newRow("P4 EN 60947-2, 1.30")
				<< 16.0 << 16.0 << 16.0 << ProtectionStandard::EN60947_2 << 20.8
				<< P << P << P << P;
			QTest::newRow("P5 gG fuse: IN <= IZ but I2 > 1.45 IZ")
				<< 16.0 << 16.0 << 17.0 << ProtectionStandard::FuseGG << 25.6
				<< P << P << N << N;
			QTest::newRow("P8 all equal")
				<< 16.0 << 16.0 << 16.0 << ProtectionStandard::EN60898 << 23.2
				<< P << P << P << P;
				// at the edge of I2 <= 1.45 IZ: 1.45 x 17.6 = 25.52, 1.45 x 17.7 = 25.665
			QTest::newRow("P11 gG 16 A on 17.6 A: 25.6 > 25.52")
				<< 16.0 << 16.0 << 17.6 << ProtectionStandard::FuseGG << 25.6
				<< P << P << N << N;
			QTest::newRow("P12 gG 16 A on 17.7 A: 25.6 <= 25.665")
				<< 16.0 << 16.0 << 17.7 << ProtectionStandard::FuseGG << 25.6
				<< P << P << P << P;
			QTest::newRow("P10 IB above IN")
				<< 20.0 << 16.0 << 25.0 << ProtectionStandard::EN60898 << 23.2
				<< N << P << P << N;
		}

		void cases()
		{
			QFETCH(double, ib);
			QFETCH(double, in);
			QFETCH(double, iz);
			QFETCH(ProtectionStandard, standard);
			QFETCH(double, i2);
			QFETCH(CheckOutcome, ibIn);
			QFETCH(CheckOutcome, inIz);
			QFETCH(CheckOutcome, i2Iz);
			QFETCH(CheckOutcome, outcome);

			const CoordinationResult r = checkCoordination(input(ib, in, iz, standard));
			QCOMPARE(r.status, Status::Ok);
			QVERIFY(qAbs(*r.conventionalCurrent - i2) < 1e-6);
			QCOMPARE(r.ibIn, ibIn);
			QCOMPARE(r.inIz, inIz);
			QCOMPARE(r.i2Iz, i2Iz);
			QCOMPARE(r.outcome, outcome);
			QCOMPARE(r.rules.at(0).id, std::string("REBT-PROT-001"));
			QCOMPARE(r.rules.at(0).source, std::string("ITC-BT-22"));
			QCOMPARE(r.rules.at(1).id, std::string("REBT-PROT-002"));
		}

		void smallFuseNeedsItsI2()
		{
				// P6: gG 10 A, the I2 of the manufacturer is needed
			CoordinationInput c = input(8, 10, 12, ProtectionStandard::FuseGG);
			CoordinationResult r = checkCoordination(c);
			QCOMPARE(r.status, Status::NotComputable);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingConventionalCurrent,
							 "conventionalCurrent"));
			QCOMPARE(r.ibIn, CheckOutcome::Passed);
			QCOMPARE(r.inIz, CheckOutcome::Passed);
			QCOMPARE(r.i2Iz, CheckOutcome::NotPerformed);
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);

				// given, with its source: 19 A <= 1.45 x 12 = 17.4? no
			c.conventionalCurrent = 19.0;
			c.conventionalJustification = "manufacturer's catalogue";
			r = checkCoordination(c);
			QCOMPARE(r.status, Status::Ok);
			QCOMPARE(*r.conventionalCurrent, 19.0);
			QCOMPARE(r.i2Iz, CheckOutcome::NotPassed);
				// given without its source: not used
			c.conventionalJustification.clear();
			r = checkCoordination(c);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingJustification,
							 "conventionalCurrent"));
			QVERIFY(!r.conventionalCurrent);
		}

		void deviceWithoutOverloadProtection()
		{
				// P7: a residual current device alone
			CoordinationInput c = input(16, 25, 17, ProtectionStandard::EN60898);
			c.protectsAgainstOverload = false;
			const CoordinationResult r = checkCoordination(c);
			QCOMPARE(r.status, Status::NotComputable);
			QVERIFY(hasIssue(r.diagnostics, Issue::NoOverloadProtection, "protectionType"));
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);
		}

		void missingData()
		{
				// P9: no IN: nothing to compare with
			CoordinationInput c = input(16, 16, 17, ProtectionStandard::EN60898);
			c.ratedCurrent.reset();
			CoordinationResult r = checkCoordination(c);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingRatedCurrent, "ratedCurrent"));
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);

				// no IZ: IB <= IN still checked, and it fails
			c = input(20, 16, 17, ProtectionStandard::EN60898);
			c.admissible.reset();
			r = checkCoordination(c);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingAdmissibleCurrent, "admissibleCurrent"));
			QCOMPARE(r.ibIn, CheckOutcome::NotPassed);
			QCOMPARE(r.outcome, CheckOutcome::NotPassed);

				// no standard: no I2
			c = input(16, 16, 17, ProtectionStandard::EN60898);
			c.standard.reset();
			r = checkCoordination(c);
			QVERIFY(hasIssue(r.diagnostics, Issue::MissingProtectionStandard,
							 "protectionStandard"));
			QCOMPARE(r.i2Iz, CheckOutcome::NotPerformed);
			QCOMPARE(r.outcome, CheckOutcome::NotPerformed);

				// nothing at all
			r = checkCoordination(CoordinationInput());
			QCOMPARE(r.diagnostics.size(), size_t(4));
		}

		void invalidData()
		{
			CoordinationInput c = input(16, 0, 17, ProtectionStandard::EN60898);
			QCOMPARE(checkCoordination(c).status, Status::InvalidInput);
			c = input(16, 16, -1, ProtectionStandard::EN60898);
			QCOMPARE(checkCoordination(c).status, Status::InvalidInput);
		}

		void connectionLimits()
		{
			struct L { VoltageDropLimit limit; double percent; const char *itc; };
			const L limits[] = {
				{rules::lgaTotallyCentralized(), 0.5, "ITC-BT-14"},
				{rules::lgaPartlyCentralized(), 1.0, "ITC-BT-14"},
				{rules::diMetersInSeveralPlaces(), 0.5, "ITC-BT-15"},
				{rules::diMetersTotallyGrouped(), 1.0, "ITC-BT-15"},
				{rules::diSingleUser(), 1.5, "ITC-BT-15"}};
			for (const L &l : limits) {
				QCOMPARE(l.limit.percent, l.percent);
				QCOMPARE(l.limit.rule.source, std::string(l.itc));
				QCOMPARE(l.limit.rule.section, std::string("3"));
					// "maxima": equal passes
				QVERIFY(!l.limit.strict);
			}
				// 1.5 % exactly passes the DI of a single user
			VoltageDropResult drop;
			drop.status = Status::Ok;
			drop.percent = 1.5;
			QCOMPARE(checkVoltageDrop(drop, rules::diSingleUser()).outcome,
					 CheckOutcome::Passed);
			drop.percent = 1.5001;
			QCOMPARE(checkVoltageDrop(drop, rules::diSingleUser()).outcome,
					 CheckOutcome::NotPassed);
		}
};

QTEST_GUILESS_MAIN(tst_rebtprotection)
#include "tst_rebtprotection.moc"
