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
#include "electrical/ui/changeelectricalmodelcommand.h"
#include "electrical/ui/boarddialog.h"
#include "electrical/ui/circuitdialog.h"
#include "electrical/ui/roomsdialog.h"
#include "electrical/ui/devicepickerdialog.h"
#include "electrical/devicecatalogue.h"
#include "electrical/ui/electricalelementwidget.h"
#include "ui/elementpropertieswidget.h"

#include <QTabWidget>
#include "electrical/ui/electricalcircuitsdock.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "electrical/ampacityfile.h"
#include "diagram.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTranslator>
#include <QTreeWidget>
#include <QUndoStack>

using namespace electrical;

/**
	The electrical circuits panel and its undo command, in the real
	editor: off by default, every change undoable, numbers as typed in
	Spanish, links to the symbols of the drawing.
*/
/// Fictitious devices, for the tests only: no manufacturer's values
QByteArray testCatalogueJson()
{
	return R"({ "format": "qet-devices", "version": 1, "devices": [
		{ "manufacturer": "TEST", "partNumber": "MCB-C16-2P", "source": "test data",
		  "type": "circuit-breaker", "standard": "EN60898", "ratedCurrent": 16,
		  "curve": "C", "breakingCapacity": 6, "poles": 2,
		  "letThrough": {"energy": 20000, "atCurrent": 6} },
		{ "manufacturer": "TEST", "partNumber": "RCD-40-30", "source": "test data p. 2",
		  "type": "residual-current", "ratedCurrent": 40, "sensitivity": 30, "poles": 2 },
		{ "manufacturer": "OTHER", "partNumber": "MCCB-100", "source": "test data p. 3",
		  "type": "circuit-breaker", "standard": "EN60947-2", "ratedCurrent": 100,
		  "breakingCapacity": 25, "poles": 4, "tripCurrent": 1000,
		  "conventionalCurrent": 130 } ] })";
}

class tst_electricalpanel : public QObject
{
		Q_OBJECT

	private slots:
		void initTestCase();
		void init();
		void cleanup();

		void offByDefault();
		void settingShowsAndHides();
		void commandUndoesAndRedoes();
		void commandRefusesNoChangeAndReadOnly();
		void boardsAndCircuitsInTheTree();
		void resultsShownWithTheirCheck();
		void removingABoardIsOneUndoStep();
		void linksToTheDrawing();
		void readOnlyModelCannotBeEdited();
		void dialogReadsWhatIsTyped();
		void dialogRefusesWhatIsNotANumber();
		void ampacityColumns();
		void tablesFileWithMistakes();
		void dialogSuggestsASection();
		void chainInThePanel();
		void removingAFeederFreesItsBoard();
		void coordinationColumn();
		void boardDialogChoosesTheFeeder();
		void dialogShowsTheDropFromTheOrigin();
		void shortCircuitColumns();
		void boardDialogSupply();
		void dwellingColumn();
		void roomsDialog();
		void electricalTab();
		void deviceCatalogue();
		void indirectContacts();
		void screenshot();

	private:
		Circuit sampleCircuit(const QUuid &board) const;

		QETProject *m_project = nullptr;
		ElectricalCircuitsDock *m_dock = nullptr;
		QLocale m_previous_locale;
};

void tst_electricalpanel::initTestCase()
{
	QVERIFY(QetAppTest::editor());
	QVERIFY(QetAppTest::view());
		//Spanish: decimal comma, and a group separator that must never show
	QLocale::setDefault(QLocale(QLocale::Spanish, QLocale::Spain));
}

void tst_electricalpanel::init()
{
	m_project = QetAppTest::editor()->currentProject();
	QVERIFY(m_project);
	QVERIFY(m_project->setElectricalModel(ElectricalModel(), false));
	m_project->undoStack()->clear();
	m_dock = new ElectricalCircuitsDock;
	m_dock->setProject(m_project);
	m_dock->setDiagramProvider([] { return QetAppTest::diagram(); });
}

void tst_electricalpanel::cleanup()
{
	delete m_dock;
	m_dock = nullptr;
}

Circuit tst_electricalpanel::sampleCircuit(const QUuid &board) const
{
		// case CDT-M1 of docs/rebt/casos_h1.md: 2.898551 %
	Circuit c;
	c.id = QUuid::createUuid();
	c.board = board;
	c.name = QStringLiteral("C1 Alumbrado");
	c.system = rebt::System::SinglePhase;
	c.voltage = 230.0;
	c.use = CircuitUse::DwellingInterior;
	c.load.power = 3680.0;
	c.load.powerFactor = 1.0;
	c.conductor.length = 25.0;
	c.conductor.section = 2.5;
	c.conductor.conductivity = 48.0;
	c.protection.ratedCurrent = 16.0;
	c.protection.curve = QStringLiteral("C");
	return c;
}

void tst_electricalpanel::offByDefault()
{
	QETDiagramEditor *editor = QetAppTest::editor();
	auto *dock = editor->findChild<ElectricalCircuitsDock *>();
	QVERIFY(dock);
	QVERIFY(!ElectricalCircuitsDock::isEnabledInSettings());
	QVERIFY(!dock->isVisible());
	QVERIFY(!dock->toggleViewAction()->isVisible());
	QVERIFY(!dock->project());
}

void tst_electricalpanel::settingShowsAndHides()
{
	QETDiagramEditor *editor = QetAppTest::editor();
	auto *dock = editor->findChild<ElectricalCircuitsDock *>();
	QSettings().setValue(ElectricalCircuitsDock::settingKey(), true);
	editor->updateElectricalCircuitsDock();
	QVERIFY(dock->toggleViewAction()->isVisible());
	QCOMPARE(dock->project(), m_project);
	dock->toggleViewAction()->trigger();
	QVERIFY(dock->isVisible());

	QSettings().setValue(ElectricalCircuitsDock::settingKey(), false);
	editor->updateElectricalCircuitsDock();
	QVERIFY(!dock->isVisible());
	QVERIFY(!dock->toggleViewAction()->isVisible());
	QVERIFY(!dock->project());
}

void tst_electricalpanel::commandUndoesAndRedoes()
{
	QUndoStack *stack = m_project->undoStack();
	ElectricalModel model;
	model.setBoard({QUuid::createUuid(), QStringLiteral("CGMP"), QUuid()});
	QVERIFY(ChangeElectricalModelCommand::push(m_project, model, QStringLiteral("add")));
	QCOMPARE(stack->count(), 1);
	QVERIFY(m_project->electricalModel() == model);
	QVERIFY(!stack->isClean());

	stack->undo();
	QVERIFY(m_project->electricalModel().isEmpty());
	QVERIFY(stack->isClean());
	stack->redo();
	QVERIFY(m_project->electricalModel() == model);
}

void tst_electricalpanel::commandRefusesNoChangeAndReadOnly()
{
	QVERIFY(!ChangeElectricalModelCommand::push(m_project, ElectricalModel(),
												QStringLiteral("nothing")));
	QCOMPARE(m_project->undoStack()->count(), 0);
	QVERIFY(!ChangeElectricalModelCommand::push(nullptr, ElectricalModel(),
												QStringLiteral("no project")));
}

void tst_electricalpanel::boardsAndCircuitsInTheTree()
{
	const QUuid board = m_dock->createBoard(QStringLiteral("CGMP"));
	QVERIFY(!board.isNull());
	const QUuid circuit = m_dock->createCircuit(board, QStringLiteral("C1"));
	QVERIFY(!circuit.isNull());
	m_dock->createCircuit(QUuid(), QStringLiteral("loose"));

	QTreeWidget *tree = m_dock->tree();
	QCOMPARE(tree->topLevelItemCount(), 2);   // the board, and "no board"
	QCOMPARE(tree->topLevelItem(0)->text(0), QStringLiteral("CGMP"));
	QCOMPARE(tree->topLevelItem(0)->childCount(), 1);
	QCOMPARE(tree->topLevelItem(0)->child(0)->text(0), QStringLiteral("C1"));
	QCOMPARE(tree->topLevelItem(1)->childCount(), 1);
	QCOMPARE(m_dock->currentId(), m_project->electricalModel().circuits().last().id);
	QCOMPARE(m_project->undoStack()->count(), 3);

		//Undone, the tree follows the model
	m_project->undoStack()->undo();
	m_project->undoStack()->undo();
	QCOMPARE(tree->topLevelItemCount(), 1);
	QCOMPARE(tree->topLevelItem(0)->childCount(), 0);
}

void tst_electricalpanel::resultsShownWithTheirCheck()
{
	const QUuid board = m_dock->createBoard(QStringLiteral("CGMP"));
	QVERIFY(m_dock->applyCircuit(sampleCircuit(board)));
	QTreeWidgetItem *row = m_dock->tree()->topLevelItem(0)->child(0);
	using D = ElectricalCircuitsDock;
	QCOMPARE(row->text(D::PowerColumn), QStringLiteral("3680"));   // no "3.680"
	QCOMPARE(row->text(D::SectionColumn), QStringLiteral("2,5"));
	QCOMPARE(row->text(D::CurrentColumn), QStringLiteral("16,00"));
	QCOMPARE(row->text(D::DropColumn), QStringLiteral("2,90"));
	QCOMPARE(row->text(D::CheckColumn), QStringLiteral("Passed"));
	QCOMPARE(row->text(D::ProtectionColumn), QStringLiteral("C 16 A"));
	QVERIFY(row->toolTip(D::CheckColumn).contains(QLatin1String("ITC-BT-19 2.2.2")));
		//In the panel the drop is checked from the origin: here the board
		//is the origin itself, so both are the same
	QVERIFY(!row->toolTip(D::CheckColumn).contains(QLatin1String("Partial check")));
	QCOMPARE(row->text(D::DropTotalColumn), QStringLiteral("2,90"));

		//Longer: over 3 %
	Circuit longer = sampleCircuit(board);
	longer.id = m_project->electricalModel().circuits().first().id;
	longer.conductor.length = 30.0;
	QVERIFY(m_dock->applyCircuit(longer));
	row = m_dock->tree()->topLevelItem(0)->child(0);
	QCOMPARE(row->text(D::CheckColumn), QStringLiteral("Not passed"));

		//Without its length: no number, and why
	longer.conductor.length.reset();
	QVERIFY(m_dock->applyCircuit(longer));
	row = m_dock->tree()->topLevelItem(0)->child(0);
	QCOMPARE(row->text(D::DropColumn), QStringLiteral("—"));
	QCOMPARE(row->text(D::CheckColumn), QStringLiteral("Not performed"));
	QVERIFY(row->toolTip(D::CheckColumn).contains(QLatin1String("Missing: length")));

		//Without its use: no limit to check against
	longer.conductor.length = 25.0;
	longer.use.reset();
	QVERIFY(m_dock->applyCircuit(longer));
	row = m_dock->tree()->topLevelItem(0)->child(0);
	QCOMPARE(row->text(D::CheckColumn), QStringLiteral("No limit"));
	QVERIFY(m_dock->summaryLabel()->text().contains(QLatin1String("1 circuit")));
}

