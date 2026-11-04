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
#include "electrical/electricalmodel.h"
#include "electrical/ampacityfile.h"
#include "electrical/electricalevaluation.h"
#include "rebt/rebtcalc.h"

#include <QtTest>
#include <QTemporaryDir>

using namespace electrical;

namespace {

QUuid id(int n)
{
	return QUuid(QStringLiteral("{00000000-0000-0000-0000-%1}")
				 .arg(n, 12, 10, QLatin1Char('0')));
}

Circuit fullCircuit()
{
	Circuit c;
	c.id = id(10);
	c.name = QStringLiteral("C1 Alumbrado ñ");
	c.board = id(1);
	c.origin = id(100);
	c.destination = id(101);
	c.system = rebt::System::SinglePhase;
	c.voltage = 230.0;
	c.use = CircuitUse::DwellingInterior;
	c.load.power = 3680.0;
	c.load.powerFactor = 0.8;
	c.load.efficiency = 0.9;
	c.load.designCurrent = 16.0;
	c.conductor.material = rebt::Material::Copper;
	c.conductor.insulation = Insulation::XLPE;
	c.conductor.installationMethod = QStringLiteral("B1");
	c.conductor.section = 2.5;
	c.conductor.neutralSection = 2.5;
	c.conductor.peSection = 2.5;
	c.conductor.length = 25.0;
	c.conductor.temperature = 70.0;
	c.conductor.conductivity = 48.0;
	c.conductor.reactance = 0.08;
	c.conductor.wires = {id(200), id(201)};
	c.conductor.loadedConductors = 2;
	c.conductor.ambientTemperature = 40.0;
	c.conductor.temperatureFactor = 0.87;
	c.conductor.temperatureFactorReason = QStringLiteral("Tabla B.52.14, 40 °C");
	c.conductor.groupingFactor = 0.8;
	c.conductor.groupingFactorReason = QStringLiteral("2 circuitos en el tubo");
	c.conductor.otherFactor = 0.95;
	c.conductor.otherFactorReason = QStringLiteral("Aislamiento térmico cercano");
	c.protection.element = id(102);
	c.protection.manufacturer = QStringLiteral("Fabricante & Cía");
	c.protection.partNumber = QStringLiteral("REF-123 <C16>");
	c.protection.type = ProtectionType::CircuitBreaker;
	c.protection.ratedCurrent = 16.0;
	c.protection.curve = QStringLiteral("C");
	c.protection.breakingCapacity = 6.0;
	c.protection.sensitivity = 30.0;
	c.protection.poles = 2;
	c.protection.standard = rebt::ProtectionStandard::EN60898;
	c.protection.conventionalCurrent = 23.2;
	c.protection.conventionalCurrentReason = QStringLiteral("Catálogo del fabricante");
	c.protection.tripCurrent = 160.0;
	c.protection.tripCurrentReason = QStringLiteral("Curva C, 10 In");
	c.protection.letThrough = 20000.0;
	c.protection.letThroughReason = QStringLiteral("Curva I²t del catálogo, 3 kA");
	c.notes = QStringLiteral("Longitud estimada en obra <pendiente> & revisar");
	c.dwellingCircuit = rebt::DwellingCircuit::C12C4;
	c.conductor.conduit = 20.0;
	c.protection.residualDevice = QStringLiteral("ID2 <ñ>");
	c.points = 3;
	return c;
}

ElectricalModel fullModel()
{
	ElectricalModel model;
	Board cgmp{id(1), QStringLiteral("CGMP"), id(103), QUuid()};
	cgmp.source = rebt::SourceMethod::KnownCurrent;
	cgmp.sourceMaxCurrent = 4.5;
	cgmp.sourceMinCurrent = 1.5;
	cgmp.sourceReason = QStringLiteral("Carta de la distribuidora <ref> & fecha");
	cgmp.ownTransformer = true;
	cgmp.dwellingArea = 95.5;
	cgmp.plannedHeating = false;
	cgmp.plannedAirConditioning = true;
	cgmp.plannedVehicleCharging = false;
	cgmp.singleFamily = true;
	cgmp.generalSwitch = 40.0;
	cgmp.plannedDryer = true;
	cgmp.compensateWithIndividualLine = true;
	{
		RoomData salon;
		salon.name = QStringLiteral("Salón <1>");
		salon.type = rebt::RoomType::LivingRoom;
		salon.area = 24.5;
		salon.counts = {{rebt::RoomItem::LightPoints, 2}, {rebt::RoomItem::Sockets, 0}};
		RoomData pasillo;
		pasillo.type = rebt::RoomType::Corridor;
		pasillo.length = 6.0;
		pasillo.accesses = 3;
		cgmp.rooms = {salon, pasillo};
	}
	cgmp.residualDevices = {{QStringLiteral("ID1"), 30.0, 40.0},
							{QStringLiteral("ID2 <ñ>"), std::nullopt, 63.0}};
	cgmp.earthing = rebt::EarthingSystem::TT;
	cgmp.earthResistance = 37.5;
	cgmp.earthResistanceReason = QStringLiteral("Medida con telurómetro <fecha> & firma");
	cgmp.limitVoltage = 24.0;
	cgmp.limitVoltageReason = QStringLiteral("Local mojado");
	model.setBoard(cgmp);
	Board cs{id(2), QStringLiteral("CS"), QUuid(), id(10)};
	cs.earthing = rebt::EarthingSystem::IT;
	cs.firstFaultCurrent = 0.35;
	cs.firstFaultReason = QStringLiteral("Medida <IT> & ensayo");
	cs.massesInterconnected = true;
	cs.neutralDistributed = false;
	model.setBoard(cs);
	Board taller{id(3), QStringLiteral("Taller"), QUuid(), QUuid()};
	taller.earthing = rebt::EarthingSystem::TNC;
	model.setBoard(taller);
	model.setCircuit(fullCircuit());
	Circuit bare;
	bare.id = id(11);
	bare.board = id(1);
	model.setCircuit(bare);
	return model;
}

ElectricalModel roundTrip(const ElectricalModel &model, ReadReport *report = nullptr)
{
	QDomDocument document;
	const QDomElement root = model.toXml(document);
	document.appendChild(root);
	QDomDocument reread;
	reread.setContent(document.toString());
	ElectricalModel read;
	const ReadReport r = read.fromXml(reread.documentElement());
	if (report)
		*report = r;
	return read;
}

ElectricalModel parse(const QString &xml, ReadReport *report)
{
	QDomDocument document;
	if (!document.setContent(xml))
		qFatal("bad test XML");
	ElectricalModel model;
	*report = model.fromXml(document.documentElement());
	return model;
}

/// SYNTHETIC values, round on purpose: not a table of any standard
QByteArray validJson()
{
	return R"({"format": "qet-ampacity", "version": 1, "tables": [
		{"id": "TEST-PVC2-B1", "source": "synthetic test data", "material": "Cu",
		 "insulation": "PVC", "loadedConductors": 2, "method": "B1",
		 "referenceAmbient": 30,
		 "entries": [[4, 30], [1.5, 10], [2.5, 20], [6, 40], [10, 60]]},
		{"id": "TEST-XLPE3-E", "source": "synthetic test data", "material": "Cu",
		 "insulation": "XLPE", "loadedConductors": 3, "method": "E",
		 "referenceAmbient": 30, "entries": [[2.5, 25], [4, 35], [6, 45]]}]})";
}

/// CGMP (origin) -> feeder "Alimentación CS" (3~ 400 V, 30 A, 20 m, 10 mm2)
/// -> CS -> circuit C1 (1~ 230 V, 16 A, 25 m, 2.5 mm2), dwelling
ElectricalModel chainModel()
{
	ElectricalModel model;
	model.setBoard({id(201), QStringLiteral("CGMP"), QUuid(), QUuid()});
	model.setBoard({id(202), QStringLiteral("CS"), QUuid(), id(300)});
	Circuit f;
	f.id = id(300);
	f.name = QStringLiteral("Alimentación CS");
	f.board = id(201);
	f.use = CircuitUse::OtherUses;
	f.system = rebt::System::ThreePhase;
	f.voltage = 400.0;
	f.load.designCurrent = 30.0;
	f.load.powerFactor = 0.9;
	f.conductor.length = 20.0;
	f.conductor.section = 10.0;
	f.conductor.conductivity = 48.0;
	model.setCircuit(f);
	Circuit c;
	c.id = id(301);
	c.name = QStringLiteral("C1");
	c.board = id(202);
	c.use = CircuitUse::DwellingInterior;
	c.system = rebt::System::SinglePhase;
	c.voltage = 230.0;
	c.load.designCurrent = 16.0;
	c.load.powerFactor = 1.0;
	c.conductor.length = 25.0;
	c.conductor.section = 2.5;
	c.conductor.conductivity = 48.0;
	model.setCircuit(c);
	return model;
}

} // namespace

class tst_electricalmodel : public QObject
{
		Q_OBJECT

		/// Case M of docs/rebt/refs_h7.py, through the model
		ElectricalModel faultModel()
		{
			ElectricalModel model;
			Board cpm{id(400), QStringLiteral("CPM"), QUuid(), QUuid()};
			cpm.source = rebt::SourceMethod::GuideFactor;
			model.setBoard(cpm);
			model.setBoard({id(401), QStringLiteral("CGMP"), QUuid(), id(410)});
			Circuit di;
			di.id = id(410);
			di.name = QStringLiteral("DI");
			di.board = id(400);
			di.use = CircuitUse::DiSingleUser;
			di.system = rebt::System::SinglePhase;
			di.voltage = 230.0;
			di.conductor.material = rebt::Material::Copper;
			di.conductor.insulation = Insulation::XLPE;
			di.conductor.section = 10.0;
			di.conductor.neutralSection = 10.0;
			di.conductor.length = 15.0;
			model.setCircuit(di);
			Circuit c;
			c.id = id(411);
			c.name = QStringLiteral("C1");
			c.board = id(401);
			c.use = CircuitUse::DwellingInterior;
			c.system = rebt::System::SinglePhase;
			c.voltage = 230.0;
			c.conductor.material = rebt::Material::Copper;
			c.conductor.insulation = Insulation::PVC;
			c.conductor.section = 2.5;
			c.conductor.length = 20.0;
			c.protection.type = ProtectionType::CircuitBreaker;
			c.protection.ratedCurrent = 16.0;
			c.protection.curve = QStringLiteral(" c ");
			c.protection.standard = rebt::ProtectionStandard::EN60898;
			c.protection.breakingCapacity = 6.0;
			model.setCircuit(c);
			return model;
		}

