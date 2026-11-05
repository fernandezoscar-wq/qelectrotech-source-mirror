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
#include "qetapptest.h"
#include "electrical/ampacityfile.h"
#include "electrical/report/electricalreport.h"
#include "electrical/report/reportlogo.h"
#include "electrical/report/reportpdfwriter.h"
#include "electrical/ui/electricalcircuitsdock.h"
#include "electrical/ui/reportdialog.h"
#include "qetproject.h"
#include "qetresult.h"

#include <QBuffer>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTranslator>
#include <QUndoStack>

using namespace electrical;

namespace {

/// SYNTHETIC values, round on purpose: not a table of any standard
QByteArray syntheticTables()
{
	return R"({"format": "qet-ampacity", "version": 1, "tables": [
		{"id": "TEST-PVC2-B1", "source": "synthetic test data", "material": "Cu",
		 "insulation": "PVC", "loadedConductors": 2, "method": "B1",
		 "referenceAmbient": 30,
		 "entries": [[1.5, 10], [2.5, 20], [4, 30], [6, 40], [10, 60]]}]})";
}

/// CGMP -> feeder (0.49 %) -> CS -> C1 (2.90 %, 3.39 % from the origin),
/// the chain of docs/rebt/coordinacion_h5.md, with data for IZ
ElectricalModel chainModel(int extra_circuits = 0)
{
	ElectricalModel model;
	const QUuid cgmp = QUuid::createUuid(), cs = QUuid::createUuid();
	Circuit f;
	f.id = QUuid::createUuid();
	f.name = QStringLiteral("Alimentación CS");
	f.board = cgmp;
	f.use = CircuitUse::OtherUses;
	f.system = rebt::System::ThreePhase;
	f.voltage = 400.0;
	f.load.designCurrent = 30.0;
	f.load.powerFactor = 0.9;
	f.conductor.length = 20.0;
	f.conductor.section = 10.0;
	f.conductor.conductivity = 48.0;
	model.setBoard({cgmp, QStringLiteral("CGMP"), QUuid(), QUuid()});
	model.setBoard({cs, QStringLiteral("CS"), QUuid(), f.id});
	model.setCircuit(f);
	Circuit c;
	c.id = QUuid::createUuid();
	c.name = QStringLiteral("C1 Alumbrado");
	c.board = cs;
	c.use = CircuitUse::DwellingInterior;
	c.system = rebt::System::SinglePhase;
	c.voltage = 230.0;
	c.load.designCurrent = 16.0;
	c.load.powerFactor = 1.0;
	c.conductor.length = 25.0;
	c.conductor.section = 2.5;
	c.conductor.conductivity = 48.0;
	c.conductor.material = rebt::Material::Copper;
	c.conductor.insulation = rebt::Insulation::PVC;
	c.conductor.installationMethod = QStringLiteral("B1");
	c.conductor.ambientTemperature = 40.0;
	c.conductor.groupingFactor = 1.0;
	c.conductor.groupingFactorReason = QStringLiteral("circuito único");
	c.protection.type = ProtectionType::CircuitBreaker;
	c.protection.standard = rebt::ProtectionStandard::EN60898;
	c.protection.ratedCurrent = 16.0;
	c.protection.curve = QStringLiteral("C");
	c.notes = QStringLiteral("Longitud medida en obra");
	model.setCircuit(c);
	for (int i = 0; i < extra_circuits; ++i) {
		Circuit more = c;
		more.id = QUuid::createUuid();
		more.name = QStringLiteral("Toma %1").arg(i + 1);
		model.setCircuit(more);
	}
	ReportInfo info;
	info.company = QStringLiteral("EMPRESA DE PRUEBA SL");
	info.planNumber = QStringLiteral("E-01");
	info.revision = QStringLiteral("B");
	info.date = QStringLiteral("03/11/2026");
	info.author = QStringLiteral("Autor de prueba");
	model.setReportInfo(info);
	return model;
}

/// A PNG of @p size, two colours: a stand-in logo
QByteArray pngOf(const QSize &size)
{
	QImage image(size, QImage::Format_ARGB32);
	image.fill(QColor(0x0a, 0x70, 0xb0));
	QPainter p(&image);
	p.fillRect(QRect(QPoint(0, 0), size / 2), QColor(0xf0, 0xa0, 0x20));
	p.end();
	QByteArray png;
	QBuffer buffer(&png);
	buffer.open(QIODevice::WriteOnly);
	image.save(&buffer, "PNG");
	return png;
}

rebt::AmpacityLibrary tables()
{
	return readAmpacityJson(syntheticTables()).library;
}

/// The text of a PDF, empty when pdftotext is not there
QString pdfText(const QString &path, int page = 0)
{
	QProcess p;
	QStringList args{QStringLiteral("-layout")};
	if (page > 0)
		args << QStringLiteral("-f") << QString::number(page)
			 << QStringLiteral("-l") << QString::number(page);
	p.start(QStringLiteral("pdftotext"), args << path << QStringLiteral("-"));
	if (!p.waitForFinished(30000))
		return QString();
	return QString::fromUtf8(p.readAllStandardOutput());
}