void tst_electricalpanel::removingABoardIsOneUndoStep()
{
	const QUuid board = m_dock->createBoard(QStringLiteral("CGMP"));
	m_dock->createCircuit(board, QStringLiteral("C1"));
	m_dock->createCircuit(board, QStringLiteral("C2"));
	const ElectricalModel before = m_project->electricalModel();
	const int steps = m_project->undoStack()->count();

	QVERIFY(m_dock->removeItem(board));
	QVERIFY(m_project->electricalModel().isEmpty());
	QCOMPARE(m_project->undoStack()->count(), steps + 1);
	m_project->undoStack()->undo();
	QVERIFY(m_project->electricalModel() == before);
}

void tst_electricalpanel::linksToTheDrawing()
{
		//A project with symbols and conductors, its folio as the one shown
	QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
	QCOMPARE(project.state(), QETProject::Ok);
	Diagram *diagram = nullptr;
	for (Diagram *d : project.diagrams())
		if (d->elements().size() >= 2 && !d->conductors().isEmpty()) {
			diagram = d;
			break;
		}
	QVERIFY(diagram);
	ElectricalCircuitsDock dock;
	dock.setProject(&project);
	dock.setDiagramProvider([diagram] { return diagram; });

	const QUuid board = dock.createBoard(QStringLiteral("CGMP"));
	const QUuid circuit = dock.createCircuit(board, QStringLiteral("C1"));
	const QList<Element *> elements = diagram->elements();
	Conductor *wire = diagram->conductors().first();

		//Nothing selected, or two symbols for one place: refused
	diagram->clearSelection();
	QVERIFY(!dock.assignSelection(ElectricalCircuitsDock::Role::Origin).isEmpty());
	elements.at(0)->setSelected(true);
	elements.at(1)->setSelected(true);
	QVERIFY(!dock.assignSelection(ElectricalCircuitsDock::Role::Origin).isEmpty());
	QVERIFY(project.electricalModel().circuit(circuit)->origin.isNull());

	diagram->clearSelection();
	elements.at(0)->setSelected(true);
	QCOMPARE(dock.assignSelection(ElectricalCircuitsDock::Role::Origin), QString());
	diagram->clearSelection();
	elements.at(1)->setSelected(true);
	QCOMPARE(dock.assignSelection(ElectricalCircuitsDock::Role::Protection), QString());
	diagram->clearSelection();
	wire->setSelected(true);
	QCOMPARE(dock.assignSelection(ElectricalCircuitsDock::Role::Wires), QString());

	const Circuit c = *project.electricalModel().circuit(circuit);
	QCOMPARE(c.origin, elements.at(0)->uuid());
	QCOMPARE(c.protection.element, elements.at(1)->uuid());
	QCOMPARE(c.conductor.wires, QList<QUuid>{wire->uuid()});

		//The board takes a symbol too, once it is the current row
	dock.setCurrentId(board);
	diagram->clearSelection();
	elements.at(1)->setSelected(true);
	QCOMPARE(dock.assignSelection(ElectricalCircuitsDock::Role::BoardSymbol), QString());
	QCOMPARE(project.electricalModel().board(board)->element, elements.at(1)->uuid());

		//Shown in the drawing: what the circuit points at, selected
	dock.setCurrentId(circuit);
	diagram->clearSelection();
	QCOMPARE(dock.showInDrawing(), 3);
	QVERIFY(elements.at(0)->isSelected());
	QVERIFY(elements.at(1)->isSelected());
	QVERIFY(wire->isSelected());

		//Each link was one step of the undo history
	QCOMPARE(project.undoStack()->count(), 6);
}

void tst_electricalpanel::readOnlyModelCannotBeEdited()
{
		//A project of its own: its model stays read only
	QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
	QDomDocument document;
	document.setContent(QStringLiteral("<electrical version=\"9\"><x/></electrical>"));
	ElectricalModel newer;
	newer.fromXml(document.documentElement());
	QVERIFY(newer.isReadOnly());
		//As opening a file of a newer version would
	QVERIFY(project.setElectricalModel(newer, false));
	ElectricalCircuitsDock dock;
	dock.setProject(&project);
	QVERIFY(dock.messageLabel()->isVisibleTo(&dock));
	QVERIFY(dock.createBoard(QStringLiteral("x")).isNull());
	QCOMPARE(project.undoStack()->count(), 0);
	QVERIFY(!ChangeElectricalModelCommand::push(&project, ElectricalModel(),
												QStringLiteral("clear")));
	QVERIFY(!project.setElectricalModel(ElectricalModel(), false));
}

void tst_electricalpanel::dialogReadsWhatIsTyped()
{
	const QList<Board> boards{{QUuid::createUuid(), QStringLiteral("CGMP"), QUuid()},
							  {QUuid::createUuid(), QStringLiteral("CS1"), QUuid()}};
	Circuit original;
	original.id = QUuid::createUuid();
	original.origin = QUuid::createUuid();   // not shown, kept
	original.conductor.wires = {QUuid::createUuid()};
	CircuitDialog dialog(original, boards);

	dialog.numberField(QStringLiteral("voltage"))->setText(QStringLiteral(" 230 "));
	dialog.numberField(QStringLiteral("section"))->setText(QStringLiteral("2,5"));
	dialog.numberField(QStringLiteral("length"))->setText(QStringLiteral("25.5"));
	dialog.numberField(QStringLiteral("poles"))->setText(QStringLiteral("2"));
	dialog.choiceField(QStringLiteral("board"))->setCurrentIndex(2);   // CS1
	dialog.choiceField(QStringLiteral("system"))->setCurrentIndex(2);  // three phase
	QVERIFY(dialog.isValid());

	const Circuit c = dialog.circuit();
	QCOMPARE(c.id, original.id);
	QCOMPARE(c.origin, original.origin);
	QCOMPARE(c.conductor.wires, original.conductor.wires);
	QCOMPARE(*c.voltage, 230.0);
	QCOMPARE(*c.conductor.section, 2.5);
	QCOMPARE(*c.conductor.length, 25.5);
	QCOMPARE(*c.protection.poles, 2);
	QCOMPARE(c.board, boards.at(1).id);
	QCOMPARE(*c.system, rebt::System::ThreePhase);
		//Left empty: unknown, never a default
	QVERIFY(!c.load.power);
	QVERIFY(!c.load.powerFactor);
	QVERIFY(!c.conductor.material);
	QVERIFY(!c.use);
	QVERIFY(dialog.resultLabel()->text().contains(QLatin1String("not computable")));

		//A known value shows as typed in Spanish
	Circuit known = original;
	known.conductor.section = 1.5;
	known.load.power = 3680.0;
	CircuitDialog again(known, boards);
	QCOMPARE(again.numberField(QStringLiteral("section"))->text(), QStringLiteral("1,5"));
	QCOMPARE(again.numberField(QStringLiteral("power"))->text(), QStringLiteral("3680"));
}

void tst_electricalpanel::dialogRefusesWhatIsNotANumber()
{
	CircuitDialog dialog(Circuit(), {});
	auto *ok = dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
	QVERIFY(ok->isEnabled());
	const QStringList bad{QStringLiteral("abc"), QStringLiteral("1.234,5"),
						  QStringLiteral("2,5,1"), QStringLiteral("inf")};
	for (const QString &text : bad) {
		dialog.numberField(QStringLiteral("length"))->setText(text);
		QVERIFY2(!dialog.isValid(), qPrintable(text));
		QVERIFY2(!ok->isEnabled(), qPrintable(text));
	}
	dialog.numberField(QStringLiteral("length"))->clear();
	QVERIFY(dialog.isValid());
	dialog.numberField(QStringLiteral("poles"))->setText(QStringLiteral("1,5"));
	QVERIFY(!dialog.isValid());
	dialog.numberField(QStringLiteral("poles"))->setText(QStringLiteral("0"));
	QVERIFY(!dialog.isValid());
}

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

QString writeTables(const QTemporaryDir &dir, const QByteArray &json)
{
	const QString path = dir.filePath(QStringLiteral("tables.json"));
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly)) return QString();
	file.write(json);
	return path;
}

Circuit withAmpacityData(Circuit c)
{
	c.conductor.conductivity = 48.0;
	c.conductor.material = rebt::Material::Copper;
	c.conductor.insulation = rebt::Insulation::PVC;
	c.conductor.installationMethod = QStringLiteral("B1");
	c.conductor.ambientTemperature = 40.0;
	c.conductor.groupingFactor = 1.0;
	c.conductor.groupingFactorReason = QStringLiteral("circuito único");
	return c;
}

} // namespace