	private slots:
		void faultPathThroughEveryFeeder()
		{
			const ElectricalModel model = faultModel();
			const CircuitEvaluation e = evaluate(*model.circuit(id(411)), model);
			QVERIFY(e.faultDiagnostics.empty());
				// the DI is in the fault path, unlike the interior drop
			QCOMPARE(e.faultFeeders, QList<QUuid>{id(410)});
			QCOMPARE(e.faultFeederNames, QStringList{QStringLiteral("DI")});
			QCOMPARE(e.faultOriginBoard, id(400));
			QVERIFY(qAbs(*e.shortCircuit.maxCurrent - 3557.333333) < 1e-5);
			QVERIFY(qAbs(*e.shortCircuit.minCurrent - 347.395833) < 1e-5);
				// curve " c " read as C: 160 A; 6 kA read as 6000 A
			QCOMPARE(*e.shortCircuit.tripCurrent, 160.0);
			QCOMPARE(e.shortCircuit.breaking, rebt::CheckOutcome::Passed);
			QCOMPARE(e.shortCircuit.trip, rebt::CheckOutcome::Passed);
				// (115 x 2.5 / 3557)^2 < 0.1 s: the I2t is needed
			QCOMPARE(e.shortCircuit.thermal, rebt::CheckOutcome::NotPerformed);

				// three phase: U0 = U / sqrt(3)
			Circuit three = *model.circuit(id(411));
			three.system = rebt::System::ThreePhase;
			three.voltage = 400.0;
			const CircuitEvaluation t = evaluate(three, model);
			QVERIFY(qAbs(*t.shortCircuit.maxCurrent
						 - 3557.333333 * 400.0 / std::sqrt(3.0) / 230.0) < 1e-4);
		}

		void sourceNeedsItsJustification()
		{
			ElectricalModel model = faultModel();
			Board cpm = *model.board(id(400));
			cpm.source = rebt::SourceMethod::KnownCurrent;
			cpm.sourceMaxCurrent = 10.0;
			cpm.sourceMinCurrent = 6.0;
			model.setBoard(cpm);
			CircuitEvaluation e = evaluate(*model.circuit(id(411)), model);
			QVERIFY(!e.shortCircuit.maxCurrent);
			QCOMPARE(e.faultDiagnostics.size(), std::size_t(1));
			QCOMPARE(e.faultDiagnostics.front().issue, rebt::Issue::MissingJustification);
			cpm.sourceReason = QStringLiteral("Carta de la distribuidora");
			model.setBoard(cpm);
			e = evaluate(*model.circuit(id(411)), model);
				// kA of the board read as A
			const double zs = 230.0 / 10000.0;
			const double r = (1.0 / 58.0) * 15.0 / 10.0;
			QVERIFY(qAbs(*e.shortCircuit.maxCurrent - 230.0 / std::sqrt(zs * zs + r * r))
					< 1e-6);
		}

		void faultPathProblems()
		{
				// no supply data: nothing computed upstream
			ElectricalModel model = faultModel();
			Board cpm = *model.board(id(400));
			cpm.source.reset();
			model.setBoard(cpm);
			CircuitEvaluation e = evaluate(*model.circuit(id(411)), model);
			QVERIFY(!e.shortCircuit.maxCurrent);
			QVERIFY(!e.shortCircuit.minCurrent);
			QCOMPARE(e.shortCircuit.outcome, rebt::CheckOutcome::NotPerformed);

				// boards feeding each other
			model = faultModel();
			cpm = *model.board(id(400));
			cpm.feeder = id(411);
			model.setBoard(cpm);
			Circuit c = *model.circuit(id(411));
			e = evaluate(c, model);
			QVERIFY(!e.faultDiagnostics.empty());
			QCOMPARE(e.faultDiagnostics.front().issue, rebt::Issue::FeederLoop);
			QVERIFY(!e.shortCircuit.maxCurrent);
				// not even from the circuit alone: the supply is not known
			QVERIFY(!e.shortCircuit.minCurrent);

				// a circuit of no board
			model = faultModel();
			c = *model.circuit(id(411));
			c.board = id(999);
			e = evaluate(c, model);
			QCOMPARE(e.faultDiagnostics.front().issue, rebt::Issue::MissingSource);
			QVERIFY(!e.shortCircuit.maxCurrent);

				// without a model, no short circuit at all
			e = evaluate(*faultModel().circuit(id(411)));
			QVERIFY(!e.shortCircuit.maxCurrent);
			QVERIFY(e.shortCircuit.rules.empty());
		}

		void faultPathOrderAndNeutral()
		{
				// CPM -> DI -> CGMP -> FS -> CS -> C2: origin first
			ElectricalModel model = faultModel();
			model.setBoard({id(402), QStringLiteral("CS"), QUuid(), id(412)});
			Circuit fs = *model.circuit(id(411));
			fs.id = id(412);
			fs.name = QStringLiteral("FS");
			model.setCircuit(fs);
			Circuit c2 = *model.circuit(id(411));
			c2.id = id(413);
			c2.name = QStringLiteral("C2");
			c2.board = id(402);
			model.setCircuit(c2);
			CircuitEvaluation e = evaluate(c2, model);
			QCOMPARE(e.faultFeederNames,
					 (QStringList{QStringLiteral("DI"), QStringLiteral("FS")}));
			QCOMPARE(e.faultFeeders, (QList<QUuid>{id(410), id(412)}));

				// the neutral of a DI is not set by ITC-BT-19: never assumed
				// for the minimum; that of an interior circuit is
			Circuit di = *model.circuit(id(410));
			di.conductor.neutralSection.reset();
			model.setCircuit(di);
			e = evaluate(c2, model);
			QVERIFY(e.shortCircuit.maxCurrent);
			QVERIFY(!e.shortCircuit.minCurrent);
			QCOMPARE(e.shortCircuit.diagnostics.size(), std::size_t(1));
			QCOMPARE(e.shortCircuit.diagnostics.front().field,
					 std::string("feeder[0]/neutralSection"));
		}

		void boardEqualityCoversTheSupply()
		{
			Board a{id(1), QStringLiteral("CGMP"), QUuid(), QUuid()};
			a.source = rebt::SourceMethod::KnownCurrent;
			a.sourceMaxCurrent = 6.0;
			a.sourceMinCurrent = 3.0;
			a.sourceReason = QStringLiteral("Carta");
			Board b = a;
			QVERIFY(a == b);
			b.sourceReason = QStringLiteral("Estudio");
			QVERIFY(!(a == b));
			b = a; b.sourceMinCurrent = 2.0;
			QVERIFY(!(a == b));
			b = a; b.source = rebt::SourceMethod::GuideFactor;
			QVERIFY(!(a == b));
		}

		/// chainModel: feeder 3~ 400 V 30 A cos 0.9 20 m 10 mm2 gamma 48,
		/// dU = sqrt(3) 20 30 0.9 / (48 x 10) = 1.948557 V = 0.487139 %
		void dwellingDropWithTheBreaker()
		{
			ElectricalModel model = chainModel();
			Circuit c = *model.circuit(id(301));
			c.dwellingCircuit = rebt::DwellingCircuit::C2;
			c.points = 12;
			c.conductor.conduit = 20.0;   // table 1: 20 mm for C2
			c.conductor.material = rebt::Material::Copper;
			c.protection.ratedCurrent = 16.0;
			model.setCircuit(c);
				// 2 x 25 x 16 / (48 x 2.5) = 6.666667 V = 2.898551 %
			CircuitEvaluation e = evaluate(c, model);
			QVERIFY(qAbs(*e.dropAtRated.percent - 2.898551) < 1e-5);
			QVERIFY(qAbs(*e.cumulativeAtRated - 3.385690) < 1e-5);
			QCOMPARE(e.dwelling.drop, rebt::CheckOutcome::NotPassed);
			QCOMPARE(e.dwelling.breaker, rebt::CheckOutcome::Passed);
			QCOMPARE(e.dwelling.points, rebt::CheckOutcome::Passed);
			QCOMPARE(e.dwelling.outcome, rebt::CheckOutcome::NotPassed);
				// the drop uses IN, not IB: a 10 A breaker, 6 A of load
			c.load.designCurrent = 6.0;
			c.protection.ratedCurrent = 10.0;
			model.setCircuit(c);
			e = evaluate(c, model);
				// 2 x 25 x 10 / 120 = 4.166667 V = 1.811594 %, + 0.487139
			QVERIFY(qAbs(*e.cumulativeAtRated - 2.298733) < 1e-5);
			QCOMPARE(e.dwelling.drop, rebt::CheckOutcome::Passed);
			QCOMPARE(e.dwelling.outcome, rebt::CheckOutcome::Passed);
				// cos phi 1 as Guia-BT-25 table B when not given, and said
			c.load.powerFactor.reset();
			model.setCircuit(c);
			e = evaluate(c, model);
			QVERIFY(std::find(e.dropAtRated.assumptions.begin(),
							  e.dropAtRated.assumptions.end(),
							  rebt::Assumption::PowerFactorOne)
					!= e.dropAtRated.assumptions.end());
			QVERIFY(qAbs(*e.cumulativeAtRated - 2.298733) < 1e-5);
				// IN is not a current the user gave
			QCOMPARE(e.dropAtRated.assumptions.front(), rebt::Assumption::DropAtRatedCurrent);
			QVERIFY(std::find(e.dropAtRated.assumptions.begin(),
							  e.dropAtRated.assumptions.end(),
							  rebt::Assumption::CurrentGiven)
					== e.dropAtRated.assumptions.end());
				// without the model the drop from the origin is unknown
			const CircuitEvaluation alone = evaluate(c);
			QCOMPARE(alone.dwelling.drop, rebt::CheckOutcome::NotPerformed);
			QCOMPARE(alone.dwelling.breaker, rebt::CheckOutcome::Passed);
		}