/// "Pages:" and "Page size:" of pdfinfo
QString pdfInfo(const QString &path, const QString &key)
{
	QProcess p;
	p.start(QStringLiteral("pdfinfo"), {path});
	p.waitForFinished(30000);
	for (const QString &line : QString::fromUtf8(p.readAllStandardOutput()).split('\n'))
		if (line.startsWith(key))
			return line.mid(key.size()).trimmed();
	return QString();
}

bool havePdfTools()
{
	return !QStandardPaths::findExecutable(QStringLiteral("pdftotext")).isEmpty()
		   && !QStandardPaths::findExecutable(QStringLiteral("pdfinfo")).isEmpty();
}

} // namespace

class tst_electricalreport : public QObject
{
		Q_OBJECT

	private slots:
		void initTestCase()
		{
			QVERIFY(m_dir.isValid());
			QLocale::setDefault(QLocale(QLocale::Spanish, QLocale::Spain));
		}

		void pvInTheReport()
		{
			ElectricalModel model = chainModel();
			ElectricalReport r = ElectricalReportBuilder::build(
				model, tables(), QStringLiteral("t.json"), QStringLiteral("Vivienda"));
			QCOMPARE(r.pv.passed + r.pv.notPassed + r.pv.notPerformed, 0);
				// the test example of tst_rebtpv on C1, 14 modules: over 600 V
			for (Circuit c : model.circuits()) {
				if (c.name != QLatin1String("C1 Alumbrado")) continue;
				PvData &p = c.pv;
				p.openCircuitVoltage = 42.5; p.mppVoltage = 37.0;
				p.shortCircuitCurrent = 9.18; p.mppCurrent = 8.64;
				p.vocCoefficient = -0.29; p.vmppCoefficient = -0.37;
				p.maxSystemVoltage = 1000.0; p.maxSeriesFuse = 15.0;
				p.moduleSource = QStringLiteral("Hoja");
				p.maxDcVoltage = 600.0; p.mpptMinVoltage = 90.0; p.mpptMaxVoltage = 550.0;
				p.maxInputCurrent = 22.0; p.maxShortCircuitCurrent = 27.5;
				p.inverterSource = QStringLiteral("Ficha");
				p.minCellTemperature = -10.0; p.maxCellTemperature = 70.0;
				p.temperatureReason = QStringLiteral("Sitio");
				PvStringGroup g;
				g.name = QStringLiteral("MPPT1");
				g.modulesPerString = 14; g.parallelStrings = 2;
				g.cableLength = 30.0; g.cableSection = 4.0;
				g.cableMaterial = rebt::Material::Copper; g.conductorTemperature = 90.0;
				g.cableAdmissible = 20.0; g.cableAdmissibleReason = QStringLiteral("Tabla");
				p.strings = {g};
				model.setCircuit(c);
			}
			r = ElectricalReportBuilder::build(model, tables(), QStringLiteral("t.json"),
											   QStringLiteral("Vivienda"));
			QCOMPARE(r.pv.notPassed, 1);
			bool found = false;
			for (const CircuitNotes &n : r.notes) {
				const QString text = n.lines.join('\n');
				if (!text.contains(QStringLiteral("Photovoltaic generator, DC side: Not passed")))
					continue;
				found = true;
				QVERIFY(n.hasProblem);
				QVERIFY2(text.contains(QStringLiteral("UOC MAX 655,4 V")), qPrintable(text));
			}
			QVERIFY(found);
			bool cited = false;
			for (const rebt::NormRef &n : r.rules)
				cited = cited || n.id == "REBT-FV-001";
			QVERIFY(cited);
		}

		void indirectContactsInTheReport()
		{
			ElectricalModel model = chainModel();
				// 5 m: every other check of C1 passes or is not performed
			for (Circuit c : model.circuits()) {
				if (c.name == QLatin1String("C1 Alumbrado")) {
					c.conductor.length = 5.0;
					model.setCircuit(c);
				}
			}
				// no earthing given: the column says it, nothing else
			ElectricalReport r = ElectricalReportBuilder::build(
				model, tables(), QStringLiteral("t.json"), QStringLiteral("Vivienda"));
			QCOMPARE(r.columns.last(), QStringLiteral("ITC-BT-24"));
			QVERIFY(!r.earthingGiven);
			QCOMPARE(r.indirect.passed + r.indirect.notPassed + r.indirect.notPerformed, 0);
			QCOMPARE(r.boards.at(1).rows.first().at(19).text, QStringLiteral("Not performed"));
			for (const CircuitNotes &n : r.notes) {
				QVERIFY(!n.lines.join('\n').contains(QStringLiteral("Indirect contacts")));
				if (n.title.endsWith(QLatin1String("C1 Alumbrado")))
					QVERIFY(!n.hasProblem);
			}

				// TT on CGMP, RA 30 ohm: C1 has only its C16 breaker, Ia 160 A
			for (Board b : model.boards()) {
				if (b.feeder.isNull()) {
					b.earthing = rebt::EarthingSystem::TT;
					b.earthResistance = 30.0;
					b.earthResistanceReason = QStringLiteral("Medida");
					model.setBoard(b);
				}
			}
			r = ElectricalReportBuilder::build(model, tables(), QStringLiteral("t.json"),
											   QStringLiteral("Vivienda"));
			QVERIFY(r.earthingGiven);
			const ReportRow &row = r.boards.at(1).rows.first();
			QCOMPARE(row.size(), r.columns.size());
			QCOMPARE(row.at(19).text, QStringLiteral("Not passed"));
			QCOMPARE(row.at(19).state, CellState::NotPassed);
			QCOMPARE(r.indirect.notPassed, 1);
			bool found = false;
			for (const CircuitNotes &n : r.notes) {
				const QString text = n.lines.join('\n');
				if (!text.contains(QStringLiteral("Indirect contacts: Not passed"))) continue;
				found = true;
				QVERIFY(n.hasProblem);
					// 30 x 160 = 4800 V
				QVERIFY2(text.contains(QStringLiteral("RA × Ia = 4800,00 V; RA × Ia ≤ U: not passed")),
						 qPrintable(text));
				QVERIFY(text.contains(QStringLiteral("RA as given: Medida")));
			}
			QVERIFY(found);
			bool cited = false;
			for (const rebt::NormRef &n : r.rules)
				cited = cited || n.id == "REBT-CIN-001";
			QVERIFY(cited);
			QVERIFY(r.disclaimer.contains(QStringLiteral("conditions of ITC-BT-24 4.1")));
		}