void tst_electricalpanel::ampacityColumns()
{
	using D = ElectricalCircuitsDock;
	const QUuid board = m_dock->createBoard(QStringLiteral("CGMP"));
	QVERIFY(m_dock->applyCircuit(withAmpacityData(sampleCircuit(board))));

		//No tables: no IZ, and the summary says why
	AmpacityTables::setFilePath(QString());
	m_dock->refresh();
	QTreeWidgetItem *row = m_dock->tree()->topLevelItem(0)->child(0);
	QCOMPARE(row->text(D::AmpacityColumn), QStringLiteral("—"));
	QCOMPARE(row->text(D::AmpacityCheckColumn), QStringLiteral("Not performed"));
	QVERIFY(row->toolTip(D::AmpacityCheckColumn).contains(QLatin1String("No ampacity table")));
	QVERIFY(m_dock->summaryLabel()->text().contains(QLatin1String("No ampacity table loaded")));

		//With them: 20 A x 0.866 = 17.32 A for 16 A, case A of tablas_h4.md
	QTemporaryDir dir;
	const QString message = m_dock->loadTables(writeTables(dir, syntheticTables()));
	QVERIFY2(message.contains(QLatin1String("1 ampacity table")), qPrintable(message));
	row = m_dock->tree()->topLevelItem(0)->child(0);
	QCOMPARE(row->text(D::AmpacityColumn), QStringLiteral("17,32"));
	QCOMPARE(row->text(D::AmpacityCheckColumn), QStringLiteral("Passed"));
	const QString tip = row->toolTip(D::AmpacityCheckColumn);
	QVERIFY2(tip.contains(QLatin1String("TEST-PVC2-B1")), qPrintable(tip));
	QVERIFY(tip.contains(QStringLiteral("circuito único")));
	QVERIFY(tip.contains(QLatin1String("ITC-BT-19 2.2.3")));
	QVERIFY(tip.contains(QStringLiteral("Suggested section: 2,5 mm²")));

		//Two circuits in the conduit, 0.8: 13.86 A, not passed
	Circuit grouped = m_project->electricalModel().circuits().first();
	grouped.conductor.groupingFactor = 0.8;
	grouped.conductor.groupingFactorReason = QStringLiteral("2 circuitos en el tubo");
	QVERIFY(m_dock->applyCircuit(grouped));
	row = m_dock->tree()->topLevelItem(0)->child(0);
	QCOMPARE(row->text(D::AmpacityColumn), QStringLiteral("13,86"));
	QCOMPARE(row->text(D::AmpacityCheckColumn), QStringLiteral("Not passed"));

		//A factor without its reason: not performed, and why
	grouped.conductor.groupingFactorReason.clear();
	QVERIFY(m_dock->applyCircuit(grouped));
	row = m_dock->tree()->topLevelItem(0)->child(0);
	QCOMPARE(row->text(D::AmpacityCheckColumn), QStringLiteral("Not performed"));
	QVERIFY(row->toolTip(D::AmpacityCheckColumn).contains(QLatin1String("needs its justification")));
	AmpacityTables::setFilePath(QString());
}

void tst_electricalpanel::tablesFileWithMistakes()
{
	QTemporaryDir dir;
	QByteArray json = syntheticTables();
	json.replace("[4, 30]", "[4, 15]");
	const QString message = m_dock->loadTables(writeTables(dir, json));
	QVERIFY2(message.contains(QLatin1String("0 ampacity table")), qPrintable(message));
	QVERIFY(message.contains(QLatin1String("Left out")));
	QVERIFY(message.contains(QLatin1String("a typo?")));
	AmpacityTables::setFilePath(QString());
}

void tst_electricalpanel::dialogSuggestsASection()
{
	QTemporaryDir dir;
	Circuit c = withAmpacityData(sampleCircuit(QUuid()));
	c.conductor.length = 40.0;   // 2.5 mm2: 4.64 %, 4 mm2: 2.90 %
	CircuitDialog dialog(c, {});
	dialog.setTables(readAmpacityJson(syntheticTables()).library);
	QVERIFY2(dialog.resultLabel()->text().contains(QStringLiteral("Suggested section: 4 mm²")),
			 qPrintable(dialog.resultLabel()->text()));
	QVERIFY(dialog.useSuggestionButton()->isEnabled());
	dialog.useSuggestionButton()->click();
	QCOMPARE(dialog.numberField(QStringLiteral("section"))->text(), QStringLiteral("4"));
	QCOMPARE(*dialog.circuit().conductor.section, 4.0);
	QVERIFY(!dialog.useSuggestionButton()->isEnabled());   // already used

		//What is typed reaches the circuit, reasons included
	dialog.numberField(QStringLiteral("groupingFactor"))->setText(QStringLiteral("0,8"));
	dialog.reasonField(QStringLiteral("groupingFactor"))->setText(QStringLiteral(" 2 circuitos "));
	dialog.numberField(QStringLiteral("loadedConductors"))->setText(QStringLiteral("3"));
	const Circuit read = dialog.circuit();
	QCOMPARE(*read.conductor.groupingFactor, 0.8);
	QCOMPARE(read.conductor.groupingFactorReason, QStringLiteral("2 circuitos"));
	QCOMPARE(*read.conductor.loadedConductors, 3);
	QVERIFY(dialog.isValid());
	dialog.numberField(QStringLiteral("loadedConductors"))->setText(QStringLiteral("4"));
	QVERIFY(!dialog.isValid());
}

namespace {

/// The chain of docs/rebt/coordinacion_h5.md: CGMP -> feeder -> CS -> C1
QUuid buildChain(ElectricalCircuitsDock *dock, QUuid *c1_id = nullptr)
{
	const QUuid cgmp = dock->createBoard(QStringLiteral("CGMP"));
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
	dock->applyCircuit(f);
	const QUuid cs = dock->createBoard(QStringLiteral("CS"));
	Board board = *dock->project()->electricalModel().board(cs);
	board.feeder = f.id;
	dock->applyBoard(board);
	Circuit c;
	c.id = QUuid::createUuid();
	c.name = QStringLiteral("C1");
	c.board = cs;
	c.use = CircuitUse::DwellingInterior;
	c.system = rebt::System::SinglePhase;
	c.voltage = 230.0;
	c.load.designCurrent = 16.0;
	c.load.powerFactor = 1.0;
	c.conductor.length = 25.0;
	c.conductor.section = 2.5;
	c.conductor.conductivity = 48.0;
	dock->applyCircuit(c);
	if (c1_id) *c1_id = c.id;
	return f.id;
}

QTreeWidgetItem *rowNamed(QTreeWidget *tree, const QString &name)
{
	for (QTreeWidgetItemIterator it(tree); *it; ++it)
		if ((*it)->text(0) == name) return *it;
	return nullptr;
}

} // namespace

void tst_electricalpanel::chainInThePanel()
{
	using D = ElectricalCircuitsDock;
	buildChain(m_dock);
	QTreeWidgetItem *c1 = rowNamed(m_dock->tree(), QStringLiteral("C1"));
	QVERIFY(c1);
		//2.90 % alone, 3.39 % from the origin: over 3 %
	QCOMPARE(c1->text(D::DropColumn), QStringLiteral("2,90"));
	QCOMPARE(c1->text(D::DropTotalColumn), QStringLiteral("3,39"));
	QCOMPARE(c1->text(D::CheckColumn), QStringLiteral("Not passed"));
	QVERIFY(c1->toolTip(D::CheckColumn).contains(QLatin1String("From the origin: 3,39 %")));
	QTreeWidgetItem *cs = rowNamed(m_dock->tree(), QStringLiteral("CS"));
	QVERIFY(cs->toolTip(0).contains(QStringLiteral("Fed by Alimentación")));
	QTreeWidgetItem *cgmp = rowNamed(m_dock->tree(), QStringLiteral("CGMP"));
	QVERIFY(cgmp->toolTip(0).contains(QLatin1String("origin of the interior installation")));
}

void tst_electricalpanel::removingAFeederFreesItsBoard()
{
	QUuid c1;
	const QUuid feeder = buildChain(m_dock, &c1);
	const ElectricalModel before = m_project->electricalModel();
	QVERIFY(m_dock->removeItem(feeder));
	for (const Board &b : m_project->electricalModel().boards())
		QVERIFY(b.feeder.isNull());
		//C1 alone again: 2.90 %, passed
	QCOMPARE(rowNamed(m_dock->tree(), QStringLiteral("C1"))
			 ->text(ElectricalCircuitsDock::CheckColumn), QStringLiteral("Passed"));
	m_project->undoStack()->undo();
	QVERIFY(m_project->electricalModel() == before);
}