		void indirectContactEvaluation()
		{
				// C1 on CS, fed from CGMP; the earthing is given on CGMP only
			ElectricalModel model = chainModel();
			Circuit c = *model.circuit(id(301));
			c.protection.type = ProtectionType::CircuitBreaker;
			c.protection.ratedCurrent = 16.0;
			c.protection.standard = rebt::ProtectionStandard::EN60898;
			c.protection.curve = QStringLiteral("C");
			model.setCircuit(c);
			CircuitEvaluation e = evaluate(c, model);
			QVERIFY(e.earthingBoard.isNull());
			QCOMPARE(e.indirect.outcome, rebt::CheckOutcome::NotPerformed);
			QVERIFY(std::find(e.indirect.diagnostics.begin(), e.indirect.diagnostics.end(),
							  rebt::Diagnostic{rebt::Issue::MissingEarthing, "earthing"})
					!= e.indirect.diagnostics.end());

			Board cgmp = *model.board(id(201));
			cgmp.earthing = rebt::EarthingSystem::TT;
			cgmp.earthResistance = 30.0;
			cgmp.earthResistanceReason = QStringLiteral("Medida");
			cgmp.residualDevices = {{QStringLiteral("ID1"), 30.0, 40.0}};
			model.setBoard(cgmp);
				// no residual current device: Ia 10 x 16 = 160 A, 30 x 160 = 4800 V
			e = evaluate(c, model);
			QCOMPARE(e.earthingBoard, id(201));
			QCOMPARE(e.earthResistanceReason, QStringLiteral("Medida"));
			QCOMPARE(*e.indirect.operatingCurrent, 160.0);
			QCOMPARE(e.indirect.outcome, rebt::CheckOutcome::NotPassed);
				// ID1 of CGMP protects the feeder of CS: 30 x 0.030 = 0.9 V
			c.protection.residualDevice = QStringLiteral(" ID1 ");
			model.setCircuit(c);
			e = evaluate(c, model);
			QCOMPARE(e.residualDeviceName, QStringLiteral("ID1"));
			QCOMPARE(*e.indirect.operatingCurrent, 0.030);
			QVERIFY(qAbs(*e.indirect.contactVoltage - 0.9) < 1e-12);
			QCOMPARE(e.indirect.outcome, rebt::CheckOutcome::Passed);
				// CS gives its own earthing: it goes before that of CGMP
			Board cs = *model.board(id(202));
			cs.earthing = rebt::EarthingSystem::TT;
			cs.earthResistance = 2000.0;
			cs.earthResistanceReason = QStringLiteral("Estimada ITC-BT-18");
			model.setBoard(cs);
			e = evaluate(c, model);
			QCOMPARE(e.earthingBoard, id(202));
			QVERIFY(qAbs(*e.indirect.contactVoltage - 60.0) < 1e-9);   // 2000 x 0.030
			QCOMPARE(e.indirect.outcome, rebt::CheckOutcome::NotPassed);
				// a TN board: its rule, and the PE asked for
			cs.earthing = rebt::EarthingSystem::TN;
			model.setBoard(cs);
			e = evaluate(c, model);
			QCOMPARE(e.indirect.outcome, rebt::CheckOutcome::NotPerformed);
			QVERIFY(std::any_of(e.indirect.rules.begin(), e.indirect.rules.end(),
								[](const rebt::NormRef &n) { return n.id == "REBT-CIN-002"; }));
			QVERIFY(std::find(e.indirect.diagnostics.begin(), e.indirect.diagnostics.end(),
							  rebt::Diagnostic{rebt::Issue::MissingProtectiveSection,
											   "protectiveSection"})
					!= e.indirect.diagnostics.end());
			cs.earthing.reset();
			model.setBoard(cs);
				// a device named that is not there: not the breaker instead
			c.protection.residualDevice = QStringLiteral("ID9");
			model.setCircuit(c);
			e = evaluate(c, model);
			QVERIFY(!e.indirect.operatingCurrent);
			QCOMPARE(e.indirect.outcome, rebt::CheckOutcome::NotPerformed);
			QVERIFY(std::find(e.indirect.diagnostics.begin(), e.indirect.diagnostics.end(),
							  rebt::Diagnostic{rebt::Issue::MissingResidualDevice, "residualDevice"})
					!= e.indirect.diagnostics.end());
				// one with no sensitivity
			cgmp.residualDevices = {{QStringLiteral("ID9"), std::nullopt, 40.0}};
			model.setBoard(cgmp);
			e = evaluate(c, model);
			QVERIFY(!e.indirect.operatingCurrent);
			QVERIFY(std::find(e.indirect.diagnostics.begin(), e.indirect.diagnostics.end(),
							  rebt::Diagnostic{rebt::Issue::MissingSensitivity, "residualDevice"})
					!= e.indirect.diagnostics.end());
				// an RCBO: its own 30 mA
			c.protection.type = ProtectionType::Rcbo;
			c.protection.sensitivity = 30.0;
			model.setCircuit(c);
			e = evaluate(c, model);
			QCOMPARE(*e.indirect.operatingCurrent, 0.030);
			QCOMPARE(e.indirect.outcome, rebt::CheckOutcome::Passed);
			c.protection.sensitivity.reset();
			model.setCircuit(c);
			e = evaluate(c, model);
			QVERIFY(!e.indirect.operatingCurrent);
			QVERIFY(std::find(e.indirect.diagnostics.begin(), e.indirect.diagnostics.end(),
							  rebt::Diagnostic{rebt::Issue::MissingSensitivity, "residualDevice"})
					!= e.indirect.diagnostics.end());
		}

		void indirectContactTnAndIt()
		{
				// C1 on CS, fed from CGMP by 20 m 10/10 mm2 Cu XLPE;
				// C1 25 m 2.5/2.5 mm2 Cu PVC, C16
			ElectricalModel model = chainModel();
			Circuit f = *model.circuit(id(300));
			f.conductor.material = rebt::Material::Copper;
			f.conductor.insulation = Insulation::XLPE;
			f.conductor.peSection = 10.0;
			model.setCircuit(f);
			Circuit c = *model.circuit(id(301));
			c.conductor.material = rebt::Material::Copper;
			c.conductor.insulation = Insulation::PVC;
			c.conductor.peSection = 2.5;
			c.protection.type = ProtectionType::CircuitBreaker;
			c.protection.ratedCurrent = 16.0;
			c.protection.standard = rebt::ProtectionStandard::EN60898;
			c.protection.curve = QStringLiteral("C");
			model.setCircuit(c);
			Board cgmp = *model.board(id(201));
			cgmp.earthing = rebt::EarthingSystem::TN;
			cgmp.source = rebt::SourceMethod::KnownCurrent;
			cgmp.sourceMaxCurrent = 6.0;
			cgmp.sourceMinCurrent = 5.0;
			cgmp.sourceReason = QStringLiteral("Estudio del transformador");
			model.setBoard(cgmp);
				// TN: 230 / 5000 + 0.132414 + 0.537931 = 0.716345 ohm,
				// x 160 A = 114.62 V <= 230 V
			CircuitEvaluation e = evaluate(c, model);
			QVERIFY(qAbs(*e.indirect.loopImpedance - 0.716344828) < 1e-8);
			QVERIFY(qAbs(*e.indirect.loopVoltage - 114.615172) < 1e-5);
			QCOMPARE(e.indirect.outcome, rebt::CheckOutcome::Passed);
				// the PE of the feeder is asked for by its name
			f.conductor.peSection.reset();
			model.setCircuit(f);
			e = evaluate(c, model);
			QVERIFY(std::find(e.indirect.diagnostics.begin(), e.indirect.diagnostics.end(),
							  rebt::Diagnostic{rebt::Issue::MissingProtectiveSection,
											   "feeder[0]/protectiveSection"})
					!= e.indirect.diagnostics.end());
			QCOMPARE(e.indirect.outcome, rebt::CheckOutcome::NotPerformed);
			f.conductor.peSection = 10.0;
			model.setCircuit(f);

				// IT given on CS: only the circuit, 2 x 0.537931 x 160 A
			Board cs = *model.board(id(202));
			cs.earthing = rebt::EarthingSystem::IT;
			cs.earthResistance = 20.0;
			cs.earthResistanceReason = QStringLiteral("Medida");
			cs.firstFaultCurrent = 0.5;
			cs.firstFaultReason = QStringLiteral("Medida");
			cs.massesInterconnected = true;
			cs.neutralDistributed = false;
			model.setBoard(cs);
			e = evaluate(c, model);
			QCOMPARE(e.firstFaultReason, QStringLiteral("Medida"));
			QVERIFY(qAbs(*e.indirect.loopImpedance - 0.537931034) < 1e-8);
			QVERIFY(qAbs(*e.indirect.loopVoltage - 172.137931) < 1e-5);
			QCOMPARE(e.indirect.firstFault, rebt::CheckOutcome::Passed);
			QCOMPARE(e.indirect.outcome, rebt::CheckOutcome::Passed);
				// IT given on CGMP: the feeder from it counts too
			cs.earthing.reset();
			model.setBoard(cs);
			cgmp = *model.board(id(201));
			cgmp.earthing = rebt::EarthingSystem::IT;
			cgmp.earthResistance = 20.0;
			cgmp.earthResistanceReason = QStringLiteral("Medida");
			cgmp.firstFaultCurrent = 0.5;
			cgmp.firstFaultReason = QStringLiteral("Medida");
			cgmp.massesInterconnected = true;
			cgmp.neutralDistributed = false;
			model.setBoard(cgmp);
			e = evaluate(c, model);
			QVERIFY(qAbs(*e.indirect.loopImpedance - 0.670344828) < 1e-8);
				// the chain broken: CS fed by a circuit that is not there
			{
				ElectricalModel broken = model;
				Board island = *broken.board(id(202));
				island.earthing = rebt::EarthingSystem::TN;
				broken.setBoard(island);
				broken.removeCircuit(id(300));
				const CircuitEvaluation b = evaluate(c, broken);
				QVERIFY(!b.indirect.loopImpedance);
				QCOMPARE(b.indirect.outcome, rebt::CheckOutcome::NotPerformed);
			}
				// TN-C with a residual current device named
			cgmp.earthing = rebt::EarthingSystem::TNC;
			cgmp.residualDevices = {{QStringLiteral("ID1"), 30.0, 40.0}};
			model.setBoard(cgmp);
			c.protection.residualDevice = QStringLiteral("ID1");
			model.setCircuit(c);
			e = evaluate(c, model);
			QCOMPARE(e.indirect.outcome, rebt::CheckOutcome::NotPassed);
			QVERIFY(std::find(e.indirect.diagnostics.begin(), e.indirect.diagnostics.end(),
							  rebt::Diagnostic{rebt::Issue::ResidualInTNC, "residualDevice"})
					!= e.indirect.diagnostics.end());
		}