		void reportInfoRoundTrips()
		{
			const ElectricalModel model = chainModel();
			QDomDocument document;
			document.appendChild(model.toXml(document));
			ElectricalModel read;
			QVERIFY(read.fromXml(document.documentElement()).badValues.isEmpty());
			QVERIFY(read.reportInfo() == model.reportInfo());
			QVERIFY(read == model);
				// a model with report data only is not empty: it is kept
			ElectricalModel only;
			only.setReportInfo(model.reportInfo());
			QVERIFY(!only.isEmpty());
		}

		void builderUsesTheModel()
		{
			const ElectricalReport r = ElectricalReportBuilder::build(
				chainModel(), tables(), QStringLiteral("t.json"), QStringLiteral("Vivienda"));
			QCOMPARE(r.projectTitle, QStringLiteral("Vivienda"));
			QCOMPARE(r.info.planNumber, QStringLiteral("E-01"));
			QCOMPARE(r.info.documentTitle, ElectricalReportBuilder::defaultDocumentTitle());
			QCOMPARE(r.columns.size(), r.columnWidths.size());
			double width = 0;
			for (double w : r.columnWidths) width += w;
			QCOMPARE(width, 277.0);

			QCOMPARE(r.boards.size(), 2);
			QCOMPARE(r.boards.at(1).title, QStringLiteral("CS"));
			QVERIFY(r.boards.at(1).subtitle.contains(QStringLiteral("Alimentación CS")));
			const ReportRow &row = r.boards.at(1).rows.first();
			QCOMPARE(row.size(), r.columns.size());
			QCOMPARE(row.at(0).text, QStringLiteral("C1 Alumbrado"));
			QCOMPARE(row.at(5).text, QStringLiteral("16,00"));       // Ib
			QCOMPARE(row.at(6).text, QStringLiteral("Circuit breaker, C 16 A, UNE-EN 60898"));
			QCOMPARE(row.at(7).text, QStringLiteral("Cu PVC 2,5 mm² B1"));
			QCOMPARE(row.at(9).text, QStringLiteral("17,32"));       // Iz
			QCOMPARE(row.at(10).text, QStringLiteral("2,90"));
			QCOMPARE(row.at(11).text, QStringLiteral("3,39"));
				// 3.39 % from the origin: over 3 %, the same as the panel says
			QCOMPARE(row.at(12).state, CellState::NotPassed);
			QCOMPARE(row.at(13).state, CellState::Passed);
			QCOMPARE(row.at(14).state, CellState::Passed);
				// the supply of the CGMP is not known: no short circuit
			QCOMPARE(row.at(15).text, QStringLiteral("—"));
			QCOMPARE(row.at(16).text, QStringLiteral("—"));
			QCOMPARE(row.at(17).state, CellState::NotPerformed);
			QVERIFY(r.boards.at(0).subtitle.contains(QStringLiteral("Supply not known")));
			QCOMPARE(r.shortCircuit.notPerformed, 2);

			QCOMPARE(r.circuitCount, 2);
			QCOMPARE(r.drop.notPassed, 1);
			QCOMPARE(r.ampacity.passed, 1);
			QCOMPARE(r.coordination.passed, 1);

			QStringList ids;
			for (const rebt::NormRef &rule : r.rules)
				ids << QString::fromStdString(rule.id);
			for (const char *id : {"REBT-CDT-001", "REBT-CDT-002", "REBT-CDT-004",
								   "REBT-IZ-001", "REBT-IZ-002", "REBT-PROT-001",
								   "REBT-PROT-002"})
				QVERIFY2(ids.contains(QLatin1String(id)), id);
			QCOMPARE(ids.removeDuplicates(), 0);
			QCOMPARE(r.tables, QStringList{QStringLiteral("TEST-PVC2-B1 (synthetic test data)")});

			const CircuitNotes *c1 = nullptr;
			for (const CircuitNotes &n : r.notes)
				if (n.title.endsWith(QStringLiteral("C1 Alumbrado"))) c1 = &n;
			QVERIFY(c1);
			QVERIFY(c1->hasProblem);
			const QString all = c1->lines.join('\n');
			QVERIFY2(all.contains(QStringLiteral("From the origin: 3,39 %")), qPrintable(all));
			QVERIFY(all.contains(QStringLiteral("circuito único")));
			QVERIFY(all.contains(QStringLiteral("Longitud medida en obra")));
				// the rules are listed once, in their own section
			QVERIFY(!all.contains(QStringLiteral("Rules:")));
			QVERIFY(r.disclaimer.contains(QStringLiteral("does not state")));
		}