void tst_electricalpanel::coordinationColumn()
{
	using D = ElectricalCircuitsDock;
	QTemporaryDir dir;
	m_dock->loadTables(writeTables(dir, syntheticTables()));
	const QUuid board = m_dock->createBoard(QStringLiteral("CGMP"));
	Circuit c = withAmpacityData(sampleCircuit(board));
	c.protection.type = ProtectionType::CircuitBreaker;
	c.protection.standard = rebt::ProtectionStandard::EN60898;
	QVERIFY(m_dock->applyCircuit(c));
		//P1: 16 <= 16 <= 17.32, I2 23.2 <= 25.11
	QTreeWidgetItem *row = m_dock->tree()->topLevelItem(0)->child(0);
	QCOMPARE(row->text(D::CoordinationColumn), QStringLiteral("Passed"));
	QVERIFY(row->toolTip(D::CoordinationColumn).contains(QStringLiteral("IN ≤ IZ: passed")));
	QVERIFY(row->toolTip(D::CoordinationColumn).contains(QLatin1String("ITC-BT-22 1.1")));
		//A 20 A breaker on that cable
	c.protection.ratedCurrent = 20.0;
	QVERIFY(m_dock->applyCircuit(c));
	row = m_dock->tree()->topLevelItem(0)->child(0);
	QCOMPARE(row->text(D::CoordinationColumn), QStringLiteral("Not passed"));
	QVERIFY(row->toolTip(D::CoordinationColumn).contains(QStringLiteral("IN ≤ IZ: not passed")));
	QVERIFY(m_dock->summaryLabel()->text().contains(QStringLiteral("Protection ↔ cable: 0 passed, 1 not passed")));
		//A residual current device alone: not performed, and why
	c.protection.type = ProtectionType::ResidualCurrent;
	QVERIFY(m_dock->applyCircuit(c));
	row = m_dock->tree()->topLevelItem(0)->child(0);
	QCOMPARE(row->text(D::CoordinationColumn), QStringLiteral("Not performed"));
	QVERIFY(row->toolTip(D::CoordinationColumn).contains(QLatin1String("does not protect the cable")));
	AmpacityTables::setFilePath(QString());
}

void tst_electricalpanel::boardDialogChoosesTheFeeder()
{
	const QUuid feeder = buildChain(m_dock);
	const ElectricalModel &model = m_project->electricalModel();
	Board cs;
	for (const Board &b : model.boards())
		if (b.name == QLatin1String("CS")) cs = b;
	BoardDialog dialog(cs, model);
	QCOMPARE(dialog.feederField()->currentData().toUuid(), feeder);
		//its own circuit C1 is not offered: a board cannot feed itself
	for (int i = 0; i < dialog.feederField()->count(); ++i)
		QVERIFY(!dialog.feederField()->itemText(i).endsWith(QStringLiteral("› C1")));
	dialog.feederField()->setCurrentIndex(0);
	QVERIFY(dialog.board().feeder.isNull());
	dialog.nameField()->setText(QStringLiteral(" CS garaje "));
	QCOMPARE(dialog.board().name, QStringLiteral("CS garaje"));
	QVERIFY(m_dock->applyBoard(dialog.board()));
	QVERIFY(m_project->electricalModel().board(cs.id)->feeder.isNull());
}

void tst_electricalpanel::dialogShowsTheDropFromTheOrigin()
{
	QUuid c1;
	buildChain(m_dock, &c1);
	const ElectricalModel &model = m_project->electricalModel();
	CircuitDialog dialog(*model.circuit(c1), model.boards());
	dialog.setModel(model);
	QVERIFY2(dialog.resultLabel()->text().contains(QLatin1String("from the origin: 3,39 %")),
			 qPrintable(dialog.resultLabel()->text()));
		//shorter, as it is typed: 20 m gives 2.32 + 0.49 = 2.81 %
	dialog.numberField(QStringLiteral("length"))->setText(QStringLiteral("20"));
	QVERIFY2(dialog.resultLabel()->text().contains(QLatin1String("from the origin: 2,81 %")),
			 qPrintable(dialog.resultLabel()->text()));
		//standard and I2 read back
	dialog.choiceField(QStringLiteral("protectionStandard"))->setCurrentIndex(4);   // gG
	dialog.numberField(QStringLiteral("conventionalCurrent"))->setText(QStringLiteral("19"));
	dialog.reasonField(QStringLiteral("conventionalCurrent"))->setText(QStringLiteral("catálogo"));
	const Circuit read = dialog.circuit();
	QCOMPARE(*read.protection.standard, rebt::ProtectionStandard::FuseGG);
	QCOMPARE(*read.protection.conventionalCurrent, 19.0);
	QCOMPARE(read.protection.conventionalCurrentReason, QStringLiteral("catálogo"));
}

void tst_electricalpanel::shortCircuitColumns()
{
	using D = ElectricalCircuitsDock;
	const QUuid board_id = m_dock->createBoard(QStringLiteral("CGMP"));
	Circuit c = withAmpacityData(sampleCircuit(board_id));
	c.protection.type = ProtectionType::CircuitBreaker;
	c.protection.standard = rebt::ProtectionStandard::EN60898;
	c.protection.breakingCapacity = 6.0;
	QVERIFY(m_dock->applyCircuit(c));
		//Supply not known: nothing computed, and the board says so
	QTreeWidgetItem *row = m_dock->tree()->topLevelItem(0)->child(0);
	QCOMPARE(row->text(D::MaxFaultColumn), QStringLiteral("—"));
	QCOMPARE(row->text(D::ShortCircuitColumn), QStringLiteral("Not performed"));
	QVERIFY(m_dock->tree()->topLevelItem(0)->toolTip(D::NameColumn)
				.contains(QLatin1String("Supply not known")));

		//6 kA / 3 kA at the board, from the distributor
	Board board = *m_project->electricalModel().board(board_id);
	board.source = rebt::SourceMethod::KnownCurrent;
	board.sourceMaxCurrent = 6.0;
	board.sourceMinCurrent = 3.0;
	board.sourceReason = QStringLiteral("Carta de la distribuidora");
	QVERIFY(m_dock->applyBoard(board));
	row = m_dock->tree()->topLevelItem(0)->child(0);
		//no feeder: the board's own 6 kA; minimum 230 / (230/3000 + 0.537931)
	QCOMPARE(row->text(D::MaxFaultColumn), QStringLiteral("6,00"));
	QCOMPARE(row->text(D::MinFaultColumn), QStringLiteral("0,37"));
	const QString tip = row->toolTip(D::ShortCircuitColumn);
	QVERIFY2(tip.contains(QStringLiteral("Breaking capacity ≥ Icc max: passed")), qPrintable(tip));
	QVERIFY(tip.contains(QLatin1String("instantaneous trip 160 A: passed")));
	QVERIFY(tip.contains(QLatin1String("let-through energy")));
	QVERIFY(tip.contains(QLatin1String("REBT-ICC-005")));
	QCOMPARE(row->text(D::ShortCircuitColumn), QStringLiteral("Not performed"));

		//with the I2t of the catalogue
	c.protection.letThrough = 15000.0;
	c.protection.letThroughReason = QStringLiteral("Curva del catálogo");
	QVERIFY(m_dock->applyCircuit(c));
	row = m_dock->tree()->topLevelItem(0)->child(0);
	QCOMPARE(row->text(D::ShortCircuitColumn), QStringLiteral("Passed"));
		//the values given, with their source
	const QString given = row->toolTip(D::ShortCircuitColumn);
	QVERIFY2(given.contains(QStringLiteral("I²t of the manufacturer: 15000 A²s (Curva del catálogo)")),
			 qPrintable(given));
	QVERIFY(given.contains(QStringLiteral("6 / 3 kA, CGMP (Carta de la distribuidora)")));
	QVERIFY(m_dock->summaryLabel()->text().contains(
		QStringLiteral("Short circuit: 1 passed, 0 not passed, 0 not performed.")));
		//that I2t given at 3 kA: not valid at the 6 kA of the board
	c.protection.letThroughAt = 3.0;
	QVERIFY(m_dock->applyCircuit(c));
	row = m_dock->tree()->topLevelItem(0)->child(0);
	QCOMPARE(row->text(D::ShortCircuitColumn), QStringLiteral("Not performed"));
	QVERIFY2(row->toolTip(D::ShortCircuitColumn)
				 .contains(QStringLiteral("I²t given at a current below the maximum Icc")),
			 qPrintable(row->toolTip(D::ShortCircuitColumn)));
		//given at 6 kA, the Icc max itself: used
	c.protection.letThroughAt = 6.0;
	QVERIFY(m_dock->applyCircuit(c));
	row = m_dock->tree()->topLevelItem(0)->child(0);
	QCOMPARE(row->text(D::ShortCircuitColumn), QStringLiteral("Passed"));
	c.protection.letThroughAt.reset();

		//a 4.5 kA breaker under 6 kA
	c.protection.breakingCapacity = 4.5;
	QVERIFY(m_dock->applyCircuit(c));
	row = m_dock->tree()->topLevelItem(0)->child(0);
	QCOMPARE(row->text(D::ShortCircuitColumn), QStringLiteral("Not passed"));
}

void tst_electricalpanel::boardDialogSupply()
{
	buildChain(m_dock);
	const ElectricalModel &model = m_project->electricalModel();
	Board cgmp, cs;
	for (const Board &b : model.boards()) {
		if (b.name == QLatin1String("CGMP")) cgmp = b;
		if (b.name == QLatin1String("CS")) cs = b;
	}
	{
		BoardDialog dialog(cgmp, model);
		QVERIFY(dialog.sourceField()->isEnabled());
			//not known by default, and nothing written for it
		QVERIFY(!dialog.board().source);
		QVERIFY(!dialog.sourceMaxField()->isEnabled());
		dialog.sourceField()->setCurrentIndex(
			dialog.sourceField()->findData(int(rebt::SourceMethod::KnownCurrent)));
		QVERIFY(dialog.sourceMaxField()->isEnabled());
		dialog.sourceMaxField()->setText(QStringLiteral("10"));
		dialog.sourceMinField()->setText(QStringLiteral("6,5"));
		dialog.sourceReasonField()->setText(QStringLiteral(" Carta "));
		const Board read = dialog.board();
		QCOMPARE(*read.source, rebt::SourceMethod::KnownCurrent);
		QCOMPARE(*read.sourceMaxCurrent, 10.0);
		QCOMPARE(*read.sourceMinCurrent, 6.5);
		QCOMPARE(read.sourceReason, QStringLiteral("Carta"));
			//the guide method keeps no values
		dialog.sourceField()->setCurrentIndex(
			dialog.sourceField()->findData(int(rebt::SourceMethod::GuideFactor)));
		QCOMPARE(*dialog.board().source, rebt::SourceMethod::GuideFactor);
		QVERIFY(!dialog.board().sourceMaxCurrent);
		QVERIFY(m_dock->applyBoard(dialog.board()));
	}
	QCOMPARE(*m_project->electricalModel().board(cgmp.id)->source,
			 rebt::SourceMethod::GuideFactor);
	{
			//a fed board takes its Icc from the line: no supply of its own
		Board with = cs;
		with.source = rebt::SourceMethod::GuideFactor;
		BoardDialog dialog(with, model);
		QVERIFY(!dialog.sourceField()->isEnabled());
		QVERIFY(!dialog.board().source);
	}
}