		void ownTransformerLimits()
		{
			ElectricalModel model = chainModel();
			Circuit c = *model.circuit(id(301));
			c.use = CircuitUse::OtherUses;
			model.setCircuit(c);
				// 3.385690 % from the origin: under 5 % already
			CircuitEvaluation e = evaluate(c, model);
			QCOMPARE(e.limit->percent, 5.0);
			c.conductor.length = 45.0;   // 5.217391 + 0.487139 = 5.704530 %
			model.setCircuit(c);
			e = evaluate(c, model);
			QCOMPARE(e.check.outcome, rebt::CheckOutcome::NotPassed);
				// the same with an own transformer: 6.5 %
			Board cgmp = *model.board(id(201));
			cgmp.ownTransformer = true;
	cgmp.dwellingArea = 95.5;
	cgmp.plannedHeating = false;
	cgmp.plannedAirConditioning = true;
	cgmp.plannedVehicleCharging = false;
	cgmp.singleFamily = true;
	cgmp.generalSwitch = 40.0;
	cgmp.plannedDryer = true;
	cgmp.compensateWithIndividualLine = true;
	{
		RoomData salon;
		salon.name = QStringLiteral("Salón <1>");
		salon.type = rebt::RoomType::LivingRoom;
		salon.area = 24.5;
		salon.counts = {{rebt::RoomItem::LightPoints, 2}, {rebt::RoomItem::Sockets, 0}};
		RoomData pasillo;
		pasillo.type = rebt::RoomType::Corridor;
		pasillo.length = 6.0;
		pasillo.accesses = 3;
		cgmp.rooms = {salon, pasillo};
	}
	cgmp.residualDevices = {{QStringLiteral("ID1"), 30.0, 40.0},
							{QStringLiteral("ID2 <ñ>"), std::nullopt, 63.0}};
			model.setBoard(cgmp);
			e = evaluate(c, model);
			QCOMPARE(e.limit->percent, 6.5);
			QCOMPARE(e.limit->rule.id, std::string("REBT-CDT-006"));
			QVERIFY(qAbs(*e.cumulativePercent - 5.704530) < 1e-5);
			QCOMPARE(e.check.outcome, rebt::CheckOutcome::Passed);
				// a feeder with no drop data does not hide the transformer
			{
				ElectricalModel broken = model;
				Circuit f = *broken.circuit(id(300));
				f.conductor.length.reset();
				broken.setCircuit(f);
				const CircuitEvaluation b = evaluate(c, broken);
				QCOMPARE(b.limit->percent, 6.5);
				QCOMPARE(b.originBoard, id(201));
				QVERIFY(!b.cumulativePercent);
			}
				// lighting: 4.5 %, equal passes ("maximas admisibles")
			c.use = CircuitUse::Lighting;
			model.setCircuit(c);
			e = evaluate(c, model);
			QCOMPARE(e.limit->percent, 4.5);
			QCOMPARE(e.check.outcome, rebt::CheckOutcome::NotPassed);
				// a dwelling keeps 3 %
			c.use = CircuitUse::DwellingInterior;
			model.setCircuit(c);
			QCOMPARE(evaluate(c, model).limit->percent, 3.0);
				// only for the origin: a board fed by an LGA is not
			c.use = CircuitUse::OtherUses;
			model.setCircuit(c);
			Circuit lga;
			lga.id = id(320);
			lga.board = id(321);
			lga.use = CircuitUse::LgaTotallyCentralized;
			model.setCircuit(lga);
			model.setBoard({id(321), QStringLiteral("CGP"), QUuid(), QUuid()});
			cgmp.feeder = lga.id;
			model.setBoard(cgmp);
			QCOMPARE(evaluate(c, model).limit->percent, 5.0);
		}

		void minimumCircuitsPerDwelling()
		{
			ElectricalModel model = chainModel();
				// C1 on the CS, behind the feeder: same dwelling as the CGMP
			Circuit c = *model.circuit(id(301));
			c.dwellingCircuit = rebt::DwellingCircuit::C1;
			model.setCircuit(c);
			rebt::MinimumCircuitsResult r = minimumCircuits(id(201), model);
			QCOMPARE(r.outcome, rebt::CheckOutcome::NotPassed);
			QCOMPARE(r.missing.size(), std::size_t(4));
			int n = 400;
			for (rebt::DwellingCircuit d : {rebt::DwellingCircuit::C2, rebt::DwellingCircuit::C3,
											rebt::DwellingCircuit::C4Single,
											rebt::DwellingCircuit::C5}) {
				Circuit more = c;
				more.id = id(n++);
				more.board = id(201);
				more.dwellingCircuit = d;
				model.setCircuit(more);
			}
			QCOMPARE(minimumCircuits(id(201), model).outcome, rebt::CheckOutcome::Passed);
				// the CS is not an origin: nothing of its own
			QCOMPARE(minimumCircuits(id(202), model).outcome,
					 rebt::CheckOutcome::NotPerformed);
			QVERIFY(isInteriorOrigin(*model.board(id(201)), model));
			QVERIFY(!isInteriorOrigin(*model.board(id(202)), model));
				// a CGMP fed by a DI is still the origin of the dwelling
			Circuit di;
			di.id = id(330);
			di.board = id(331);
			di.use = CircuitUse::DiSingleUser;
			model.setCircuit(di);
			model.setBoard({id(331), QStringLiteral("CPM"), QUuid(), QUuid()});
			Board cgmp = *model.board(id(201));
			cgmp.feeder = di.id;
			model.setBoard(cgmp);
			QVERIFY(isInteriorOrigin(*model.board(id(201)), model));
			QCOMPARE(minimumCircuits(id(201), model).outcome, rebt::CheckOutcome::Passed);
				// a feeder of unknown use: not known to start one
			di.use.reset();
			model.setCircuit(di);
			QVERIFY(!isInteriorOrigin(*model.board(id(201)), model));
		}

		/// A dwelling: CPM -> DI 10 mm2 -> CGMP with C1 to C5 on two devices
		ElectricalModel dwellingModel()
		{
			ElectricalModel model;
			model.setBoard({id(500), QStringLiteral("CPM"), QUuid(), QUuid()});
			Board cgmp{id(501), QStringLiteral("CGMP"), QUuid(), id(510)};
			cgmp.dwellingArea = 90.0;
			cgmp.plannedHeating = false;
			cgmp.plannedAirConditioning = false;
			cgmp.plannedAutomation = false;
			cgmp.plannedVehicleCharging = false;
			cgmp.generalSwitch = 25.0;
			cgmp.plannedDryer = false;
			RoomData bath;
			bath.name = QStringLiteral("Baño");
			bath.type = rebt::RoomType::Bathroom;
			bath.counts = {{rebt::RoomItem::LightPoints, 1}, {rebt::RoomItem::Switches, 1},
						   {rebt::RoomItem::WetSockets, 2}};
			cgmp.rooms = {bath};
			cgmp.residualDevices = {{QStringLiteral("ID1"), 30.0, 40.0}};
			model.setBoard(cgmp);
			Circuit di;
			di.id = id(510);
			di.name = QStringLiteral("DI");
			di.board = id(500);
			di.use = CircuitUse::DiSingleUser;
			di.system = rebt::System::SinglePhase;
			di.conductor.material = rebt::Material::Copper;
			di.conductor.insulation = Insulation::PVC;
			di.conductor.installationMethod = QStringLiteral("B1");
			di.conductor.section = 10.0;
			di.conductor.ambientTemperature = 30.0;
			di.conductor.groupingFactor = 1.0;
			di.conductor.groupingFactorReason = QStringLiteral("solo");
			model.setCircuit(di);
			int n = 520;
			using D = rebt::DwellingCircuit;
			for (D d : {D::C1, D::C2, D::C3, D::C4, D::C5}) {
				Circuit c;
				c.id = id(n++);
				c.name = QStringLiteral("C%1").arg(n - 520);
				c.board = id(501);
				c.use = CircuitUse::DwellingInterior;
				c.dwellingCircuit = d;
				c.protection.residualDevice = QStringLiteral(" ID1 ");
				model.setCircuit(c);
			}
			return model;
		}

		/// chainModel with a DI of a single user feeding the CGMP:
		/// 1~ 230 V, 40 A, cos 1, 12 m, 10 mm2, gamma 48:
		/// dU = 2 x 12 x 40 / (48 x 10) = 2 V = 0.869565 %
		ElectricalModel compensationModel()
		{
			ElectricalModel model = chainModel();
			Circuit di;
			di.id = id(340);
			di.name = QStringLiteral("DI");
			di.board = id(341);
			di.use = CircuitUse::DiSingleUser;
			di.system = rebt::System::SinglePhase;
			di.voltage = 230.0;
			di.load.designCurrent = 40.0;
			di.load.powerFactor = 1.0;
			di.conductor.length = 12.0;
			di.conductor.section = 10.0;
			di.conductor.conductivity = 48.0;
			model.setCircuit(di);
			model.setBoard({id(341), QStringLiteral("CPM"), QUuid(), QUuid()});
			Board cgmp = *model.board(id(201));
			cgmp.feeder = di.id;
			model.setBoard(cgmp);
			return model;
		}

		void compensationWithTheIndividualLine()
		{
			ElectricalModel model = compensationModel();
				// C1: 3.385690 % from the origin, over 3 %
			CircuitEvaluation e = evaluate(*model.circuit(id(301)), model);
			QCOMPARE(e.check.outcome, rebt::CheckOutcome::NotPassed);
			QVERIFY(!e.compensated);
				// compensated: 3.385690 + 0.869565 = 4.255255 % < 3 + 1.5 %
			Board cgmp = *model.board(id(201));
			cgmp.compensateWithIndividualLine = true;
			model.setBoard(cgmp);
			e = evaluate(*model.circuit(id(301)), model);
			QVERIFY(e.compensated);
			QVERIFY(qAbs(*e.individualPercent - 0.869565) < 1e-5);
			QVERIFY(qAbs(*e.compensatedPercent - 4.255255) < 1e-5);
			QCOMPARE(e.limit->percent, 4.5);
			QVERIFY(e.limit->strict);
			QCOMPARE(e.limit->rule.id, std::string("REBT-CDT-007"));
			QCOMPARE(e.check.outcome, rebt::CheckOutcome::Passed);
				// the DI is then checked with the circuits, not alone
			const CircuitEvaluation di = evaluate(*model.circuit(id(340)), model);
			QVERIFY(di.compensated);
			QCOMPARE(di.check.outcome, rebt::CheckOutcome::NotPerformed);
				// a DI whose drop is not known: not performed, not failed
			Circuit broken = *model.circuit(id(340));
			broken.conductor.length.reset();
			model.setCircuit(broken);
			e = evaluate(*model.circuit(id(301)), model);
			QCOMPARE(e.check.outcome, rebt::CheckOutcome::NotPerformed);
			QCOMPARE(e.chainDiagnostics.back().issue, rebt::Issue::MissingIndividualDrop);
				// the dwelling drop with IN too: 3.385690 + 0.869565 < 4.5
			model = compensationModel();
			cgmp = *model.board(id(201));
			cgmp.compensateWithIndividualLine = true;
			model.setBoard(cgmp);
			Circuit c = *model.circuit(id(301));
			c.dwellingCircuit = rebt::DwellingCircuit::C2;
			c.protection.ratedCurrent = 16.0;
			c.conductor.material = rebt::Material::Copper;
			model.setCircuit(c);
			e = evaluate(c, model);
			QCOMPARE(e.dwelling.drop, rebt::CheckOutcome::Passed);
				// not asked for: over 3 %
			cgmp.compensateWithIndividualLine = false;
			model.setBoard(cgmp);
			QCOMPARE(evaluate(c, model).dwelling.drop, rebt::CheckOutcome::NotPassed);
				// 30 m: 2 x 30 x 16 / 120 = 8 V = 3.478261 %, + 0.487139 =
				// 3.965400 % from the origin, under 4.5 % alone, but with the
				// DI 3.965400 + 0.869565 = 4.834965 %: not passed
			cgmp.compensateWithIndividualLine = true;
			model.setBoard(cgmp);
			c.conductor.length = 30.0;
			model.setCircuit(c);
			e = evaluate(c, model);
			QVERIFY(qAbs(*e.cumulativePercent - 3.965400) < 1e-5);
			QVERIFY(qAbs(*e.compensatedPercent - 4.834965) < 1e-5);
			QCOMPARE(e.check.outcome, rebt::CheckOutcome::NotPassed);
			QCOMPARE(e.dwelling.drop, rebt::CheckOutcome::NotPassed);
		}