		void shortCircuitInTheReport()
		{
			ElectricalModel model = chainModel();
			Board cgmp;
			for (const Board &b : model.boards())
				if (b.feeder.isNull()) cgmp = b;
			cgmp.source = rebt::SourceMethod::KnownCurrent;
			cgmp.sourceMaxCurrent = 6.0;
			cgmp.sourceMinCurrent = 3.0;
			cgmp.sourceReason = QStringLiteral("Carta de la distribuidora");
			model.setBoard(cgmp);
			const ElectricalReport r = ElectricalReportBuilder::build(
				model, tables(), QStringLiteral("t.json"), QStringLiteral("Vivienda"));
			QVERIFY(r.boards.at(0).subtitle.contains(
				QStringLiteral("maximum 6 kA, minimum 3 kA (Carta de la distribuidora)")));
			QCOMPARE(r.shortCircuit.passed + r.shortCircuit.notPassed
						 + r.shortCircuit.notPerformed, r.circuitCount);
			QStringList ids;
			for (const rebt::NormRef &rule : r.rules)
				ids << QString::fromStdString(rule.id);
			QVERIFY(ids.contains(QStringLiteral("REBT-ICC-002")));
			QVERIFY(ids.contains(QStringLiteral("REBT-ICC-003")));
			QVERIFY(ids.contains(QStringLiteral("REBT-ICC-005")));
			QString all;
			for (const CircuitNotes &n : r.notes)
				all += n.lines.join('\n');
			QVERIFY(all.contains(QStringLiteral("Short circuit: ")));
			QVERIFY(all.contains(QStringLiteral("Icc max at the board")));
			QVERIFY(!all.contains(QStringLiteral("Rules:")));
				// the disclaimer no longer says the short circuit is left out
			QVERIFY(r.disclaimer.contains(QStringLiteral("only for the phase conductors")));
		}

		void dwellingInTheReport()
		{
			ElectricalModel model = chainModel();
			for (Circuit c : model.circuits()) {
				if (c.name == QLatin1String("C1 Alumbrado")) {
					c.dwellingCircuit = rebt::DwellingCircuit::C2;
					c.points = 8;
					model.setCircuit(c);
				}
			}
			for (Board b : model.boards()) {
				if (b.feeder.isNull()) {
					b.ownTransformer = true;
					model.setBoard(b);
				}
			}
			const ElectricalReport r = ElectricalReportBuilder::build(
				model, tables(), QStringLiteral("t.json"), QStringLiteral("Vivienda"));
			QCOMPARE(r.columns.size(), 20);
			double width = 0;
			for (double w : r.columnWidths) width += w;
			QCOMPARE(width, 277.0);
			const ReportRow &row = r.boards.at(1).rows.first();
			QCOMPARE(row.size(), r.columns.size());
				// IN 16 A: 2.90 % + 0.49 % from the origin > 3 %
			QCOMPARE(row.at(18).text, QStringLiteral("C2: Not passed"));
			QCOMPARE(row.at(18).state, CellState::NotPassed);
			QCOMPARE(r.dwelling.notPassed, 1);
				// the feeder is not a dwelling circuit: empty, not counted
			QVERIFY(r.boards.at(0).rows.first().at(18).text.isEmpty());
			QCOMPARE(r.dwelling.passed + r.dwelling.notPerformed, 0);
			QVERIFY(r.boards.at(0).subtitle.contains(QStringLiteral("Own transformer")));
			QVERIFY(r.boards.at(0).subtitle.contains(QStringLiteral("ITC-BT-25: Dwelling: not passed")));
			QCOMPARE(r.dwellingNotes.size(), 1);
			QCOMPARE(r.dwellingNotes.first().title, QStringLiteral("CGMP"));
			QVERIFY(r.dwellingNotes.first().lines.join('\n')
						.contains(QStringLiteral("Missing circuits: C1, C3, C4, C5")));
			QCOMPARE(r.dwellings.notPassed, 1);
				// no C13 in this dwelling: no line about it
			QVERIFY(!r.dwellingNotes.first().lines.join('\n').contains(QStringLiteral("C13")));
				// the CS hangs from an interior feeder: not an origin
			QVERIFY(!r.boards.at(1).subtitle.contains(QStringLiteral("ITC-BT-25")));
			QStringList ids;
			for (const rebt::NormRef &rule : r.rules)
				ids << QString::fromStdString(rule.id);
			for (const char *id : {"REBT-VIV-001", "REBT-VIV-002", "REBT-VIV-003",
								   "REBT-VIV-004", "REBT-VIV-005", "REBT-VIV-006"})
				QVERIFY2(ids.contains(QLatin1String(id)), id);
			QString all;
			for (const CircuitNotes &n : r.notes)
				all += n.lines.join('\n');
			QVERIFY2(all.contains(QStringLiteral("ITC-BT-25, C2 General use sockets and fridge: Not passed")),
					 qPrintable(all));
		}