void tst_electricalpanel::dwellingColumn()
{
	using D = ElectricalCircuitsDock;
	const QUuid board_id = m_dock->createBoard(QStringLiteral("CGMP"));
		//sampleCircuit: 2.5 mm2, 25 m, gamma 48, C 16 A
	Circuit c = sampleCircuit(board_id);
	c.conductor.material = rebt::Material::Copper;
	c.dwellingCircuit = rebt::DwellingCircuit::C2;
	c.points = 12;
	c.conductor.conduit = 20.0;
	QVERIFY(m_dock->applyCircuit(c));
	QTreeWidgetItem *board = m_dock->tree()->topLevelItem(0);
	QTreeWidgetItem *row = board->child(0);
		//2 x 25 x 16 / (48 x 2.5) = 6.67 V = 2.90 %: within 3 %
	QCOMPARE(row->text(D::DwellingColumn), QStringLiteral("C2: Passed"));
	const QString tip = row->toolTip(D::DwellingColumn);
	QVERIFY2(tip.contains(QStringLiteral("ΔU with IN from the origin ≤ 3 %: 2,90 %")),
			 qPrintable(tip));
	QVERIFY(tip.contains(QStringLiteral("Points ≤ 20: passed")));
	QVERIFY(tip.contains(QStringLiteral("Conduit ≥ 20 mm: passed")));
	QVERIFY(tip.contains(QStringLiteral("REBT-VIV-001")));
		//the board: only C2 of C1 to C5, the grade not known
	QCOMPARE(board->text(D::DwellingColumn), QStringLiteral("Dwelling: not passed"));
	const QString dwelling_tip = board->toolTip(D::DwellingColumn);
	QVERIFY2(dwelling_tip.contains(QStringLiteral("Missing circuits: C1, C3, C4, C5")),
			 qPrintable(dwelling_tip));
	QVERIFY(dwelling_tip.contains(QStringLiteral("Grade of electrification: not known")));
	QVERIFY(dwelling_tip.contains(QStringLiteral("REBT-VIV-006")));
	QVERIFY(m_dock->summaryLabel()->text().contains(
		QStringLiteral("ITC-BT-25: 1 passed, 0 not passed, 0 not performed.")));
		//as a C1: a 16 A breaker and 2.5 mm2 on a 10 A circuit
	c.dwellingCircuit = rebt::DwellingCircuit::C1;
	QVERIFY(m_dock->applyCircuit(c));
	row = m_dock->tree()->topLevelItem(0)->child(0);
	QCOMPARE(row->text(D::DwellingColumn), QStringLiteral("C1: Not passed"));
	QVERIFY(row->toolTip(D::DwellingColumn).contains(QStringLiteral("Breaker ≤ 10 A: not passed")));
		//not a dwelling circuit: nothing shown, nothing counted
	c.dwellingCircuit.reset();
	QVERIFY(m_dock->applyCircuit(c));
	row = m_dock->tree()->topLevelItem(0)->child(0);
	QVERIFY(row->text(D::DwellingColumn).isEmpty());
	QVERIFY(m_dock->tree()->topLevelItem(0)->text(D::DwellingColumn).isEmpty());
	QVERIFY(!m_dock->summaryLabel()->text().contains(QStringLiteral("ITC-BT-25")));

		//the dialog reads and writes them
	c.dwellingCircuit = rebt::DwellingCircuit::C4Single;
	c.points = 1;
	CircuitDialog dialog(c, m_project->electricalModel().boards());
	QCOMPARE(dialog.choiceField(QStringLiteral("dwellingCircuit"))->currentData().toInt(),
			 int(rebt::DwellingCircuit::C4Single));
	dialog.numberField(QStringLiteral("points"))->setText(QStringLiteral("3"));
	dialog.choiceField(QStringLiteral("dwellingCircuit"))->setCurrentIndex(0);
	Circuit read = dialog.circuit();
	QVERIFY(!read.dwellingCircuit);
	QCOMPARE(*read.points, 3);

		//own transformer in the board dialog, kept only for an origin
	Board b = *m_project->electricalModel().board(board_id);
	BoardDialog board_dialog(b, m_project->electricalModel());
	QVERIFY(!board_dialog.ownTransformerField()->isChecked());
	board_dialog.ownTransformerField()->setChecked(true);
	QVERIFY(board_dialog.board().ownTransformer);
		//the dwelling and its residual current devices
	QVERIFY(board_dialog.areaField()->isEnabled());
	board_dialog.areaField()->setText(QStringLiteral("170,5"));
	board_dialog.generalSwitchField()->setText(QStringLiteral("40"));
	board_dialog.plannedField(QStringLiteral("heating"))->setCurrentIndex(2);   // No
	board_dialog.plannedField(QStringLiteral("singleFamily"))->setCurrentIndex(1);   // Yes
	board_dialog.addResidualDevice();
	board_dialog.residualTable()->item(0, 1)->setText(QStringLiteral("30"));
	board_dialog.residualTable()->item(0, 2)->setText(QStringLiteral("40"));
	Board read_board = board_dialog.board();
	QCOMPARE(*read_board.dwellingArea, 170.5);
	QCOMPARE(*read_board.generalSwitch, 40.0);
	QCOMPARE(*read_board.plannedHeating, false);
	QCOMPARE(*read_board.singleFamily, true);
	QVERIFY(!read_board.plannedAirConditioning);
	QCOMPARE(read_board.residualDevices.size(), 1);
	QCOMPARE(read_board.residualDevices.first().name, QStringLiteral("ID1"));
	QCOMPARE(*read_board.residualDevices.first().sensitivity, 30.0);
		//not a number: the dialog cannot be accepted
	board_dialog.residualTable()->item(0, 2)->setText(QStringLiteral("cuarenta"));
	QVERIFY(!board_dialog.findChild<QDialogButtonBox *>()
				 ->button(QDialogButtonBox::Ok)->isEnabled());
	board_dialog.residualTable()->item(0, 2)->setText(QStringLiteral("40"));
	QVERIFY(m_dock->applyBoard(board_dialog.board()));

		//the circuit names its device; the dialog offers those of the boards
	c.dwellingCircuit = rebt::DwellingCircuit::C2;
	c.points = 12;
	c.conductor.conduit = 20.0;
	CircuitDialog with_device(c, m_project->electricalModel().boards());
	QVERIFY(with_device.residualDeviceField()->findText(QStringLiteral("ID1")) >= 0);
	with_device.residualDeviceField()->setCurrentText(QStringLiteral("ID1"));
	with_device.numberField(QStringLiteral("conduit"))->setText(QStringLiteral("25"));
	const Circuit named = with_device.circuit();
	QCOMPARE(named.protection.residualDevice, QStringLiteral("ID1"));
	QCOMPARE(*named.conductor.conduit, 25.0);
	QVERIFY(m_dock->applyCircuit(named));
		//elevated by its area: C7 missing, among others
	const QString after = m_dock->tree()->topLevelItem(0)->toolTip(D::DwellingColumn);
	QVERIFY2(after.contains(QStringLiteral("elevated (useful area over 160 m²)")),
			 qPrintable(after));
	QVERIFY(after.contains(QStringLiteral("C7")));

		//a board fed by an interior circuit is not where a dwelling starts
	const QUuid sub = m_dock->createBoard(QStringLiteral("CS"));
	Board cs = *m_project->electricalModel().board(sub);
	cs.feeder = named.id;
	BoardDialog fed(cs, m_project->electricalModel());
	QVERIFY(!fed.areaField()->isEnabled());
	QVERIFY(fed.residualTable()->isEnabled());
}