		void dwellingAsAWhole()
		{
			ElectricalModel model = dwellingModel();
				// the DI: IZ from the tables, synthetic, 60 A at 30 degC
			const rebt::AmpacityLibrary tables = readAmpacityJson(validJson()).library;
			rebt::DwellingCheck r = dwellingCheck(id(501), model, tables);
			QCOMPARE(*r.grade, rebt::Electrification::Basic);
			QCOMPARE(r.individualLine, rebt::CheckOutcome::Passed);
			QCOMPARE(r.residualCount, rebt::CheckOutcome::Passed);
			QCOMPARE(r.residualRating, rebt::CheckOutcome::Passed);
			QCOMPARE(r.outcome, rebt::CheckOutcome::Passed);
				// names compared without the spaces around them
			{
				ElectricalModel spaced = model;
				Board b = *spaced.board(id(501));
				b.residualDevices.first().name = QStringLiteral("ID1  ");
				spaced.setBoard(b);
				QCOMPARE(dwellingCheck(id(501), spaced, tables).outcome,
						 rebt::CheckOutcome::Passed);
			}
				// a subboard: its circuits may name a device upstream,
				// protecting its feeder; the same device counted once
			{
				ElectricalModel sub = model;
				sub.setBoard({id(502), QStringLiteral("CS"), QUuid(), id(523)});
				Circuit c5 = *sub.circuit(id(524));
				c5.board = id(502);
				sub.setCircuit(c5);
				r = dwellingCheck(id(501), sub, tables);
					// the feeder of the CS has no data for its drop: the
					// origin is still the CGMP
				QCOMPARE(evaluate(c5, sub).originBoard, id(501));
				QCOMPARE(r.minimumCircuits, rebt::CheckOutcome::Passed);
				QCOMPARE(r.residualCount, rebt::CheckOutcome::Passed);
				QCOMPARE(r.outcome, rebt::CheckOutcome::Passed);
					// a name the CS's own devices shadow
				Board cs = *sub.board(id(502));
				cs.residualDevices = {{QStringLiteral("ID1"), 300.0, 40.0}};
				sub.setBoard(cs);
				r = dwellingCheck(id(501), sub, tables);
				QCOMPARE(r.residualSensitivity, rebt::CheckOutcome::NotPassed);
			}
				// without tables the DI's IZ is not known
			r = dwellingCheck(id(501), model);
			QCOMPARE(r.individualLine, rebt::CheckOutcome::NotPerformed);
				// a sixth circuit on an RCBO: two devices for six circuits
			Circuit c10;
			c10.id = id(530);
			c10.name = QStringLiteral("C10");
			c10.board = id(501);
			c10.use = CircuitUse::DwellingInterior;
			c10.dwellingCircuit = rebt::DwellingCircuit::C10;
			c10.protection.type = ProtectionType::Rcbo;
			c10.protection.sensitivity = 30.0;
			c10.protection.ratedCurrent = 16.0;
			model.setCircuit(c10);
			r = dwellingCheck(id(501), model, tables);
			QCOMPARE(r.residualNeeded, 2);
			QCOMPARE(r.residualCount, rebt::CheckOutcome::Passed);
				// its 16 A is not compared with the IGA: an RCBO of one circuit
			QCOMPARE(r.residualRating, rebt::CheckOutcome::Passed);
				// a device name the board does not have
			c10.protection.type = ProtectionType::CircuitBreaker;
			c10.protection.residualDevice = QStringLiteral("ID9");
			model.setCircuit(c10);
			r = dwellingCheck(id(501), model, tables);
			QCOMPARE(r.residualCount, rebt::CheckOutcome::NotPerformed);
			QCOMPARE(r.diagnostics.front().issue, rebt::Issue::MissingResidualDevice);
				// air conditioning planned: elevated, C9 and 40 A required
			Board cgmp = *model.board(id(501));
			cgmp.plannedAirConditioning = true;
			model.setBoard(cgmp);
			model.removeCircuit(c10.id);
			r = dwellingCheck(id(501), model, tables);
			QCOMPARE(*r.grade, rebt::Electrification::Elevated);
			QCOMPARE(r.missing, std::vector<rebt::DwellingCircuit>{rebt::DwellingCircuit::C9});
			QCOMPARE(r.generalSwitch, rebt::CheckOutcome::NotPassed);
				// the CPM is not where the dwelling starts
			QVERIFY(dwellingCheck(id(500), model, tables).rules.empty());
		}

		void shortCircuitFieldsRoundTrip()
		{
			const ElectricalModel read = roundTrip(fullModel());
			const Board b = *read.board(id(1));
			QCOMPARE(*b.source, rebt::SourceMethod::KnownCurrent);
			QCOMPARE(*b.sourceMaxCurrent, 4.5);
			QCOMPARE(*b.sourceMinCurrent, 1.5);
			QCOMPARE(b.sourceReason, QStringLiteral("Carta de la distribuidora <ref> & fecha"));
			const ProtectionData p = read.circuit(id(10))->protection;
			QCOMPARE(*p.tripCurrent, 160.0);
			QCOMPARE(*p.letThrough, 20000.0);
			QCOMPARE(p.letThroughReason, QStringLiteral("Curva I²t del catálogo, 3 kA"));
			QVERIFY(b.ownTransformer);
			QCOMPARE(*b.dwellingArea, 95.5);
			QCOMPARE(*b.plannedHeating, false);
			QCOMPARE(*b.plannedAirConditioning, true);
			QVERIFY(!b.plannedAutomation);
			QCOMPARE(*b.generalSwitch, 40.0);
			QCOMPARE(*b.plannedDryer, true);
			QVERIFY(b.compensateWithIndividualLine);
			QCOMPARE(b.rooms.size(), 2);
			QCOMPARE(b.rooms.at(0).name, QStringLiteral("Salón <1>"));
			QCOMPARE(b.rooms.at(0).counts.value(rebt::RoomItem::Sockets, -1), 0);
			QVERIFY(!b.rooms.at(0).counts.contains(rebt::RoomItem::Switches));
			QCOMPARE(*b.rooms.at(1).accesses, 3);
			QCOMPARE(*b.rooms.at(1).type, rebt::RoomType::Corridor);
			QCOMPARE(b.residualDevices.size(), 2);
			QVERIFY(!b.residualDevices.at(1).sensitivity);
			QCOMPARE(*b.earthing, rebt::EarthingSystem::TT);
			QCOMPARE(*b.earthResistance, 37.5);
			QCOMPARE(b.earthResistanceReason,
					 QStringLiteral("Medida con telurómetro <fecha> & firma"));
			QCOMPARE(*b.limitVoltage, 24.0);
			QCOMPARE(b.limitVoltageReason, QStringLiteral("Local mojado"));
			const Board cs = *read.board(id(2));
			QCOMPARE(*cs.earthing, rebt::EarthingSystem::IT);
			QVERIFY(!cs.earthResistance);
			QVERIFY(!cs.limitVoltage);
			QCOMPARE(*cs.firstFaultCurrent, 0.35);
			QCOMPARE(cs.firstFaultReason, QStringLiteral("Medida <IT> & ensayo"));
			QCOMPARE(*cs.massesInterconnected, true);
			QCOMPARE(*cs.neutralDistributed, false);
			QCOMPARE(*read.board(id(3))->earthing, rebt::EarthingSystem::TNC);
			QVERIFY(!read.board(id(3))->massesInterconnected);
			QVERIFY(!b.firstFaultCurrent);
			QCOMPARE(b.residualDevices.at(1).name, QStringLiteral("ID2 <ñ>"));
			QCOMPARE(read.circuit(id(10))->protection.residualDevice, QStringLiteral("ID2 <ñ>"));
			QCOMPARE(*read.circuit(id(10))->conductor.conduit, 20.0);
			const Circuit rc = *read.circuit(id(10));
			QCOMPARE(*rc.dwellingCircuit, rebt::DwellingCircuit::C12C4);
			QCOMPARE(*rc.points, 3);
				// a board with none writes none: older files stay as they were
			ElectricalModel bare;
			bare.setBoard({id(1), QStringLiteral("CGMP"), QUuid(), QUuid()});
			QVERIFY(!bare.toXmlText().contains(QStringLiteral("source")));
			QVERIFY(!bare.toXmlText().contains(QStringLiteral("ownTransformer")));
		}

		void unknownSourceWordIsReported()
		{
			ReadReport report;
			const ElectricalModel m = parse(QStringLiteral(
				"<electrical version=\"1\"><board id=\"%1\" source=\"magic\"/>"
				"</electrical>").arg(id(1).toString()), &report);
			QVERIFY(!m.board(id(1))->source);
			QVERIFY(report.badValues.contains(QStringLiteral("board[0]@source")));
			ReadReport other;
			const ElectricalModel o = parse(QStringLiteral(
				"<electrical version=\"1\"><board id=\"%1\" ownTransformer=\"yes\"/>"
				"<circuit id=\"%2\" dwellingCircuit=\"C14\"/></electrical>")
				.arg(id(1).toString(), id(2).toString()), &other);
			QVERIFY(!o.board(id(1))->ownTransformer);
			QVERIFY(!o.circuit(id(2))->dwellingCircuit);
			QVERIFY(other.badValues.contains(QStringLiteral("board[0]@ownTransformer")));
			QVERIFY(other.badValues.contains(QStringLiteral("circuit[0]@dwellingCircuit")));
		}