		void builderWithoutTables()
		{
			const ElectricalReport r = ElectricalReportBuilder::build(
				chainModel(), rebt::AmpacityLibrary(), QString(), QString());
			QCOMPARE(r.tables, QStringList{QStringLiteral("No ampacity table loaded: IZ not computed.")});
			QCOMPARE(r.boards.at(1).rows.first().at(9).text, QStringLiteral("—"));
			QCOMPARE(r.ampacity.notPerformed, 2);
		}

		void pdfShowsTheModel()
		{
			if (!havePdfTools()) QSKIP("pdftotext and pdfinfo are needed");
			const QString path = m_dir.filePath(QStringLiteral("annex.pdf"));
			const ElectricalReport r = ElectricalReportBuilder::build(
				chainModel(), tables(), QString(), QStringLiteral("Vivienda unifamiliar"));
			QString error;
			QVERIFY2(ReportPdfWriter::write(r, path, &error), qPrintable(error));

				// A4 landscape: pdfinfo writes "842 x 595 pts (A4)" or "841.89 x 595.28 pts (A4)"
			const QString size = pdfInfo(path, QStringLiteral("Page size:"));
			QVERIFY2(size.contains(QStringLiteral("(A4)")), qPrintable(size));
			QVERIFY2(size.startsWith(QStringLiteral("84")) && size.contains(QStringLiteral("x 595")),
					 qPrintable(size));
			const int pages = pdfInfo(path, QStringLiteral("Pages:")).toInt();
			QVERIFY2(pages >= 4, qPrintable(QString::number(pages)));
			QCOMPARE(pdfInfo(path, QStringLiteral("Title:")),
					 QStringLiteral("Electrical calculation annex — Vivienda unifamiliar"));

			const QString text = pdfText(path);
			for (const QString &expected : {QStringLiteral("EMPRESA DE PRUEBA SL"),
											QStringLiteral("Vivienda unifamiliar"),
											QStringLiteral("E-01"),
											QStringLiteral("C1 Alumbrado"),
											QStringLiteral("17,32"), QStringLiteral("2,90"),
											QStringLiteral("3,39"), QStringLiteral("Not passed"),
											QStringLiteral("REBT-PROT-001"),
											QStringLiteral("ITC-BT-22"),
											QStringLiteral("synthetic test data"),
											QStringLiteral("does not state")})
				QVERIFY2(text.contains(expected), qPrintable(expected));
				// the title block on every page, with its number
			for (int page = 1; page <= pages; ++page) {
				const QString t = pdfText(path, page);
				QVERIFY2(QString(t).remove(QLatin1Char(' '))
							 .contains(QStringLiteral("%1/%2").arg(page).arg(pages)),
						 qPrintable(QStringLiteral("page %1").arg(page)));
				QVERIFY(t.contains(QStringLiteral("Autor de prueba")));
			}
		}

		void longTablesGoOnTheNextPage()
		{
			if (!havePdfTools()) QSKIP("pdftotext and pdfinfo are needed");
			const QString path = m_dir.filePath(QStringLiteral("long.pdf"));
			const ElectricalReport r = ElectricalReportBuilder::build(
				chainModel(60), tables(), QString(), QStringLiteral("Edificio"));
			QVERIFY(ReportPdfWriter::write(r, path));
			const QString text = pdfText(path);
			QVERIFY(text.contains(QStringLiteral("CS (continued)")));
				// every circuit is there once in the tables, none lost at a page end
			for (int i = 1; i <= 60; ++i)
				QVERIFY2(text.contains(QStringLiteral("Toma %1 ").arg(i))
						 || text.contains(QStringLiteral("Toma %1\n").arg(i)),
						 qPrintable(QString::number(i)));
			const int pages = pdfInfo(path, QStringLiteral("Pages:")).toInt();
			QVERIFY(pages > 6);
		}