void tst_electricalpanel::roomsDialog()
{
	RoomData kitchen;
	kitchen.name = QStringLiteral("Cocina");
	kitchen.type = rebt::RoomType::Kitchen;
	kitchen.area = 9.5;
	kitchen.counts = {{rebt::RoomItem::LightPoints, 1}, {rebt::RoomItem::WetSockets, 3}};
	RoomsDialog dialog({kitchen});
	QCOMPARE(dialog.table()->rowCount(), 1);
	QCOMPARE(dialog.rooms().first(), kitchen);
		//a new corridor, typed in
	dialog.addRoom();
	auto *type = qobject_cast<QComboBox *>(dialog.table()->cellWidget(1, 1));
	QVERIFY(type);
	type->setCurrentIndex(type->findData(int(rebt::RoomType::Corridor)));
	dialog.table()->item(1, 3)->setText(QStringLiteral("6,5"));
	dialog.table()->item(1, 4)->setText(QStringLiteral("2"));
	const int lights = 5 + RoomsDialog::items().indexOf(rebt::RoomItem::LightPoints);
	dialog.table()->item(1, lights)->setText(QStringLiteral("2"));
	QList<RoomData> rooms = dialog.rooms();
	QCOMPARE(rooms.size(), 2);
	QCOMPARE(*rooms.at(1).type, rebt::RoomType::Corridor);
	QCOMPARE(*rooms.at(1).length, 6.5);
	QCOMPARE(*rooms.at(1).accesses, 2);
	QCOMPARE(rooms.at(1).counts.value(rebt::RoomItem::LightPoints), 2);
	QVERIFY(!rooms.at(1).area);
		//not a whole number: refused
	dialog.table()->item(1, lights)->setText(QStringLiteral("2,5"));
	QVERIFY(!dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled());
	dialog.table()->item(1, lights)->setText(QStringLiteral("-1"));
	QVERIFY(!dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled());

		//in the board: the rooms reach the check and its details
	const QUuid board_id = m_dock->createBoard(QStringLiteral("CGMP"));
	Board board = *m_project->electricalModel().board(board_id);
	BoardDialog board_dialog(board, m_project->electricalModel());
	board_dialog.setRooms({kitchen});
	board_dialog.plannedField(QStringLiteral("dryer"))->setCurrentIndex(2);   // No
	QVERIFY(!board_dialog.compensateField()->isEnabled());   // fed by nothing
	board = board_dialog.board();
	QCOMPARE(board.rooms.size(), 1);
	QCOMPARE(*board.plannedDryer, false);
	QVERIFY(!board.compensateWithIndividualLine);
	QVERIFY(m_dock->applyBoard(board));
	Circuit c = sampleCircuit(board_id);
	c.dwellingCircuit = rebt::DwellingCircuit::C2;
	QVERIFY(m_dock->applyCircuit(c));
	const QString tip = m_dock->tree()->topLevelItem(0)->toolTip(
		ElectricalCircuitsDock::DwellingColumn);
		//kitchen 9.5 m2: 1 light of 1, worktop 3 of 3, C2 sockets unknown of 2
	QVERIFY2(tip.contains(QStringLiteral("Room Cocina: not passed")) == false
			 && tip.contains(QStringLiteral("Room Cocina: not performed")), qPrintable(tip));
	QVERIFY(tip.contains(QStringLiteral("light points 1/1")));
	QVERIFY(tip.contains(QStringLiteral("C2 sockets ?/2")));
	QVERIFY(tip.contains(QStringLiteral("C5 sockets 3/3")));
}

void tst_electricalpanel::electricalTab()
{
	QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
	QCOMPARE(project.state(), QETProject::Ok);
	Diagram *diagram = nullptr;
	for (Diagram *d : project.diagrams())
		if (d->elements().size() >= 2) {
			diagram = d;
			break;
		}
	QVERIFY(diagram);
	Element *breaker = diagram->elements().at(0);
	Element *other = diagram->elements().at(1);
	ElectricalCircuitsDock dock;
	dock.setProject(&project);
	const QUuid board = dock.createBoard(QStringLiteral("CGMP"));
	Circuit c1 = sampleCircuit(board);
	c1.protection.element = other->uuid();   // linked elsewhere for now
	QVERIFY(dock.applyCircuit(c1));
	const int undo_before = project.undoStack()->count();

		//The tab only with the setting on
	QSettings().setValue(ElectricalCircuitsDock::settingKey(), false);
	{
		ElementPropertiesWidget props(breaker);
		bool found = false;
		for (auto *tab : props.findChildren<ElectricalElementWidget *>()) found = tab;
		QVERIFY(!found);
	}
	QSettings().setValue(ElectricalCircuitsDock::settingKey(), true);
	ElementPropertiesWidget props(breaker);
	ElectricalElementWidget *tab = props.findChild<ElectricalElementWidget *>();
	QVERIFY(tab);
	QCOMPARE(tab->title(), QStringLiteral("Electrical"));
	QVERIFY(tab->rolesLabel()->text().contains(QStringLiteral("Not in the electrical model")));
	QVERIFY(!tab->associatedUndo());

		//This symbol becomes the board and the protection of C1
	tab->boardField()->setCurrentIndex(tab->boardField()->findData(board));
	tab->circuitField()->setCurrentIndex(tab->circuitField()->findData(c1.id));
		//the circuit's own device data come in, to edit them
	QCOMPARE(tab->textField(QStringLiteral("ratedCurrent"))->text(), QStringLiteral("16"));
	QCOMPARE(tab->textField(QStringLiteral("curve"))->text(), QStringLiteral("C"));
	tab->textField(QStringLiteral("manufacturer"))->setText(QStringLiteral(" Fabricante "));
	tab->textField(QStringLiteral("partNumber"))->setText(QStringLiteral("REF-1"));
	tab->textField(QStringLiteral("breakingCapacity"))->setText(QStringLiteral("6"));
	tab->choiceField(QStringLiteral("protectionStandard"))->setCurrentIndex(
		tab->choiceField(QStringLiteral("protectionStandard"))
			->findData(int(rebt::ProtectionStandard::EN60898)));
	QVERIFY(tab->rolesLabel()->text().contains(QStringLiteral("Board CGMP")));
	QVERIFY(tab->rolesLabel()->text().contains(QStringLiteral("Protection of CGMP › C1 Alumbrado")));
	QVERIFY2(tab->resultsLabel()->text().contains(QStringLiteral("Protection ↔ cable:")),
			 qPrintable(tab->resultsLabel()->text()));
		//not applied yet: the project is as it was
	QVERIFY(project.electricalModel().board(board)->element.isNull());

		//not a number: nothing applied
	tab->textField(QStringLiteral("sensitivity"))->setText(QStringLiteral("treinta"));
	QVERIFY(!tab->isValid());
	QVERIFY(!tab->associatedUndo());
	tab->textField(QStringLiteral("sensitivity"))->clear();

		//apply: one undo step, the link moved from the other symbol
	props.apply();
	QCOMPARE(project.undoStack()->count(), undo_before + 1);
	const ElectricalModel &after = project.electricalModel();
	QCOMPARE(after.board(board)->element, breaker->uuid());
	const Circuit applied = *after.circuit(c1.id);
	QCOMPARE(applied.protection.element, breaker->uuid());
	QCOMPARE(applied.protection.manufacturer, QStringLiteral("Fabricante"));
	QCOMPARE(applied.protection.partNumber, QStringLiteral("REF-1"));
	QCOMPARE(*applied.protection.breakingCapacity, 6.0);
	QCOMPARE(*applied.protection.ratedCurrent, 16.0);   // kept
	QCOMPARE(applied.name, c1.name);                     // nothing else touched
		//undo puts it back, and the tab shows it
	project.undoStack()->undo();
	QVERIFY(project.electricalModel().board(board)->element.isNull());
	QCOMPARE(project.electricalModel().circuit(c1.id)->protection.element, other->uuid());
	QCOMPARE(tab->circuitField()->currentData().toUuid(), QUuid());
	project.undoStack()->redo();
	QCOMPARE(tab->circuitField()->currentData().toUuid(), c1.id);

		//"None" unlinks
	tab->boardField()->setCurrentIndex(0);
	tab->circuitField()->setCurrentIndex(0);
	props.apply();
	QVERIFY(project.electricalModel().board(board)->element.isNull());
	QVERIFY(project.electricalModel().circuit(c1.id)->protection.element.isNull());
		//nothing to change: no empty undo step
	const int count = project.undoStack()->count();
	props.apply();
	QCOMPARE(project.undoStack()->count(), count);

		//live edit, as in the properties dock: applied at once
	tab->setLiveEdit(true);
	tab->circuitField()->setCurrentIndex(tab->circuitField()->findData(c1.id));
	emit tab->circuitField()->activated(tab->circuitField()->currentIndex());
	QCOMPARE(project.electricalModel().circuit(c1.id)->protection.element, breaker->uuid());
	tab->setLiveEdit(false);

		//A device of the catalogue: also the data the tab does not show
	const DeviceCatalogue catalogue = readDeviceCatalogueJson(testCatalogueJson());
	QCOMPARE(catalogue.devices.size(), 3);
	tab->applyDevice(catalogue.devices.at(0));
	QCOMPARE(tab->textField(QStringLiteral("partNumber"))->text(), QStringLiteral("MCB-C16-2P"));
	QCOMPARE(tab->textField(QStringLiteral("breakingCapacity"))->text(), QStringLiteral("6"));
	props.apply();
	{
		const ProtectionData pd = project.electricalModel().circuit(c1.id)->protection;
		QCOMPARE(pd.element, breaker->uuid());
		QCOMPARE(*pd.letThrough, 20000.0);
		QCOMPARE(*pd.letThroughAt, 6.0);
		QCOMPARE(pd.letThroughReason, QStringLiteral("Catalogue: test data"));
		QCOMPARE(*pd.poles, 2);
	}
		//another device, then a field changed by hand: what is not shown
		//is not taken from the catalogue for a device that is not its own
	tab->applyDevice(catalogue.devices.at(2));
	tab->textField(QStringLiteral("ratedCurrent"))->setText(QStringLiteral("80"));
	props.apply();
	{
		const ProtectionData pd = project.electricalModel().circuit(c1.id)->protection;
		QCOMPARE(*pd.ratedCurrent, 80.0);
		QCOMPARE(pd.partNumber, QStringLiteral("MCCB-100"));
		QVERIFY(!pd.tripCurrent);                 // not the 1000 A of the catalogue
		QCOMPARE(*pd.letThrough, 20000.0);        // as it was
	}
		//live edit: applied at once, with the data not shown
	tab->setLiveEdit(true);
	tab->applyDevice(catalogue.devices.at(2));
	{
		const ProtectionData pd = project.electricalModel().circuit(c1.id)->protection;
		QCOMPARE(*pd.ratedCurrent, 100.0);
		QCOMPARE(*pd.tripCurrent, 1000.0);
		QCOMPARE(pd.tripCurrentReason, QStringLiteral("Catalogue: test data p. 3"));
		QVERIFY(!pd.letThrough);
		QVERIFY(!pd.letThroughAt);
	}
	tab->setLiveEdit(false);

		//Only when asked: a picture of the tab, in the translation given
	const QString shot = qEnvironmentVariable("QET_ELECTRICAL_SCREENSHOT");
	if (!shot.isEmpty()) {
		QTranslator translator;
		const QString qm = qEnvironmentVariable("QET_ELECTRICAL_SCREENSHOT_QM");
		if (!qm.isEmpty() && translator.load(qm))
			QCoreApplication::installTranslator(&translator);
		Board b = *project.electricalModel().board(board);
		b.element = breaker->uuid();
		Circuit c = *project.electricalModel().circuit(c1.id);
		c.protection.manufacturer = QStringLiteral("Fabricante ejemplo");
		c.protection.partNumber = QStringLiteral("REF-C16-6KA");
		c.protection.type = ProtectionType::CircuitBreaker;
		c.protection.standard = rebt::ProtectionStandard::EN60898;
		c.protection.breakingCapacity = 6.0;
		c.conductor.material = rebt::Material::Copper;
		dock.applyBoard(b);
		dock.applyCircuit(c);
		ElementPropertiesWidget shown(breaker);
		auto *tabs = shown.findChild<QTabWidget *>();
		for (int i = 0; i < tabs->count(); ++i)
			if (qobject_cast<ElectricalElementWidget *>(tabs->widget(i)))
				tabs->setCurrentIndex(i);
		shown.resize(760, 900);
		shown.show();
		QTest::qWait(50);
		QString path = shot;
		path.replace(QLatin1String(".png"), QLatin1String("_tab.png"));
		QVERIFY(shown.grab().save(path));
		QCoreApplication::removeTranslator(&translator);
	}
	QSettings().setValue(ElectricalCircuitsDock::settingKey(), false);
}