	private slots:
		void emptyModelWritesNothing()
		{
			const ElectricalModel model;
			QVERIFY(model.isEmpty());
			QVERIFY(model.toXmlText().isEmpty());
		}

		void fullRoundTrip()
		{
			const ElectricalModel model = fullModel();
			ReadReport report;
			const ElectricalModel read = roundTrip(model, &report);
			QCOMPARE(report.version, 1);
			QVERIFY(!report.tooNew);
			QVERIFY2(report.badValues.isEmpty(),
					 qPrintable(report.badValues.join(QLatin1Char(' '))));
			QVERIFY(read == model);
			QCOMPARE(read.circuits().size(), 2);
			QCOMPARE(read.circuits().first().conductor.wires.size(), 2);
		}

		void unknownValuesStayUnknown()
		{
			ElectricalModel model;
			Circuit bare;
			bare.id = id(5);
			model.setCircuit(bare);

			QDomDocument document;
			const QDomElement root = model.toXml(document);
			const QDomElement circuit = root.firstChildElement(QStringLiteral("circuit"));
				// nothing but the id, no empty children
			QCOMPARE(circuit.attributes().count(), 1);
			QVERIFY(!circuit.hasChildNodes());

			const Circuit read = roundTrip(model).circuits().first();
			QVERIFY(!read.system && !read.voltage && !read.use);
			QVERIFY(!read.load.power && !read.load.powerFactor);
			QVERIFY(!read.conductor.section && !read.conductor.length);
			QVERIFY(!read.conductor.material && !read.conductor.temperature);
			QVERIFY(!read.protection.ratedCurrent && !read.protection.type);
			QVERIFY(read.board.isNull() && read.origin.isNull());
		}

		void numbersAreExactAndLocaleFree()
		{
			const QLocale previous;
			QLocale::setDefault(QLocale(QLocale::German));   // decimal comma
			ElectricalModel model;
			Circuit c;
			c.id = id(6);
			c.voltage = 0.1 + 0.2;
			c.conductor.length = 1.0 / 3.0;
			c.conductor.section = 1e-7;
			c.load.power = 123456789.125;
			model.setCircuit(c);
			const QString text = model.toXmlText();
			QLocale::setDefault(previous);

			QVERIFY2(!text.contains(QLatin1String("0,3")), qPrintable(text));
			const Circuit read = roundTrip(model).circuits().first();
			QCOMPARE(*read.voltage, 0.1 + 0.2);
			QCOMPARE(*read.conductor.length, 1.0 / 3.0);
			QCOMPARE(*read.conductor.section, 1e-7);
			QCOMPARE(*read.load.power, 123456789.125);
		}

		void badValuesAreNotGuessed()
		{
			ReadReport report;
			const ElectricalModel model = parse(QStringLiteral(
				"<electrical version='1'>"
				" <circuit id='%1' voltage='abc' system='two-phase' name='ok'>"
				"  <load power='nan' powerFactor='0.9'/>"
				"  <conductor material='Cu2' section='2,5' length='30'>"
				"   <wire uuid='not-a-uuid'/><wire uuid='%2'/>"
				"  </conductor>"
				"  <protection poles='two' ratedCurrent='16'/>"
				" </circuit>"
				" <circuit id='bad'/>"
				" <circuit id='%1'/>"
				" <circuit/>"
				"</electrical>").arg(id(7).toString(), id(8).toString()), &report);

			QCOMPARE(model.circuits().size(), 1);
			const Circuit c = model.circuits().first();
			QCOMPARE(c.name, QStringLiteral("ok"));
			QVERIFY(!c.voltage);
			QVERIFY(!c.system);
			QVERIFY(!c.load.power);
			QCOMPARE(*c.load.powerFactor, 0.9);
			QVERIFY(!c.conductor.material);
			QVERIFY(!c.conductor.section);   // a decimal comma is not read
			QCOMPARE(*c.conductor.length, 30.0);
			QCOMPARE(c.conductor.wires, QList<QUuid>{id(8)});
			QVERIFY(!c.protection.poles);
			QCOMPARE(*c.protection.ratedCurrent, 16.0);

			const QStringList expected{
				QStringLiteral("circuit[0]@voltage"),
				QStringLiteral("circuit[0]@system"),
				QStringLiteral("circuit[0]/load@power"),
				QStringLiteral("circuit[0]/conductor@material"),
				QStringLiteral("circuit[0]/conductor@section"),
				QStringLiteral("circuit[0]/conductor/wire[0]@uuid"),
				QStringLiteral("circuit[0]/protection@poles"),
				QStringLiteral("circuit[1]@id"),
				QStringLiteral("circuit[2]@id"),
				QStringLiteral("circuit[3]@id")};
			for (const QString &e : expected)
				QVERIFY2(report.badValues.contains(e),
						 qPrintable(e + QLatin1String(" not in ")
									+ report.badValues.join(QLatin1Char(' '))));
		}

		void newerVersionIsKeptReadOnly()
		{
			const QString xml = QStringLiteral(
				"<electrical version=\"2\"><circuit id=\"%1\" future=\"x\">"
				"<thing a=\"1\"/></circuit></electrical>").arg(id(9).toString());
			ReadReport report;
			ElectricalModel model = parse(xml, &report);
			QVERIFY(report.tooNew);
			QCOMPARE(report.version, 2);
			QVERIFY(model.isReadOnly());
			QVERIFY(!model.isEmpty());
			QVERIFY(model.circuits().isEmpty());

			Circuit c;
			c.id = id(12);
			QVERIFY(!model.setCircuit(c));
			QVERIFY(!model.setBoard({id(13), QString(), QUuid(), QUuid()}));

				// written back exactly as read
			QDomDocument a, b;
			a.setContent(xml);
			b.setContent(model.toXmlText());
			QCOMPARE(b.toString(), a.toString());
		}

		void missingVersionIsKeptReadOnly()
		{
			ReadReport report;
			const ElectricalModel model = parse(
				QStringLiteral("<electrical><circuit/></electrical>"), &report);
			QVERIFY(model.isReadOnly());
			QVERIFY(!report.tooNew);
			QVERIFY(report.badValues.contains(QStringLiteral("electrical@version")));
		}

		void setReplacesAndRemoves()
		{
			ElectricalModel model = fullModel();
			Circuit c = fullCircuit();
			c.name = QStringLiteral("renamed");
			QVERIFY(model.setCircuit(c));
			QCOMPARE(model.circuits().size(), 2);
			QCOMPARE(model.circuit(c.id)->name, QStringLiteral("renamed"));
			QVERIFY(model.removeCircuit(c.id));
			QVERIFY(!model.circuit(c.id));
			QVERIFY(!model.removeCircuit(c.id));
			QVERIFY(!model.setCircuit(Circuit()));   // null id
		}

		void referencesDanglingAndOrphans()
		{
			ElectricalModel model = fullModel();
			const QSet<QUuid> refs = model.referencedItems();
			const QSet<QUuid> expected{id(100), id(101), id(102), id(103),
									   id(200), id(201)};
			QCOMPARE(refs, expected);

			const QSet<QUuid> drawn{id(100), id(101), id(102), id(200)};
			QCOMPARE(model.danglingItems(drawn), (QSet<QUuid>{id(103), id(201)}));

			QVERIFY(model.orphanCircuits().isEmpty());
			model.removeBoard(id(1));
			QCOMPARE(model.orphanCircuits().size(), 2);
		}

		void feedsTheCalculation()
		{
				// case CDT-M1 of docs/rebt/casos_h1.md
			Circuit c;
			c.id = id(20);
			c.system = rebt::System::SinglePhase;
			c.voltage = 230.0;
			c.load.powerFactor = 1.0;
			c.load.designCurrent = 16.0;
			c.conductor.length = 25.0;
			c.conductor.section = 2.5;
			c.conductor.conductivity = 48.0;
			const ElectricalModel read = roundTrip([&c] {
				ElectricalModel m; m.setCircuit(c); return m; }());

			const rebt::VoltageDropResult r =
				rebt::voltageDrop(toCircuitInput(read.circuits().first()));
			QCOMPARE(r.status, rebt::Status::Ok);
			QVERIFY(qAbs(*r.percent - 2.898551) < 2.898551e-3);
		}

		void parseNumber_data()
		{
			QTest::addColumn<QString>("text");
			QTest::addColumn<bool>("ok");
			QTest::addColumn<bool>("known");
			QTest::addColumn<double>("value");

			QTest::newRow("empty") << QString() << true << false << 0.0;
			QTest::newRow("spaces") << QStringLiteral("   ") << true << false << 0.0;
			QTest::newRow("point") << QStringLiteral("2.5") << true << true << 2.5;
			QTest::newRow("comma") << QStringLiteral("2,5") << true << true << 2.5;
			QTest::newRow("trimmed") << QStringLiteral(" 230 ") << true << true << 230.0;
			QTest::newRow("negative") << QStringLiteral("-0,5") << true << true << -0.5;
			QTest::newRow("both separators") << QStringLiteral("1.234,5") << false << false << 0.0;
			QTest::newRow("two commas") << QStringLiteral("2,5,1") << false << false << 0.0;
			QTest::newRow("letters") << QStringLiteral("abc") << false << false << 0.0;
			QTest::newRow("unit") << QStringLiteral("25 m") << false << false << 0.0;
			QTest::newRow("inf") << QStringLiteral("inf") << false << false << 0.0;
			QTest::newRow("nan") << QStringLiteral("nan") << false << false << 0.0;
		}

		void parseNumber()
		{
			QFETCH(QString, text);
			QFETCH(bool, ok);
			QFETCH(bool, known);
			QFETCH(double, value);
			bool parsed_ok = !ok;
			const auto parsed = electrical::parseNumber(text, &parsed_ok);
			QCOMPARE(parsed_ok, ok);
			QCOMPARE(parsed.has_value(), known);
			if (known)
				QCOMPARE(*parsed, value);
		}