		void commandLineWritesFoliosAndAnnex()
		{
			if (!havePdfTools()) QSKIP("pdftotext and pdfinfo are needed");
				// a project with folios and the model, saved
			const QString project_path = m_dir.filePath(QStringLiteral("project.qet"));
			int folios = 0;
			{
				QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
				QCOMPARE(project.state(), QETProject::Ok);
				folios = project.diagrams().size();
				QVERIFY(project.setElectricalModel(chainModel()));
				project.setFilePath(project_path);
				QVERIFY(project.write().isOk());
			}
			QFile tables_file(m_dir.filePath(QStringLiteral("tables.json")));
			QVERIFY(tables_file.open(QIODevice::WriteOnly));
			tables_file.write(syntheticTables());
			tables_file.close();

			const QString both = m_dir.filePath(QStringLiteral("both.pdf"));
			const QString annex = m_dir.filePath(QStringLiteral("annex-cli.pdf"));
			for (const auto &run : {qMakePair(QStringLiteral("--export-pdf-calculations"), both),
									qMakePair(QStringLiteral("--export-calculations"), annex)}) {
				QProcess p;
				p.start(QStringLiteral(QET_TEST_BINARY_PATH),
						{run.first, project_path, run.second,
						 QStringLiteral("--ampacity-tables=") + tables_file.fileName()});
				QVERIFY(p.waitForFinished(120000));
				QVERIFY2(p.exitCode() == 0, p.readAllStandardError().constData());
			}
			const int annex_pages = pdfInfo(annex, QStringLiteral("Pages:")).toInt();
			QVERIFY(annex_pages >= 4);
			QCOMPARE(pdfInfo(both, QStringLiteral("Pages:")).toInt(), folios + annex_pages);
				// the annex after the folios, its values from the file
			QVERIFY(pdfText(both, folios + 1).contains(QStringLiteral("Electrical calculation annex")));
				// in the language of the system the command runs in
			const QString cli_text = pdfText(annex);
			QVERIFY(cli_text.contains(QStringLiteral("17,32"))
					|| cli_text.contains(QStringLiteral("17.32")));
				// the suggestion counts the feeder: 4 mm2, not 2.5
			QVERIFY(cli_text.contains(QStringLiteral("Suggested section: 4 mm²")));
		}

		void panelKeepsTheDataAndWrites()
		{
			if (!havePdfTools()) QSKIP("pdftotext and pdfinfo are needed");
			QETProject *project = QetAppTest::editor()->currentProject();
			if (!project) {
				QetAppTest::view();
				project = QetAppTest::editor()->currentProject();
			}
			QVERIFY(project);
			ElectricalModel model = chainModel();
			model.setReportInfo(ReportInfo());
			QVERIFY(project->setElectricalModel(model, false));
			project->undoStack()->clear();
			ElectricalCircuitsDock dock;
			dock.setProject(project);

				// the dialog starts from the title block of the project
			const ReportInfo proposed = ReportDialog::proposal(
				project->electricalModel().reportInfo(), QStringLiteral("Firma"),
				QStringLiteral("A"));
			QCOMPARE(proposed.author, QStringLiteral("Firma"));
			QCOMPARE(proposed.revision, QStringLiteral("A"));
			QVERIFY(!proposed.date.isEmpty());
			ReportDialog dialog(proposed);
			dialog.field(QStringLiteral("company"))->setText(QStringLiteral(" Mi empresa "));
			dialog.field(QStringLiteral("planNumber"))->setText(QStringLiteral("E-07"));
			QCOMPARE(dialog.info().company, QStringLiteral("Mi empresa"));

			const QString path = m_dir.filePath(QStringLiteral("panel.pdf"));
			QCOMPARE(dock.exportReport(path, dialog.info(), false), QString());
			QCOMPARE(project->electricalModel().reportInfo().planNumber, QStringLiteral("E-07"));
			QCOMPARE(project->undoStack()->count(), 1);
			QVERIFY(pdfText(path).contains(QStringLiteral("E-07")));
				// the same data again: nothing more to undo
			QCOMPARE(dock.exportReport(path, dialog.info(), false), QString());
			QCOMPARE(project->undoStack()->count(), 1);
			project->undoStack()->undo();
			QVERIFY(project->electricalModel().reportInfo().isEmpty());
		}

		void logoIsKeptInTheProject()
		{
			ElectricalModel model = chainModel();
			ReportInfo info = model.reportInfo();
			info.logo = pngOf(QSize(300, 100));
			model.setReportInfo(info);
			QDomDocument document;
			document.appendChild(model.toXml(document));
				// in the file itself, not a path to it
			QVERIFY(document.toString().contains(QStringLiteral("<logo format=\"png\">")));
			ElectricalModel read;
			QVERIFY(read.fromXml(document.documentElement()).badValues.isEmpty());
			QCOMPARE(read.reportInfo().logo, info.logo);
			QCOMPARE(ReportLogo::image(read.reportInfo().logo).size(), QSize(300, 100));

				// a damaged logo is reported, not kept
			QString text = document.toString();
			text.replace(QString::fromLatin1(info.logo.toBase64()), QStringLiteral("@@not base64@@"));
			QDomDocument damaged;
			damaged.setContent(text);
			ElectricalModel broken;
			QVERIFY(broken.fromXml(damaged.documentElement()).badValues
					.contains(QStringLiteral("report/logo")));
			QVERIFY(broken.reportInfo().logo.isEmpty());
		}