void tst_electricalpanel::deviceCatalogue()
{
	const DeviceCatalogue catalogue = readDeviceCatalogueJson(testCatalogueJson());
	{
		DevicePickerDialog picker(catalogue);
		QCOMPARE(picker.list()->topLevelItemCount(), 3);
		auto ok = [&] { return picker.buttons()->button(QDialogButtonBox::Ok)->isEnabled(); };
		QVERIFY(!picker.entry());
		QVERIFY(!ok());
			//every word, in any field
		picker.searchField()->setText(QStringLiteral("other  p. 3"));
		int shown = 0;
		for (int i = 0; i < picker.list()->topLevelItemCount(); ++i)
			shown += picker.list()->topLevelItem(i)->isHidden() ? 0 : 1;
		QCOMPARE(shown, 1);
		picker.select(QStringLiteral(" other "), QStringLiteral("mccb-100"));
		QVERIFY(ok());
		QCOMPARE(picker.entry()->partNumber, QStringLiteral("MCCB-100"));
		QVERIFY(picker.detailsLabel()->text().contains(QStringLiteral("Source: test data p. 3")));
			//hidden by the search: not chosen any more
		picker.searchField()->setText(QStringLiteral("RCD"));
		QVERIFY(!picker.entry());
		QVERIFY(!ok());
			//nor when selected while the search hides it
		picker.select(QStringLiteral("OTHER"), QStringLiteral("MCCB-100"));
		QVERIFY(!picker.entry());
		picker.select(QStringLiteral("TEST"), QStringLiteral("RCD-40-30"));
		QCOMPARE(*picker.entry()->sensitivity, 30.0);
	}
	{
			//a file with mistakes says them
		const DeviceCatalogue wrong = readDeviceCatalogueJson(
			QByteArray(testCatalogueJson()).replace("\"source\": \"test data\",", ""));
		DevicePickerDialog picker(wrong);
		QCOMPARE(picker.list()->topLevelItemCount(), 2);
		QLabel *errors = picker.findChild<QLabel *>(QStringLiteral("errors"));
		QVERIFY(errors);
		QVERIFY(errors->text().contains(QStringLiteral("MCB-C16-2P")));
	}
	{
			//the circuit dialog: every datum of the device replaced
		Circuit c = sampleCircuit(QUuid::createUuid());
		c.protection.residualDevice = QStringLiteral("ID1");
		c.protection.tripCurrent = 500.0;
		c.protection.tripCurrentReason = QStringLiteral("old");
		CircuitDialog dialog(c, {});
		QVERIFY(dialog.catalogueButton());
		dialog.applyDevice(catalogue.devices.at(0));
		const ProtectionData pd = dialog.circuit().protection;
		QCOMPARE(pd.manufacturer, QStringLiteral("TEST"));
		QCOMPARE(*pd.type, ProtectionType::CircuitBreaker);
		QCOMPARE(*pd.standard, rebt::ProtectionStandard::EN60898);
		QCOMPARE(*pd.ratedCurrent, 16.0);
		QCOMPARE(pd.curve, QStringLiteral("C"));
		QCOMPARE(*pd.breakingCapacity, 6.0);
		QCOMPARE(*pd.poles, 2);
		QVERIFY(!pd.sensitivity);
		QVERIFY(!pd.tripCurrent);
		QVERIFY(pd.tripCurrentReason.isEmpty());
		QCOMPARE(*pd.letThrough, 20000.0);
		QCOMPARE(*pd.letThroughAt, 6.0);
		QCOMPARE(pd.letThroughReason, QStringLiteral("Catalogue: test data"));
		QCOMPARE(pd.residualDevice, QStringLiteral("ID1"));
		QCOMPARE(dialog.circuit().name, c.name);
			//the I2t test current is typed like the rest
		dialog.numberField(QStringLiteral("letThroughAt"))->setText(QStringLiteral("4,5"));
		QCOMPARE(*dialog.circuit().protection.letThroughAt, 4.5);
	}
	{
			//the panel chooses the file, and says what it read
		QTemporaryDir dir;
		const QString path = dir.filePath(QStringLiteral("aparatos.json"));
		QFile file(path);
		QVERIFY(file.open(QIODevice::WriteOnly));
		file.write(QByteArray(testCatalogueJson()).replace("\"poles\": 5", "")
				   .replace("\"poles\": 4", "\"poles\": 5"));
		file.close();
		const QString said = m_dock->loadDeviceCatalogue(path);
		QVERIFY2(said.contains(QStringLiteral("2 device(s) loaded from aparatos.json.")),
				 qPrintable(said));
		QVERIFY(said.contains(QStringLiteral("MCCB-100")));
		QCOMPARE(DeviceCatalogues::filePath(), path);
		QCOMPARE(DeviceCatalogues::current().devices.size(), 2);
		DeviceCatalogues::setFilePath(QString());
	}

		//Only when asked: a picture of the picker, in the translation given
	const QString shot = qEnvironmentVariable("QET_ELECTRICAL_SCREENSHOT");
	if (!shot.isEmpty()) {
		QTranslator translator;
		const QString qm = qEnvironmentVariable("QET_ELECTRICAL_SCREENSHOT_QM");
		if (!qm.isEmpty() && translator.load(qm))
			QCoreApplication::installTranslator(&translator);
		{
			DeviceCatalogue shown = readDeviceCatalogueJson(testCatalogueJson());
			shown.path = QStringLiteral("aparatos_prueba.json");
			DevicePickerDialog picker(shown);
			picker.select(QStringLiteral("TEST"), QStringLiteral("MCB-C16-2P"));
			picker.show();
			QTest::qWait(50);
			QString path = shot;
			path.replace(QLatin1String(".png"), QLatin1String("_catalogue.png"));
			QVERIFY(picker.grab().save(path));
		}
		QCoreApplication::removeTranslator(&translator);
	}
}