		void formatNumberHasNoGroupSeparator()
		{
			const QLocale previous;
			QLocale::setDefault(QLocale(QLocale::Spanish, QLocale::Spain));
			QCOMPARE(formatNumber(3680.0), QStringLiteral("3680"));
			QCOMPARE(formatNumber(2.5), QStringLiteral("2,5"));
			QCOMPARE(formatNumber(2.898551, 2), QStringLiteral("2,90"));
			QCOMPARE(formatNumber(std::nullopt), QString());
			bool ok = false;
			QCOMPARE(*electrical::parseNumber(formatNumber(1234567.25), &ok), 1234567.25);
			QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
			QCOMPARE(formatNumber(12345.5), QStringLiteral("12345.5"));
			QLocale::setDefault(previous);
		}

		void limitFollowsTheUse()
		{
			Circuit c;
			QVERIFY(!limitFor(c));
			c.use = CircuitUse::DwellingInterior;
			QCOMPARE(limitFor(c)->rule.id, std::string("REBT-CDT-002"));
			c.use = CircuitUse::Lighting;
			QCOMPARE(limitFor(c)->rule.id, std::string("REBT-CDT-003"));
			c.use = CircuitUse::OtherUses;
			QCOMPARE(limitFor(c)->percent, 5.0);
		}

		void evaluationOfACircuit()
		{
			Circuit c;
			c.id = id(30);
			c.system = rebt::System::SinglePhase;
			c.voltage = 230.0;
			c.load.power = 3680.0;
			c.load.powerFactor = 1.0;
			c.conductor.length = 25.0;
			c.conductor.section = 2.5;
			c.conductor.conductivity = 48.0;

				// no use: computed, but nothing to check against
			CircuitEvaluation e = evaluate(c);
			QCOMPARE(*e.designCurrent(), 16.0);
			QCOMPARE(e.drop.status, rebt::Status::Ok);
			QVERIFY(!e.limit);
			QCOMPARE(e.check.outcome, rebt::CheckOutcome::NotPerformed);

			c.use = CircuitUse::DwellingInterior;
			e = evaluate(c);
			QCOMPARE(e.check.outcome, rebt::CheckOutcome::Passed);

				// a current given wins over the power
			c.load.designCurrent = 20.0;
			e = evaluate(c);
			QCOMPARE(*e.designCurrent(), 20.0);
			QCOMPARE(*e.current.current, 20.0);

				// the drop cannot be computed, the current still can
			c.load.designCurrent.reset();
			c.conductor.section.reset();
			e = evaluate(c);
			QCOMPARE(e.drop.status, rebt::Status::NotComputable);
			QCOMPARE(*e.designCurrent(), 16.0);
			QCOMPARE(e.check.outcome, rebt::CheckOutcome::NotPerformed);
		}

		void ampacityFieldsReachTheEngine()
		{
			const rebt::AmpacityInput in = toAmpacityInput(fullCircuit());
			QCOMPARE(*in.loadedConductors, 2);
			QCOMPARE(in.method, std::string("B1"));
			QCOMPARE(*in.ambient, 40.0);
			QCOMPARE(*in.temperatureFactor, 0.87);
			QCOMPARE(in.groupingJustification, std::string("2 circuitos en el tubo"));
			QCOMPARE(*in.otherFactor, 0.95);
			QCOMPARE(*in.material, rebt::Material::Copper);
			QCOMPARE(*in.insulation, rebt::Insulation::XLPE);
		}

		void readsAValidFile()
		{
			const AmpacityFile f = readAmpacityJson(validJson());
			QVERIFY2(f.errors.isEmpty(), qPrintable(f.errors.join('\n')));
			QCOMPARE(f.library.tables().size(), size_t(2));
			const rebt::AmpacityTable *t = f.library.find(
				rebt::Material::Copper, rebt::Insulation::PVC, 2, "B1");
			QVERIFY(t);
			QCOMPARE(t->source, std::string("synthetic test data"));
			QCOMPARE(t->referenceAmbient, 30.0);
			QCOMPARE(t->entries.size(), size_t(5));
			QCOMPARE(t->entries.front().section, 1.5);   // sorted
			QCOMPARE(*t->currentAt(2.5), 20.0);
		}

		void refusesWhatIsWrong_data()
		{
			QTest::addColumn<QByteArray>("json");
			QTest::addColumn<QString>("error");
			QTest::addColumn<int>("tables");

			const QByteArray head = "{\"format\":\"qet-ampacity\",\"version\":1,\"tables\":[";
			const QByteArray good = "{\"id\":\"OK\",\"source\":\"s\",\"material\":\"Al\","
									"\"insulation\":\"XLPE\",\"loadedConductors\":3,\"method\":\"E\","
									"\"referenceAmbient\":30,\"entries\":[[16,50],[25,70]]}";
			auto with = [&](const QByteArray &table) { return head + table + "," + good + "]}"; };

			QTest::newRow("not json") << QByteArray("{oops") << "Not a JSON file" << 0;
			QTest::newRow("format") << QByteArray("{\"format\":\"x\",\"version\":1,\"tables\":[]}")
									<< "format" << 0;
			QTest::newRow("version") << QByteArray("{\"format\":\"qet-ampacity\",\"version\":2,\"tables\":[]}")
									 << "Version 2" << 0;
			QTest::newRow("no table") << QByteArray("{\"format\":\"qet-ampacity\",\"version\":1,\"tables\":[]}")
									  << "no table" << 0;
			QByteArray t = good;
			QTest::newRow("no source") << with(QByteArray(t).replace("\"source\":\"s\",", "").replace("OK", "A"))
									   << "source" << 1;
			QTest::newRow("material") << with(QByteArray(t).replace("\"Al\"", "\"Fe\"").replace("OK", "B"))
									  << "material" << 1;
			QTest::newRow("insulation") << with(QByteArray(t).replace("XLPE", "Rubber").replace("OK", "C"))
										<< "insulation" << 1;
			QTest::newRow("loaded") << with(QByteArray(t).replace("\"loadedConductors\":3", "\"loadedConductors\":4"))
									<< "loadedConductors" << 1;
			QTest::newRow("negative") << with(QByteArray(t).replace("[16,50]", "[16,-50]").replace("\"method\":\"E\"", "\"method\":\"F\""))
									  << "entry 1" << 1;
			QTest::newRow("twice") << with(QByteArray(t).replace("[25,70]", "[16,70]").replace("\"method\":\"E\"", "\"method\":\"F\""))
								   << "appears twice" << 1;
			QTest::newRow("typo") << with(QByteArray(t).replace("[25,70]", "[25,40]").replace("\"method\":\"E\"", "\"method\":\"F\""))
								  << "a typo?" << 1;
			QTest::newRow("same conditions") << with(QByteArray(t).replace("OK", "DUP"))
											 << "same material" << 1;
		}

		void refusesWhatIsWrong()
		{
			QFETCH(QByteArray, json);
			QFETCH(QString, error);
			QFETCH(int, tables);
			const AmpacityFile f = readAmpacityJson(json);
			QVERIFY2(f.errors.join('\n').contains(error), qPrintable(f.errors.join('\n')));
				// the wrong table is left out whole, the good one kept
			QCOMPARE(int(f.library.tables().size()), tables);
		}

		void tablesFollowTheSetting()
		{
			QCoreApplication::setOrganizationName(QStringLiteral("QElectroTechTest"));
			QCoreApplication::setApplicationName(QStringLiteral("tst_electricalmodel"));
			QTemporaryDir dir;
			QSettings::setDefaultFormat(QSettings::IniFormat);
			QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.filePath("settings"));

			AmpacityTables::setFilePath(QString());
			QVERIFY(AmpacityTables::current().library.empty());
			QVERIFY(AmpacityTables::current().errors.isEmpty());

			const QString path = dir.filePath("tables.json");
			QFile file(path);
			QVERIFY(file.open(QIODevice::WriteOnly));
			file.write(validJson());
			file.close();
			AmpacityTables::setFilePath(path);
			QCOMPARE(AmpacityTables::current().library.tables().size(), size_t(2));

				// changed on disk: read again
			QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
			file.write(QByteArray(validJson()).replace("\"id\": \"TEST-XLPE3-E\"", "\"id\": \"\""));
			file.close();
			QCOMPARE(AmpacityTables::current().library.tables().size(), size_t(1));
			QCOMPARE(AmpacityTables::current().errors.size(), 1);