		void logoFromAFile()
		{
				// a large PNG is kept at 1200 px at most, undistorted
			const QString big = m_dir.filePath(QStringLiteral("big.png"));
			QImage(3000, 1000, QImage::Format_ARGB32).save(big);
			QString error;
			const QByteArray png = ReportLogo::fromFile(big, &error);
			QVERIFY2(!png.isEmpty(), qPrintable(error));
			QCOMPARE(ReportLogo::image(png).size(), QSize(1200, 400));
				// small: as it is
			const QString small = m_dir.filePath(QStringLiteral("small.jpg"));
			QImage(200, 80, QImage::Format_RGB32).save(small);
			QCOMPARE(ReportLogo::image(ReportLogo::fromFile(small)).size(), QSize(200, 80));
				// not an image
			const QString text = m_dir.filePath(QStringLiteral("not.png"));
			QFile f(text);
			QVERIFY(f.open(QIODevice::WriteOnly));
			f.write("hello");
			f.close();
			QVERIFY(ReportLogo::fromFile(text, &error).isEmpty());
			QVERIFY(error.contains(QStringLiteral("not.png")));
				// a vector logo, drawn at the size kept
			if (!QImageReader::supportedImageFormats().contains("svg"))
				QSKIP("no SVG image plugin here");
			const QString svg = m_dir.filePath(QStringLiteral("logo.svg"));
			QFile s(svg);
			QVERIFY(s.open(QIODevice::WriteOnly));
			s.write("<svg xmlns='http://www.w3.org/2000/svg' width='120' height='40'>"
					"<rect width='120' height='40' fill='#0a7'/></svg>");
			s.close();
			QCOMPARE(ReportLogo::image(ReportLogo::fromFile(svg)).size(), QSize(1200, 400));
		}

		void logoOnEveryPage()
		{
			if (QStandardPaths::findExecutable(QStringLiteral("pdfimages")).isEmpty())
				QSKIP("pdfimages is needed");
			ElectricalModel model = chainModel();
			ReportInfo info = model.reportInfo();
			info.logo = pngOf(QSize(400, 120));
			model.setReportInfo(info);
			const QString path = m_dir.filePath(QStringLiteral("logo.pdf"));
			QVERIFY(ReportPdfWriter::write(ElectricalReportBuilder::build(
				model, tables(), QString(), QStringLiteral("P")), path));
			const int pages = pdfInfo(path, QStringLiteral("Pages:")).toInt();
				// pdfimages -list: one line per image, the page first
			QProcess p;
			p.start(QStringLiteral("pdfimages"), {QStringLiteral("-list"), path});
			QVERIFY(p.waitForFinished(30000));
			QMap<int, int> per_page;
			const QStringList lines = QString::fromUtf8(p.readAllStandardOutput()).split('\n');
			for (const QString &line : lines.mid(2)) {
				const int page = line.simplified().section(QLatin1Char(' '), 0, 0).toInt();
				if (page > 0) ++per_page[page];
			}
			QCOMPARE(per_page.value(1), 2);   // cover and title block
			for (int page = 2; page <= pages; ++page)
				QCOMPARE(per_page.value(page), 1);

				// without a logo, no image at all
			model.setReportInfo(chainModel().reportInfo());
			QVERIFY(ReportPdfWriter::write(ElectricalReportBuilder::build(
				model, tables(), QString(), QStringLiteral("P")), path));
			p.start(QStringLiteral("pdfimages"), {QStringLiteral("-list"), path});
			QVERIFY(p.waitForFinished(30000));
			QCOMPARE(QString::fromUtf8(p.readAllStandardOutput()).split('\n',
					 Qt::SkipEmptyParts).size(), 2);   // the two header lines
		}

		void dialogTakesTheLogo()
		{
			ReportDialog dialog(ReportInfo{});
			QCOMPARE(dialog.logoPreview()->text(), QStringLiteral("No logo"));
			const QString file = m_dir.filePath(QStringLiteral("dialog.png"));
			QImage(500, 200, QImage::Format_ARGB32).save(file);
			QCOMPARE(dialog.setLogoFile(file), QString());
			QVERIFY(!dialog.info().logo.isEmpty());
			QVERIFY(!dialog.logoPreview()->pixmap().isNull());
			QVERIFY(!dialog.setLogoFile(m_dir.filePath(QStringLiteral("missing.png"))).isEmpty());
			QVERIFY(!dialog.info().logo.isEmpty());   // the one before is kept
			dialog.clearLogo();
			QVERIFY(dialog.info().logo.isEmpty());
		}