void tst_electricalpanel::indirectContacts()
{
	using D = ElectricalCircuitsDock;
	const QUuid board_id = m_dock->createBoard(QStringLiteral("CGMP"));
	Circuit c = sampleCircuit(board_id);
	c.protection.type = ProtectionType::CircuitBreaker;
	c.protection.standard = rebt::ProtectionStandard::EN60898;
	QVERIFY(m_dock->applyCircuit(c));
	QTreeWidgetItem *row = m_dock->tree()->topLevelItem(0)->child(0);
	QCOMPARE(row->text(D::IndirectColumn), QStringLiteral("Not performed"));
	QVERIFY2(row->toolTip(D::IndirectColumn).contains(
				 QStringLiteral("Missing: earthing system of the installation")),
			 qPrintable(row->toolTip(D::IndirectColumn)));
		//nothing given: the summary does not count it
	QVERIFY(!m_dock->summaryLabel()->text().contains(QStringLiteral("Indirect contacts")));

		//The dialog: RA and U only for TT
	Board board = *m_project->electricalModel().board(board_id);
	{
		BoardDialog dialog(board, m_project->electricalModel());
		QVERIFY(!dialog.earthResistanceField()->isEnabled());
		dialog.earthingField()->setCurrentIndex(
			dialog.earthingField()->findData(int(rebt::EarthingSystem::TT)));
		QVERIFY(dialog.earthResistanceField()->isEnabled());
		dialog.earthResistanceField()->setText(QStringLiteral("37,5"));
		dialog.earthResistanceReasonField()->setText(QStringLiteral(" Medida "));
		QVERIFY(!dialog.board().limitVoltage);
		board = dialog.board();
		QCOMPARE(*board.earthing, rebt::EarthingSystem::TT);
		QCOMPARE(*board.earthResistance, 37.5);
		QCOMPARE(board.earthResistanceReason, QStringLiteral("Medida"));
			//not a number: cannot be accepted
		dialog.limitVoltageField()->setText(QStringLiteral("veinticuatro"));
		QVERIFY(!dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled());
		dialog.limitVoltageField()->setText(QStringLiteral("24"));
		dialog.limitVoltageReasonField()->setText(QStringLiteral("Local mojado"));
		QCOMPARE(*dialog.board().limitVoltage, 24.0);
			//IT keeps RA and U, and has its own data
		QVERIFY(!dialog.firstFaultField()->isEnabled());
		dialog.earthingField()->setCurrentIndex(
			dialog.earthingField()->findData(int(rebt::EarthingSystem::IT)));
		QVERIFY(dialog.firstFaultField()->isEnabled());
		QVERIFY(dialog.earthResistanceField()->isEnabled());
		dialog.firstFaultField()->setText(QStringLiteral("0,5"));
		dialog.firstFaultReasonField()->setText(QStringLiteral(" Medida "));
		dialog.massesField()->setCurrentIndex(dialog.massesField()->findData(1));
		dialog.neutralDistributedField()->setCurrentIndex(
			dialog.neutralDistributedField()->findData(0));
		{
			const Board it = dialog.board();
			QCOMPARE(*it.earthing, rebt::EarthingSystem::IT);
			QCOMPARE(*it.earthResistance, 37.5);
			QCOMPARE(*it.firstFaultCurrent, 0.5);
			QCOMPARE(it.firstFaultReason, QStringLiteral("Medida"));
			QCOMPARE(*it.massesInterconnected, true);
			QCOMPARE(*it.neutralDistributed, false);
		}
			//TN keeps no RA, U nor the data of IT
		dialog.earthingField()->setCurrentIndex(
			dialog.earthingField()->findData(int(rebt::EarthingSystem::TN)));
		QVERIFY(!dialog.earthResistanceField()->isEnabled());
		QVERIFY(!dialog.firstFaultField()->isEnabled());
		QVERIFY(!dialog.board().earthResistance);
		QVERIFY(!dialog.board().limitVoltage);
		QVERIFY(!dialog.board().firstFaultCurrent);
		QVERIFY(!dialog.board().massesInterconnected);
		QVERIFY(dialog.earthingField()->findData(int(rebt::EarthingSystem::TNC)) > 0);
	}
	QVERIFY(m_dock->applyBoard(board));
	row = m_dock->tree()->topLevelItem(0)->child(0);
		//no RCD: the C16 breaker, Ia 160 A; 37.5 x 160 = 6000 V
	QCOMPARE(row->text(D::IndirectColumn), QStringLiteral("Not passed"));
	const QString tip = row->toolTip(D::IndirectColumn);
	QVERIFY2(tip.contains(QStringLiteral("Ia = instantaneous trip = 160 A")), qPrintable(tip));
	QVERIFY(tip.contains(QStringLiteral("REBT-CIN-001")));
	QVERIFY(m_dock->summaryLabel()->text().contains(
		QStringLiteral("Indirect contacts: 0 passed, 1 not passed, 0 not performed.")));
		//with a 30 mA RCD of the board: 37.5 x 0.030 = 1.125 V
	board.residualDevices = {{QStringLiteral("ID1"), 30.0, 40.0}};
	QVERIFY(m_dock->applyBoard(board));
	c.protection.residualDevice = QStringLiteral("ID1");
	QVERIFY(m_dock->applyCircuit(c));
	row = m_dock->tree()->topLevelItem(0)->child(0);
	QCOMPARE(row->text(D::IndirectColumn), QStringLiteral("Passed"));
	QVERIFY2(row->toolTip(D::IndirectColumn).contains(QStringLiteral("Ia = IΔn of ID1 = 30 mA")),
			 qPrintable(row->toolTip(D::IndirectColumn)));
	QVERIFY(row->toolTip(D::IndirectColumn).contains(QStringLiteral("Largest RA: U / Ia = 1666,67 Ω")));
}

void tst_electricalpanel::screenshot()
{
		//Only when asked: a picture of the panel for the documentation
	const QString path = qEnvironmentVariable("QET_ELECTRICAL_SCREENSHOT");
	if (path.isEmpty())
		QSKIP("QET_ELECTRICAL_SCREENSHOT not set");
		//In another language when a translation file is given
	QTranslator translator;
	const QString qm = qEnvironmentVariable("QET_ELECTRICAL_SCREENSHOT_QM");
	if (!qm.isEmpty()) {
		QVERIFY(translator.load(qm));
		QCoreApplication::installTranslator(&translator);
		delete m_dock;
		m_dock = new ElectricalCircuitsDock;
		m_dock->setProject(m_project);
	}

	const QUuid board = m_dock->createBoard(QStringLiteral("CGMP Vivienda"));
	Circuit c1 = sampleCircuit(board);
	m_dock->applyCircuit(c1);
	Circuit c2 = sampleCircuit(board);
	c2.id = QUuid::createUuid();
	c2.name = QStringLiteral("C2 Tomas uso general");
	c2.load.power = 3450.0;
	c2.conductor.length = 32.0;
	c2.protection.ratedCurrent = 16.0;
	m_dock->applyCircuit(c2);
	Circuit c3 = sampleCircuit(board);
	c3.id = QUuid::createUuid();
	c3.name = QStringLiteral("C3 Cocina y horno");
	c3.load.power = 5400.0;
	c3.conductor.section = 6.0;
	c3.conductor.length.reset();
	c3.protection.ratedCurrent = 25.0;
	m_dock->applyCircuit(c3);
	const QUuid sub = m_dock->createBoard(QStringLiteral("CS Garaje"));
	Circuit c4 = sampleCircuit(sub);
	c4.id = QUuid::createUuid();
	c4.name = QStringLiteral("Recarga VE");
	c4.system = rebt::System::ThreePhase;
	c4.voltage = 400.0;
	c4.use = CircuitUse::OtherUses;
	c4.load.power = 11000.0;
	c4.load.powerFactor = 0.95;
	c4.conductor.section = 6.0;
	c4.conductor.length = 40.0;
	c4.conductor.conductivity.reset();
	c4.conductor.material = rebt::Material::Copper;
	c4.conductor.temperature = 90.0;
	c4.protection.ratedCurrent = 20.0;
	m_dock->applyCircuit(c4);

	for (const Circuit &c : m_project->electricalModel().circuits()) {
		Circuit with = withAmpacityData(c);
			//Its own conductor data kept; three phase has no table here
		with.conductor.conductivity = c.conductor.conductivity;
		with.conductor.material = c.conductor.material ? c.conductor.material
													   : with.conductor.material;
		with.protection.type = ProtectionType::CircuitBreaker;
		with.protection.standard = rebt::ProtectionStandard::EN60898;
		m_dock->applyCircuit(with);
	}
		//The garage board fed from the main one
	Circuit feeder = withAmpacityData(sampleCircuit(board));
	feeder.id = QUuid::createUuid();
	feeder.name = QStringLiteral("Alimentación garaje");
	feeder.use = CircuitUse::OtherUses;
	feeder.system = rebt::System::ThreePhase;
	feeder.voltage = 400.0;
	feeder.load.power = 11000.0;
	feeder.load.powerFactor = 0.95;
	feeder.conductor.section = 10.0;
	feeder.conductor.length = 15.0;
	feeder.protection.ratedCurrent = 25.0;
	feeder.protection.type = ProtectionType::CircuitBreaker;
	feeder.protection.standard = rebt::ProtectionStandard::EN60898;
	m_dock->applyCircuit(feeder);
	Board garage = *m_project->electricalModel().board(sub);
	garage.feeder = feeder.id;
	m_dock->applyBoard(garage);
	QTemporaryDir tables_dir;
	m_dock->loadTables(writeTables(tables_dir, syntheticTables()));
	m_dock->resize(1450, 340);
	m_dock->show();
	QTest::qWait(50);
	QVERIFY(m_dock->grab().save(path));

	auto *dialog = new CircuitDialog(m_project->electricalModel().circuits().first(),
									 m_project->electricalModel().boards());
	dialog->resize(1000, 900);
	dialog->show();
	QTest::qWait(50);
	QString dialog_path = path;
	dialog_path.replace(QLatin1String(".png"), QLatin1String("_dialog.png"));
	QVERIFY(dialog->grab().save(dialog_path));
	delete dialog;

		//The board dialog, with a dwelling
	Board main = *m_project->electricalModel().board(board);
	main.dwellingArea = 120.0;
	main.plannedHeating = false;
	main.plannedAirConditioning = true;
	main.generalSwitch = 40.0;
	main.residualDevices = {{QStringLiteral("ID1"), 30.0, 40.0},
							{QStringLiteral("ID2"), 30.0, 40.0}};
	main.earthing = rebt::EarthingSystem::IT;
	main.earthResistance = 37.5;
	main.earthResistanceReason = QStringLiteral("Medida con telurómetro");
	main.firstFaultCurrent = 0.5;
	main.firstFaultReason = QStringLiteral("Medida con el controlador de aislamiento");
	main.massesInterconnected = true;
	main.neutralDistributed = false;
	auto *board_dialog = new BoardDialog(main, m_project->electricalModel());
	board_dialog->show();
	QTest::qWait(50);
	QString board_path = path;
	board_path.replace(QLatin1String(".png"), QLatin1String("_board.png"));
	QVERIFY(board_dialog->grab().save(board_path));
	delete board_dialog;

		//The rooms of the dwelling
	RoomData salon;
	salon.name = QStringLiteral("Salón");
	salon.type = rebt::RoomType::LivingRoom;
	salon.area = 22.0;
	salon.counts = {{rebt::RoomItem::LightPoints, 2}, {rebt::RoomItem::Switches, 2},
					{rebt::RoomItem::Sockets, 4}};
	RoomData pasillo;
	pasillo.name = QStringLiteral("Pasillo");
	pasillo.type = rebt::RoomType::Corridor;
	pasillo.length = 7.0;
	pasillo.accesses = 3;
	auto *rooms = new RoomsDialog({salon, pasillo});
	rooms->show();
	QTest::qWait(50);
	QString rooms_path = path;
	rooms_path.replace(QLatin1String(".png"), QLatin1String("_rooms.png"));
	QVERIFY(rooms->grab().save(rooms_path));
	delete rooms;
}

int main(int argc, char **argv)
{
	return QetAppTest::run<tst_electricalpanel>(argc, argv);
}

#include "tst_electricalpanel.moc"