			AmpacityTables::setFilePath(dir.filePath("missing.json"));
			QVERIFY(AmpacityTables::current().library.empty());
			QVERIFY(AmpacityTables::current().errors.first().contains("Cannot read"));
			AmpacityTables::setFilePath(QString());
		}

		void evaluationWithTables()
		{
				// case A of docs/rebt/tablas_h4.md: 16 A in 2.5 mm2, PVC B1 at 40 degC
			Circuit c;
			c.id = id(40);
			c.system = rebt::System::SinglePhase;
			c.voltage = 230.0;
			c.use = CircuitUse::DwellingInterior;
			c.load.power = 3680.0;
			c.load.powerFactor = 1.0;
			c.conductor.length = 40.0;
			c.conductor.section = 2.5;
			c.conductor.conductivity = 48.0;
			c.conductor.material = rebt::Material::Copper;
			c.conductor.insulation = rebt::Insulation::PVC;
			c.conductor.installationMethod = QStringLiteral("B1");
			c.conductor.ambientTemperature = 40.0;
			c.conductor.groupingFactor = 1.0;
			c.conductor.groupingFactorReason = QStringLiteral("circuito único");

			const rebt::AmpacityLibrary tables = readAmpacityJson(validJson()).library;
			CircuitEvaluation e = evaluate(c, tables);
			QCOMPARE(e.ampacity.status, rebt::Status::Ok);
			QVERIFY(qAbs(*e.ampacity.admissible - 17.320508) < 1e-3);
			QCOMPARE(e.ampacityCheck.outcome, rebt::CheckOutcome::Passed);
				// 40 m: 2.5 mm2 fails the drop, 4 mm2 is suggested
			QCOMPARE(e.check.outcome, rebt::CheckOutcome::NotPassed);
			QCOMPARE(*e.suggestion.section, 4.0);
			QVERIFY(e.suggestion.dropConsidered);

				// no tables: no IZ, and said why
			e = evaluate(c);
			QCOMPARE(e.ampacity.status, rebt::Status::NotComputable);
			QCOMPARE(e.ampacityCheck.outcome, rebt::CheckOutcome::NotPerformed);
			QVERIFY(!e.suggestion.section);
		}

		void everyUseRoundTrips()
		{
			const CircuitUse uses[] = {
				CircuitUse::DwellingInterior, CircuitUse::Lighting, CircuitUse::OtherUses,
				CircuitUse::LgaTotallyCentralized, CircuitUse::LgaPartlyCentralized,
				CircuitUse::DiMetersInSeveralPlaces, CircuitUse::DiMetersTotallyGrouped,
				CircuitUse::DiSingleUser};
			const rebt::ProtectionStandard standards[] = {
				rebt::ProtectionStandard::EN60898, rebt::ProtectionStandard::EN61009,
				rebt::ProtectionStandard::EN60947_2, rebt::ProtectionStandard::FuseGG};
			ElectricalModel model;
			int n = 50;
			for (CircuitUse use : uses) {
				Circuit c;
				c.id = id(n++);
				c.use = use;
				c.protection.standard = standards[n % 4];
				model.setCircuit(c);
				QVERIFY(limitFor(c));
			}
			ReadReport report;
			QVERIFY(roundTrip(model, &report) == model);
			QVERIFY(report.badValues.isEmpty());
			QVERIFY(isInterior(CircuitUse::Lighting));
			QVERIFY(!isInterior(CircuitUse::DiSingleUser));
			Circuit di;
			di.use = CircuitUse::DiSingleUser;
			QCOMPARE(limitFor(di)->percent, 1.5);
			QCOMPARE(limitFor(di)->rule.source, std::string("ITC-BT-15"));
		}

		void dropAddsUpFromTheOrigin()
		{
				// docs/rebt/coordinacion_h5.md: feeder 0.487139 %, circuit 2.898551 %
			ElectricalModel model = chainModel();
			const Circuit c = *model.circuit(id(301));
			CircuitEvaluation e = evaluate(c, model);
			QVERIFY(e.chainEvaluated);
			QVERIFY(qAbs(*e.drop.percent - 2.898551) < 1e-4);
			QVERIFY(qAbs(*e.cumulativePercent - 3.385690) < 1e-4);
			QCOMPARE(e.feeders, QList<QUuid>{id(300)});
			QCOMPARE(e.originBoard, id(201));
				// alone it would pass 3 %, from the origin it does not
			QCOMPARE(evaluate(c).check.outcome, rebt::CheckOutcome::Passed);
			QCOMPARE(e.check.outcome, rebt::CheckOutcome::NotPassed);
			QVERIFY(qAbs(*e.check.value - 3.385690) < 1e-4);

				// other uses: 5 %, passed
			Circuit other = c;
			other.use = CircuitUse::OtherUses;
			model.setCircuit(other);
			QCOMPARE(evaluate(other, model).check.outcome, rebt::CheckOutcome::Passed);
		}

		void suggestionCountsTheFeeders()
		{
				// C1 of the chain with the data for IZ: 2.5 mm2 carries it and
				// its own drop is 2.90 %, but from the origin 3.39 %: over 3 %.
				// What is left for it is 3 - 0.49 = 2.51 %: 4 mm2 (1.81 %)
			ElectricalModel model = chainModel();
			Circuit c = *model.circuit(id(301));
			c.conductor.material = rebt::Material::Copper;
			c.conductor.insulation = rebt::Insulation::PVC;
			c.conductor.installationMethod = QStringLiteral("B1");
			c.conductor.ambientTemperature = 40.0;
			c.conductor.groupingFactor = 1.0;
			c.conductor.groupingFactorReason = QStringLiteral("único");
			model.setCircuit(c);
			const rebt::AmpacityLibrary tables = readAmpacityJson(validJson()).library;
				// alone, 2.5 mm2 would do
			QCOMPARE(*evaluate(c, tables).suggestion.section, 2.5);
			const CircuitEvaluation e = evaluate(c, model, tables);
			QCOMPARE(*e.suggestion.section, 4.0);
			QVERIFY(e.suggestion.dropConsidered);
			QVERIFY(qAbs(*e.suggestion.dropPercent - 1.811594) < 1e-4);

				// a feeder of unknown drop: by admissible current only, and said so
			Circuit f = *model.circuit(id(300));
			f.conductor.length.reset();
			model.setCircuit(f);
			const CircuitEvaluation u = evaluate(c, model, tables);
			QCOMPARE(*u.suggestion.section, 2.5);
			QVERIFY(!u.suggestion.dropConsidered);
		}

		void twoLevelsOfFeeders()
		{
			ElectricalModel model = chainModel();
				// a third board fed from the second
			Circuit f2 = *model.circuit(id(300));
			f2.id = id(302);
			f2.board = id(202);
			model.setCircuit(f2);
			model.setBoard({id(203), QStringLiteral("CS2"), QUuid(), id(302)});
			Circuit c = *model.circuit(id(301));
			c.id = id(303);
			c.board = id(203);
			model.setCircuit(c);
			const CircuitEvaluation e = evaluate(c, model);
			QVERIFY(qAbs(*e.cumulativePercent - 3.872829) < 1e-4);
			QCOMPARE(e.feeders, (QList<QUuid>{id(302), id(300)}));
			QCOMPARE(e.originBoard, id(201));
		}

		void serviceConnectionEndsTheChain()
		{
			ElectricalModel model = chainModel();
				// CGMP fed by a DI: not added to the interior drop
			Circuit di;
			di.id = id(310);
			di.board = id(200);
			di.use = CircuitUse::DiSingleUser;
			model.setCircuit(di);   // its data unknown: it does not matter
			Board cgmp = *model.board(id(201));
			cgmp.feeder = di.id;
			model.setBoard(cgmp);
			model.setBoard({id(200), QStringLiteral("CPM"), QUuid(), QUuid()});
			const CircuitEvaluation e = evaluate(*model.circuit(id(301)), model);
			QVERIFY(qAbs(*e.cumulativePercent - 3.385690) < 1e-4);
			QCOMPARE(e.originBoard, id(201));
			QVERIFY(e.chainDiagnostics.empty());

				// the DI itself: its own drop against its own limit, 1.5 %
			Circuit full = di;
			full.system = rebt::System::SinglePhase;
			full.voltage = 230.0;
			full.load.designCurrent = 16.0;
			full.load.powerFactor = 1.0;
			full.conductor.length = 10.0;
			full.conductor.section = 10.0;
			full.conductor.conductivity = 48.0;
			model.setCircuit(full);
			const CircuitEvaluation d = evaluate(full, model);
			QCOMPARE(*d.cumulativePercent, *d.drop.percent);
			QCOMPARE(d.check.outcome, rebt::CheckOutcome::Passed);
			QCOMPARE(d.limit->percent, 1.5);
		}

		void chainProblemsAreReported()
		{
				// a feeder of unknown use
			ElectricalModel model = chainModel();
			Circuit f = *model.circuit(id(300));
			f.use.reset();
			model.setCircuit(f);
			CircuitEvaluation e = evaluate(*model.circuit(id(301)), model);
			QVERIFY(!e.cumulativePercent);
			QCOMPARE(e.chainDiagnostics.front().issue, rebt::Issue::FeederUseUnknown);
			QCOMPARE(e.chainDiagnostics.front().field, std::string("Alimentación CS"));
				// own drop passes: the complete check cannot be done
			QCOMPARE(e.check.outcome, rebt::CheckOutcome::NotPerformed);

				// a feeder whose drop is unknown
			model = chainModel();
			f = *model.circuit(id(300));
			f.conductor.length.reset();
			model.setCircuit(f);
			e = evaluate(*model.circuit(id(301)), model);
			QCOMPARE(e.chainDiagnostics.front().issue, rebt::Issue::FeederNotComputable);
			QCOMPARE(e.check.outcome, rebt::CheckOutcome::NotPerformed);

				// own drop already too much: not passed whatever the feeders
			Circuit longer = *model.circuit(id(301));
			longer.conductor.length = 40.0;
			model.setCircuit(longer);
			QCOMPARE(evaluate(longer, model).check.outcome, rebt::CheckOutcome::NotPassed);

				// boards feeding each other
			model = chainModel();
			Board cgmp = *model.board(id(201));
			Circuit back = *model.circuit(id(301));
			back.id = id(320);
			model.setCircuit(back);   // in CS
			cgmp.feeder = back.id;
			model.setBoard(cgmp);
			e = evaluate(*model.circuit(id(301)), model);
			QCOMPARE(e.chainDiagnostics.front().issue, rebt::Issue::FeederLoop);
			QVERIFY(!e.cumulativePercent);
		}

		void coordinationInTheEvaluation()
		{
			Circuit c;
			c.id = id(60);
			c.system = rebt::System::SinglePhase;
			c.voltage = 230.0;
			c.load.designCurrent = 16.0;
			c.conductor.material = rebt::Material::Copper;
			c.conductor.insulation = rebt::Insulation::PVC;
			c.conductor.installationMethod = QStringLiteral("B1");
			c.conductor.section = 2.5;
			c.conductor.ambientTemperature = 40.0;
			c.conductor.groupingFactor = 1.0;
			c.conductor.groupingFactorReason = QStringLiteral("único");
			c.protection.type = ProtectionType::CircuitBreaker;
			c.protection.standard = rebt::ProtectionStandard::EN60898;
			c.protection.ratedCurrent = 16.0;
			const rebt::AmpacityLibrary tables = readAmpacityJson(validJson()).library;
				// P1: 16 <= 16 <= 17.32, 23.2 <= 25.11
			CircuitEvaluation e = evaluate(c, tables);
			QCOMPARE(e.coordination.outcome, rebt::CheckOutcome::Passed);
			QCOMPARE(*e.coordination.conventionalCurrent, 23.2);
				// 20 A breaker on 17.32 A of cable: not passed
			c.protection.ratedCurrent = 20.0;
			QCOMPARE(evaluate(c, tables).coordination.outcome, rebt::CheckOutcome::NotPassed);
				// a residual current device does not protect the cable
			c.protection.type = ProtectionType::ResidualCurrent;
			e = evaluate(c, tables);
			QCOMPARE(e.coordination.diagnostics.front().issue, rebt::Issue::NoOverloadProtection);
				// without tables, IZ is unknown
			c.protection.type = ProtectionType::CircuitBreaker;
			c.protection.ratedCurrent = 16.0;
			e = evaluate(c);
			QCOMPARE(e.coordination.ibIn, rebt::CheckOutcome::Passed);
			QCOMPARE(e.coordination.outcome, rebt::CheckOutcome::NotPerformed);
		}

		void missingDataReachesTheCalculation()
		{
			Circuit c;
			c.id = id(21);
			c.system = rebt::System::ThreePhase;
			const rebt::VoltageDropResult r = rebt::voltageDrop(toCircuitInput(c));
			QCOMPARE(r.status, rebt::Status::NotComputable);
			QVERIFY(!r.percent);
		}
};

QTEST_GUILESS_MAIN(tst_electricalmodel)
#include "tst_electricalmodel.moc"