		void screenshot()
		{
				//Only when asked: the pages as pictures, for the documentation
			const QString out = qEnvironmentVariable("QET_ELECTRICAL_REPORT_PDF");
			if (out.isEmpty()) QSKIP("QET_ELECTRICAL_REPORT_PDF not set");
			QTranslator translator;
			const QString qm = qEnvironmentVariable("QET_ELECTRICAL_SCREENSHOT_QM");
			if (!qm.isEmpty() && translator.load(qm))
				QCoreApplication::installTranslator(&translator);
			ElectricalModel model = chainModel(3);
				//Short circuit: the CGMP fed by a DI from the CPM, network
				//not known (Guía-BT Anexo 3)
			Board cpm{QUuid::createUuid(), QStringLiteral("CPM"), QUuid(), QUuid()};
			cpm.source = rebt::SourceMethod::GuideFactor;
			model.setBoard(cpm);
			Circuit di;
			di.id = QUuid::createUuid();
			di.name = QStringLiteral("DI");
			di.board = cpm.id;
			di.use = CircuitUse::DiSingleUser;
			di.system = rebt::System::SinglePhase;
			di.voltage = 230.0;
			di.load.designCurrent = 25.0;
			di.load.powerFactor = 1.0;
			di.conductor.material = rebt::Material::Copper;
			di.conductor.insulation = rebt::Insulation::XLPE;
			di.conductor.section = 10.0;
			di.conductor.neutralSection = 10.0;
			di.conductor.length = 15.0;
			di.conductor.conductivity = 48.0;
			model.setCircuit(di);
			for (Board b : model.boards()) {
				if (b.name == QLatin1String("CGMP")) {
					b.feeder = di.id;
						//The dwelling: example data
					b.dwellingArea = 95.0;
					b.plannedHeating = false;
					b.plannedAirConditioning = false;
					b.plannedAutomation = false;
					b.plannedVehicleCharging = false;
					b.generalSwitch = 40.0;
					b.residualDevices = {{QStringLiteral("ID1"), 30.0, 40.0}};
					b.plannedDryer = false;
					RoomData salon;
					salon.name = QStringLiteral("Salón");
					salon.type = rebt::RoomType::LivingRoom;
					salon.area = 22.0;
					salon.counts = {{rebt::RoomItem::LightPoints, 2},
									{rebt::RoomItem::Switches, 2},
									{rebt::RoomItem::Sockets, 4}};
					RoomData banyo;
					banyo.name = QStringLiteral("Baño");
					banyo.type = rebt::RoomType::Bathroom;
					banyo.counts = {{rebt::RoomItem::LightPoints, 1},
									{rebt::RoomItem::Switches, 1},
									{rebt::RoomItem::WetSockets, 0}};
					b.rooms = {salon, banyo};
					model.setBoard(b);
				}
			}
			for (Circuit c : model.circuits()) {
				if (c.id == di.id) continue;
				c.conductor.material = rebt::Material::Copper;
				if (!c.conductor.insulation)
					c.conductor.insulation = rebt::Insulation::XLPE;
				c.protection.type = ProtectionType::CircuitBreaker;
				c.protection.standard = rebt::ProtectionStandard::EN60898;
				if (!c.protection.ratedCurrent)
					c.protection.ratedCurrent = 32.0;
				if (c.protection.curve.isEmpty())
					c.protection.curve = QStringLiteral("C");
				c.protection.breakingCapacity = 6.0;
				if (c.name == QLatin1String("Toma 1")) {
					c.protection.letThrough = 15000.0;
					c.protection.letThroughReason =
						QStringLiteral("Ejemplo: curva I²t del catálogo");
				}
					//Dwelling circuits of table 1 of ITC-BT-25
				if (c.name == QLatin1String("C1 Alumbrado")) {
					c.dwellingCircuit = rebt::DwellingCircuit::C1;
					c.points = 12;
					c.conductor.section = 1.5;
					c.protection.ratedCurrent = 10.0;
					c.load.designCurrent = 8.0;
				} else if (c.name == QLatin1String("Toma 1")) {
					c.name = QStringLiteral("C2 Tomas");
					c.dwellingCircuit = rebt::DwellingCircuit::C2;
					c.points = 14;
				} else if (c.name == QLatin1String("Toma 2")) {
					c.name = QStringLiteral("C5 Baño y cocina");
					c.dwellingCircuit = rebt::DwellingCircuit::C5;
					c.points = 7;
				} else if (c.name == QLatin1String("Toma 3")) {
					c.name = QStringLiteral("C3 Cocina y horno");
					c.dwellingCircuit = rebt::DwellingCircuit::C3;
					c.points = 1;
					c.conductor.section = 6.0;
					c.protection.ratedCurrent = 25.0;
					c.load.designCurrent = 20.0;
				}
				if (c.dwellingCircuit) {
					c.protection.residualDevice = QStringLiteral("ID1");
					c.conductor.conduit = c.conductor.section && *c.conductor.section >= 6
										  ? 25.0 : 20.0;
				}
				model.setCircuit(c);
			}
			ReportInfo info = model.reportInfo();
			info.company = QStringLiteral("EMPRESA DE EJEMPLO SL");
			info.author = QStringLiteral("Técnico de ejemplo");
			const QString company = qEnvironmentVariable("QET_ELECTRICAL_REPORT_COMPANY");
			if (!company.isEmpty())
				info.company = company;
			const QString logo = qEnvironmentVariable("QET_ELECTRICAL_REPORT_LOGO");
			if (!logo.isEmpty())
				info.logo = ReportLogo::fromFile(logo);
			model.setReportInfo(info);
			const ElectricalReport r = ElectricalReportBuilder::build(
				model, tables(), QString(), QStringLiteral("Vivienda unifamiliar — ejemplo"));
			QVERIFY(ReportPdfWriter::write(r, out));
		}

	private:
		QTemporaryDir m_dir;
};

int main(int argc, char **argv)
{
	return QetAppTest::run<tst_electricalreport>(argc, argv);
}

#include "tst_electricalreport.moc"
