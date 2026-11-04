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
#include "electricalcircuitsdock.h"

#include "../../diagram.h"
#include "../../qetgraphicsitem/conductor.h"
#include "../../qetgraphicsitem/element.h"
#include "../../qetproject.h"
#include "boarddialog.h"
#include "changeelectricalmodelcommand.h"
#include "reportdialog.h"
#include "../../cli_export.h"
#include "../../titleblockproperties.h"
#include "circuitdialog.h"
#include "electrical/ampacityfile.h"
#include "electrical/devicecatalogue.h"
#include "electrical/electricalevaluation.h"
#include "electricaltexts.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QSet>
#include <QSettings>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

using namespace electrical;

namespace {

/// Boards fed by one of @p circuits are fed by nothing any more
void clearFeeders(ElectricalModel &model, const QSet<QUuid> &circuits)
{
	for (Board b : model.boards())
		if (circuits.contains(b.feeder)) {
			b.feeder = QUuid();
			model.setBoard(b);
		}
}

const int ID_ROLE = Qt::UserRole;
const int KIND_ROLE = Qt::UserRole + 1;
enum Kind { BoardKind, CircuitKind, NoBoardKind };

const QColor PASSED(0x1b, 0x7f, 0x3b);
const QColor FAILED(0xc6, 0x28, 0x28);
const QColor GREY(0x75, 0x75, 0x75);

} // namespace

QString ElectricalCircuitsDock::settingKey()
{
	return QStringLiteral("diagrameditor/electrical_circuits_panel");
}

bool ElectricalCircuitsDock::isEnabledInSettings()
{
	return QSettings().value(settingKey(), false).toBool();
}

ElectricalCircuitsDock::ElectricalCircuitsDock(QWidget *parent) :
	QDockWidget(tr("Electrical circuits"), parent)
{
	setObjectName(QStringLiteral("electrical_circuits_dock"));
	setAllowedAreas(Qt::AllDockWidgetAreas);
	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable
				| QDockWidget::DockWidgetFloatable);
	toggleViewAction()->setStatusTip(
		tr("Show or hide the boards and circuits, with their low voltage calculations"));

	auto *content = new QWidget(this);
	auto *layout = new QVBoxLayout(content);
	layout->setContentsMargins(2, 2, 2, 2);

	auto *tool_bar = new QToolBar(content);
	tool_bar->setToolButtonStyle(Qt::ToolButtonTextOnly);
	m_add_board = tool_bar->addAction(tr("Add board"), this,
									  &ElectricalCircuitsDock::addBoardInteractive);
	m_add_circuit = tool_bar->addAction(tr("Add circuit"), this,
										&ElectricalCircuitsDock::addCircuitInteractive);
	m_edit = tool_bar->addAction(tr("Edit"), this,
								 &ElectricalCircuitsDock::editCurrentInteractive);
	m_remove = tool_bar->addAction(tr("Remove"), this,
								   &ElectricalCircuitsDock::removeCurrentInteractive);

	auto *assign_menu = new QMenu(content);
	const QList<QPair<QString, Role>> roles{
		{tr("Selected symbol as origin"), Role::Origin},
		{tr("Selected symbol as destination"), Role::Destination},
		{tr("Selected symbol as protection"), Role::Protection},
		{tr("Selected conductors as the circuit's wires"), Role::Wires},
		{tr("Selected symbol as the board"), Role::BoardSymbol}};
	for (const auto &r : roles) {
		const Role role = r.second;
		assign_menu->addAction(r.first, this, [this, role] {
			const QString error = assignSelection(role);
			showMessage(error.isEmpty() ? tr("Linked to the drawing.") : error);
		});
	}
	m_assign = new QAction(tr("Link to drawing"), this);
	m_assign->setMenu(assign_menu);
	m_assign->setToolTip(tr("Link the symbols or conductors selected in the "
							"folio to the board or circuit of the current row"));
	tool_bar->addAction(m_assign);
	if (auto *button = qobject_cast<QToolButton *>(tool_bar->widgetForAction(m_assign)))
		button->setPopupMode(QToolButton::InstantPopup);
	m_show = tool_bar->addAction(tr("Show in drawing"), this, [this] {
		const int count = showInDrawing();
		showMessage(count ? tr("%n item(s) selected in the folio shown.", "", count)
						  : tr("Nothing it points at is on the folio shown."));
	});
	tool_bar->addSeparator();
	m_tables = tool_bar->addAction(tr("Ampacity tables..."), this,
								   &ElectricalCircuitsDock::loadTablesInteractive);
	tool_bar->addAction(tr("Device catalogue..."), this,
						&ElectricalCircuitsDock::loadDeviceCatalogueInteractive)
		->setToolTip(tr("The file of protective devices to choose from in the "
						"circuits and in the Electrical tab of the symbols"));
	m_tables->setToolTip(tr("Choose the file of ampacity tables (UNE-HD 60364-5-52 "
							"or a cable catalogue) the admissible currents are read "
							"in. The program ships none."));
	m_report = tool_bar->addAction(tr("Calculation report (PDF)..."), this,
								   &ElectricalCircuitsDock::exportReportInteractive);
	m_report->setToolTip(tr("A PDF with the folios and the calculation annex: "
							"characteristics of the boards, results and warnings, "
							"rules applied, with its title block"));
	layout->addWidget(tool_bar);

	m_message = new QLabel(content);
	m_message->setObjectName(QStringLiteral("electrical_message"));
	m_message->setWordWrap(true);
	m_message->hide();
	layout->addWidget(m_message);

	m_tree = new QTreeWidget(content);
	m_tree->setObjectName(QStringLiteral("electrical_circuits_tree"));
	m_tree->setColumnCount(ColumnCount);
	m_tree->setHeaderLabels({tr("Board / circuit"), tr("System"), tr("U (V)"),
							 tr("P (W)"), tr("L (m)"), tr("S (mm²)"),
							 tr("Protection"), tr("Ib (A)"), tr("ΔU (%)"),
							 tr("ΔU total (%)"), tr("ΔU check"), tr("Iz (A)"),
							 tr("Ib ≤ Iz"), tr("Protection ↔ cable"),
							 tr("Icc max (kA)"), tr("Icc min (kA)"),
							 tr("Short circuit"), tr("Indirect contacts"), tr("ITC-BT-25")});
	m_tree->setRootIsDecorated(true);
	m_tree->setAlternatingRowColors(true);
	m_tree->setUniformRowHeights(true);
	m_tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
	connect(m_tree, &QTreeWidget::currentItemChanged,
			this, &ElectricalCircuitsDock::updateActions);
	connect(m_tree, &QTreeWidget::itemDoubleClicked,
			this, &ElectricalCircuitsDock::editCurrentInteractive);
	layout->addWidget(m_tree, 1);

	m_summary = new QLabel(content);
	m_summary->setObjectName(QStringLiteral("electrical_summary"));
	m_summary->setWordWrap(true);
	layout->addWidget(m_summary);

	setWidget(content);
	refresh();
}

void ElectricalCircuitsDock::setProject(QETProject *project)
{
	if (project == m_project)
		return;
	disconnect(m_model_connection);
	m_project = project;
	if (m_project)
		m_model_connection = connect(m_project, &QETProject::electricalModelChanged,
									 this, &ElectricalCircuitsDock::refresh);
	m_message->hide();
	refresh();
}

void ElectricalCircuitsDock::setDiagramProvider(std::function<Diagram *()> provider)
{
	m_diagram_provider = std::move(provider);
}

QUuid ElectricalCircuitsDock::currentId() const
{
	const QTreeWidgetItem *item = m_tree->currentItem();
	return item ? item->data(0, ID_ROLE).toUuid() : QUuid();
}

void ElectricalCircuitsDock::setCurrentId(const QUuid &id)
{
	for (QTreeWidgetItemIterator it(m_tree); *it; ++it)
		if ((*it)->data(0, ID_ROLE).toUuid() == id && !id.isNull()) {
			m_tree->setCurrentItem(*it);
			return;
		}
}

QString ElectricalCircuitsDock::circuitName(const Circuit &circuit) const
{
	return circuit.name.isEmpty() ? tr("(unnamed circuit)") : circuit.name;
}

void ElectricalCircuitsDock::refresh()
{
	const QUuid current = currentId();
	m_tree->clear();

	if (!m_project) {
		m_summary->setText(tr("No project open."));
		updateActions();
		return;
	}

	const ElectricalModel &model = m_project->electricalModel();
	if (model.isReadOnly()) {
		m_message->setText(tr("The electrical data of this project were written "
							  "by a newer version. They are kept as they are and "
							  "cannot be edited here."));
		m_message->show();
	}

	int not_passed = 0, not_computable = 0, passed = 0;
	int iz_passed = 0, iz_not_passed = 0, iz_not_performed = 0;
	int co_passed = 0, co_not_passed = 0, co_not_performed = 0;
	int sc_passed = 0, sc_not_passed = 0, sc_not_performed = 0;
	int dw_passed = 0, dw_not_passed = 0, dw_not_performed = 0;
	int ic_passed = 0, ic_not_passed = 0, ic_not_performed = 0;
	bool earthing_given = false;
	for (const Board &b : model.boards())
		earthing_given = earthing_given || b.earthing.has_value();
	const rebt::AmpacityLibrary &tables = AmpacityTables::current().library;
	auto add_circuits = [&](QTreeWidgetItem *parent, const QUuid &board) {
		for (const Circuit &c : model.circuits()) {
			const bool here = board.isNull()
				? !model.board(c.board).has_value()
				: c.board == board;
			if (!here) continue;
			auto *item = new QTreeWidgetItem(parent);
			item->setData(0, ID_ROLE, c.id);
			item->setData(0, KIND_ROLE, CircuitKind);
			fillCircuitRow(item, c);
			const CircuitEvaluation e = evaluate(c, model, tables);
			switch (e.coordination.outcome) {
				case rebt::CheckOutcome::Passed: ++co_passed; break;
				case rebt::CheckOutcome::NotPassed: ++co_not_passed; break;
				case rebt::CheckOutcome::NotPerformed: ++co_not_performed; break;
			}
			switch (e.shortCircuit.outcome) {
				case rebt::CheckOutcome::Passed: ++sc_passed; break;
				case rebt::CheckOutcome::NotPassed: ++sc_not_passed; break;
				case rebt::CheckOutcome::NotPerformed: ++sc_not_performed; break;
			}
			switch (e.indirect.outcome) {
				case rebt::CheckOutcome::Passed: ++ic_passed; break;
				case rebt::CheckOutcome::NotPassed: ++ic_not_passed; break;
				case rebt::CheckOutcome::NotPerformed: ++ic_not_performed; break;
			}
			if (c.dwellingCircuit) {
				switch (e.dwelling.outcome) {
					case rebt::CheckOutcome::Passed: ++dw_passed; break;
					case rebt::CheckOutcome::NotPassed: ++dw_not_passed; break;
					case rebt::CheckOutcome::NotPerformed: ++dw_not_performed; break;
				}
			}
			switch (e.ampacityCheck.outcome) {
				case rebt::CheckOutcome::Passed: ++iz_passed; break;
				case rebt::CheckOutcome::NotPassed: ++iz_not_passed; break;
				case rebt::CheckOutcome::NotPerformed: ++iz_not_performed; break;
			}
			if (e.check.outcome == rebt::CheckOutcome::NotPerformed) ++not_computable;
			else if (e.check.outcome == rebt::CheckOutcome::NotPassed) ++not_passed;
			else if (e.check.outcome == rebt::CheckOutcome::Passed) ++passed;
		}
	};

	for (const Board &b : model.boards()) {
		auto *item = new QTreeWidgetItem(m_tree);
		item->setText(NameColumn, b.name.isEmpty() ? tr("(unnamed board)") : b.name);
		QFont font = item->font(NameColumn);
		font.setBold(true);
		item->setFont(NameColumn, font);
		item->setData(0, ID_ROLE, b.id);
		item->setData(0, KIND_ROLE, BoardKind);
		QStringList board_tip;
		if (b.element.isNull())
			board_tip << tr("Not linked to a symbol of the drawing");
		if (b.feeder.isNull()) {
			board_tip << tr("Fed by no circuit: origin of the interior installation");
			board_tip << (b.source ? ElectricalTexts::source(*b.source)
								   : tr("Supply not known: no short circuit"));
		} else if (auto feeder = model.circuit(b.feeder))
			board_tip << tr("Fed by %1").arg(circuitName(*feeder));
		if (b.feeder.isNull() && b.ownTransformer)
			board_tip << tr("Own transformer: limits of 4.5 % and 6.5 %");
		if (isInteriorOrigin(b, model)) {
				//The dwelling that starts here, as a whole
			const rebt::DwellingCheck dwelling = dwellingCheck(b.id, model, tables);
			if (!dwelling.rules.empty()) {
				item->setText(DwellingColumn, ElectricalTexts::dwellingCheckOutcome(dwelling));
				QColor color = GREY;
				if (dwelling.outcome == rebt::CheckOutcome::Passed) color = PASSED;
				if (dwelling.outcome == rebt::CheckOutcome::NotPassed) color = FAILED;
				item->setForeground(DwellingColumn, color);
				item->setToolTip(DwellingColumn, ElectricalTexts::dwellingCheckDetails(dwelling));
			}
		}
		item->setToolTip(NameColumn, board_tip.join(QLatin1Char('\n')));
		add_circuits(item, b.id);
		item->setExpanded(true);
	}
	if (!model.orphanCircuits().isEmpty()) {
		auto *item = new QTreeWidgetItem(m_tree);
		item->setText(NameColumn, tr("(no board)"));
		item->setForeground(NameColumn, GREY);
		item->setData(0, KIND_ROLE, NoBoardKind);
		add_circuits(item, QUuid());
		item->setExpanded(true);
	}

	const int total = model.circuits().size();
	m_summary->setText(total == 0
		? tr("No circuit yet. Add a board, then its circuits.")
		: tr("%n circuit(s). ΔU from the origin: %1 passed, %2 not passed, "
			 "%3 not performed.", "", total)
			  .arg(passed).arg(not_passed).arg(not_computable)
		  + QLatin1Char(' ')
		  + tr("IB ≤ IZ: %1 passed, %2 not passed, %3 not performed.")
			  .arg(iz_passed).arg(iz_not_passed).arg(iz_not_performed)
		  + QLatin1Char(' ')
		  + tr("Protection ↔ cable: %1 passed, %2 not passed, %3 not performed.")
			  .arg(co_passed).arg(co_not_passed).arg(co_not_performed)
		  + QLatin1Char(' ')
		  + tr("Short circuit: %1 passed, %2 not passed, %3 not performed.")
			  .arg(sc_passed).arg(sc_not_passed).arg(sc_not_performed)
		  + (!earthing_given ? QString()
			 : QLatin1Char(' ')
			   + tr("Indirect contacts: %1 passed, %2 not passed, %3 not performed.")
				 .arg(ic_passed).arg(ic_not_passed).arg(ic_not_performed))
		  + (dw_passed + dw_not_passed + dw_not_performed == 0 ? QString()
			 : QLatin1Char(' ')
			   + tr("ITC-BT-25: %1 passed, %2 not passed, %3 not performed.")
				 .arg(dw_passed).arg(dw_not_passed).arg(dw_not_performed))
		  + (tables.empty() ? QLatin1Char(' ') + tr("No ampacity table loaded.")
							: QString()));

	setCurrentId(current);
	updateActions();
}

void ElectricalCircuitsDock::fillCircuitRow(QTreeWidgetItem *item, const Circuit &c)
{
	item->setText(NameColumn, circuitName(c));
	item->setText(SystemColumn, c.system ? (*c.system == rebt::System::SinglePhase
											? tr("1~") : tr("3~"))
										 : QString());
	item->setText(VoltageColumn, formatNumber(c.voltage));
	item->setText(PowerColumn, formatNumber(c.load.power));
	item->setText(LengthColumn, formatNumber(c.conductor.length));
	item->setText(SectionColumn, formatNumber(c.conductor.section));
	QString protection;
	if (c.protection.ratedCurrent)
		protection = tr("%1 A").arg(formatNumber(c.protection.ratedCurrent));
	if (!c.protection.curve.isEmpty())
		protection = c.protection.curve + QLatin1Char(' ') + protection;
	item->setText(ProtectionColumn, protection.trimmed());

	const CircuitEvaluation e = m_project
		? evaluate(c, m_project->electricalModel(), AmpacityTables::current().library)
		: evaluate(c, AmpacityTables::current().library);
	item->setText(CurrentColumn, formatNumber(e.designCurrent(), 2));
	item->setText(DropColumn, e.drop.status == rebt::Status::Ok
								  ? formatNumber(e.drop.percent, 2)
								  : QStringLiteral("—"));
	item->setText(DropTotalColumn, e.cumulativePercent
									   ? formatNumber(e.cumulativePercent, 2)
									   : QStringLiteral("—"));
	item->setText(CheckColumn, ElectricalTexts::outcome(e));

	QColor color = GREY;
	if (e.limit && e.check.outcome == rebt::CheckOutcome::Passed) color = PASSED;
	if (e.limit && e.check.outcome == rebt::CheckOutcome::NotPassed) color = FAILED;
	item->setForeground(CheckColumn, color);
	if (color == FAILED) item->setForeground(DropTotalColumn, FAILED);

	const QString details = ElectricalTexts::details(e);
	for (int column = CurrentColumn; column <= CheckColumn; ++column)
		item->setToolTip(column, details);

	item->setText(AmpacityColumn, e.ampacity.status == rebt::Status::Ok
									  ? formatNumber(e.ampacity.admissible, 2)
									  : QStringLiteral("—"));
	item->setText(AmpacityCheckColumn, ElectricalTexts::ampacityOutcome(e));
	QColor iz_color = GREY;
	if (e.ampacityCheck.outcome == rebt::CheckOutcome::Passed) iz_color = PASSED;
	if (e.ampacityCheck.outcome == rebt::CheckOutcome::NotPassed) iz_color = FAILED;
	item->setForeground(AmpacityCheckColumn, iz_color);
	const QString iz_details = ElectricalTexts::ampacityDetails(e)
							   + QLatin1Char('\n') + ElectricalTexts::suggestion(e);
	item->setToolTip(AmpacityColumn, iz_details);
	item->setToolTip(AmpacityCheckColumn, iz_details);
	item->setTextAlignment(AmpacityColumn, Qt::AlignRight | Qt::AlignVCenter);
	item->setTextAlignment(DropTotalColumn, Qt::AlignRight | Qt::AlignVCenter);

	item->setText(CoordinationColumn, ElectricalTexts::coordinationOutcome(e));
	QColor co_color = GREY;
	if (e.coordination.outcome == rebt::CheckOutcome::Passed) co_color = PASSED;
	if (e.coordination.outcome == rebt::CheckOutcome::NotPassed) co_color = FAILED;
	item->setForeground(CoordinationColumn, co_color);
	item->setToolTip(CoordinationColumn, ElectricalTexts::coordinationDetails(e));

	auto kilo = [](const std::optional<double> &a) {
		return a ? formatNumber(*a / 1000.0, 2) : QStringLiteral("—");
	};
	item->setText(MaxFaultColumn, kilo(e.shortCircuit.maxCurrent));
	item->setText(MinFaultColumn, kilo(e.shortCircuit.minCurrent));
	item->setText(ShortCircuitColumn, ElectricalTexts::shortCircuitOutcome(e));
	QColor sc_color = GREY;
	if (e.shortCircuit.outcome == rebt::CheckOutcome::Passed) sc_color = PASSED;
	if (e.shortCircuit.outcome == rebt::CheckOutcome::NotPassed) sc_color = FAILED;
	item->setForeground(ShortCircuitColumn, sc_color);
	const QString sc_details = ElectricalTexts::shortCircuitDetails(e);
	for (int column : {MaxFaultColumn, MinFaultColumn, ShortCircuitColumn})
		item->setToolTip(column, sc_details);
	item->setTextAlignment(MaxFaultColumn, Qt::AlignRight | Qt::AlignVCenter);

	item->setText(IndirectColumn, ElectricalTexts::indirectOutcome(e));
	QColor ic_color = GREY;
	if (e.indirect.outcome == rebt::CheckOutcome::Passed) ic_color = PASSED;
	if (e.indirect.outcome == rebt::CheckOutcome::NotPassed) ic_color = FAILED;
	item->setForeground(IndirectColumn, ic_color);
	item->setToolTip(IndirectColumn, ElectricalTexts::indirectDetails(e));

	if (c.dwellingCircuit) {
		item->setText(DwellingColumn, ElectricalTexts::dwellingCode(*c.dwellingCircuit)
									  + QStringLiteral(": ")
									  + ElectricalTexts::dwellingOutcome(e));
		QColor dw_color = GREY;
		if (e.dwelling.outcome == rebt::CheckOutcome::Passed) dw_color = PASSED;
		if (e.dwelling.outcome == rebt::CheckOutcome::NotPassed) dw_color = FAILED;
		item->setForeground(DwellingColumn, dw_color);
		item->setToolTip(DwellingColumn,
						 ElectricalTexts::dwellingCircuit(*c.dwellingCircuit)
						 + QLatin1Char('\n') + ElectricalTexts::dwellingDetails(e));
	}
	item->setTextAlignment(MinFaultColumn, Qt::AlignRight | Qt::AlignVCenter);
	if (!c.notes.isEmpty())
		item->setToolTip(NameColumn, c.notes);
	item->setTextAlignment(VoltageColumn, Qt::AlignRight | Qt::AlignVCenter);
	for (int column : {PowerColumn, LengthColumn, SectionColumn, CurrentColumn, DropColumn})
		item->setTextAlignment(column, Qt::AlignRight | Qt::AlignVCenter);
}

void ElectricalCircuitsDock::updateActions()
{
	const bool editable = m_project && !m_project->electricalModel().isReadOnly();
	const QTreeWidgetItem *item = m_tree->currentItem();
	const int kind = item ? item->data(0, KIND_ROLE).toInt() : -1;
	const bool row = item && (kind == BoardKind || kind == CircuitKind);
	m_add_board->setEnabled(editable);
	m_add_circuit->setEnabled(editable);
	m_edit->setEnabled(editable && row);
	m_remove->setEnabled(editable && row);
	m_assign->setEnabled(editable && row);
	m_show->setEnabled(m_project && row);
	m_report->setEnabled(editable && !m_project->electricalModel().circuits().isEmpty());
}

bool ElectricalCircuitsDock::push(const ElectricalModel &model, const QString &text)
{
	return ChangeElectricalModelCommand::push(m_project, model, text);
}

void ElectricalCircuitsDock::showMessage(const QString &text)
{
	m_message->setText(text);
	m_message->setVisible(!text.isEmpty());
}

QUuid ElectricalCircuitsDock::createBoard(const QString &name)
{
	if (!m_project) return QUuid();
	ElectricalModel model = m_project->electricalModel();
	Board board;
	board.id = QUuid::createUuid();
	board.name = name.trimmed();
	if (!model.setBoard(board) || !push(model, tr("Add board %1").arg(board.name)))
		return QUuid();
	setCurrentId(board.id);
	return board.id;
}

QUuid ElectricalCircuitsDock::createCircuit(const QUuid &board, const QString &name)
{
	if (!m_project) return QUuid();
	ElectricalModel model = m_project->electricalModel();
	Circuit circuit;
	circuit.id = QUuid::createUuid();
	circuit.board = board;
	circuit.name = name.trimmed();
	if (!model.setCircuit(circuit)
		|| !push(model, tr("Add circuit %1").arg(circuitName(circuit))))
		return QUuid();
	setCurrentId(circuit.id);
	return circuit.id;
}

bool ElectricalCircuitsDock::applyCircuit(const Circuit &circuit)
{
	if (!m_project) return false;
	ElectricalModel model = m_project->electricalModel();
	if (!model.setCircuit(circuit)) return false;
	const bool done = push(model, tr("Edit circuit %1").arg(circuitName(circuit)));
	setCurrentId(circuit.id);
	return done;
}

bool ElectricalCircuitsDock::renameBoard(const QUuid &id, const QString &name)
{
	if (!m_project) return false;
	ElectricalModel model = m_project->electricalModel();
	auto board = model.board(id);
	if (!board) return false;
	board->name = name.trimmed();
	return model.setBoard(*board) && push(model, tr("Rename board %1").arg(board->name));
}

bool ElectricalCircuitsDock::applyBoard(const Board &board)
{
	if (!m_project) return false;
	ElectricalModel model = m_project->electricalModel();
	if (!model.board(board.id) || !model.setBoard(board)) return false;
	return push(model, tr("Edit board %1").arg(board.name));
}

bool ElectricalCircuitsDock::removeItem(const QUuid &id)
{
	if (!m_project) return false;
	ElectricalModel model = m_project->electricalModel();
	if (auto board = model.board(id)) {
			//Its circuits go with it: a circuit left without its board
			//would hold data nobody sees the origin of
		QSet<QUuid> removed;
		for (const Circuit &c : model.circuits())
			if (c.board == id) removed << c.id;
		for (const QUuid &c : removed) model.removeCircuit(c);
		model.removeBoard(id);
		clearFeeders(model, removed);
		return push(model, tr("Remove board %1").arg(board->name));
	}
	if (auto circuit = model.circuit(id)) {
		model.removeCircuit(id);
			//A board it fed is then fed by nothing, never by a ghost
		clearFeeders(model, {id});
		return push(model, tr("Remove circuit %1").arg(circuitName(*circuit)));
	}
	return false;
}

QString ElectricalCircuitsDock::assignSelection(Role role)
{
	if (!m_project) return tr("No project open.");
	Diagram *diagram = m_diagram_provider ? m_diagram_provider() : nullptr;
	if (!diagram) return tr("No folio is shown.");

	QList<QUuid> elements, wires;
	for (QGraphicsItem *item : diagram->selectedItems()) {
		if (auto *element = qgraphicsitem_cast<Element *>(item))
			elements << element->uuid();
		else if (auto *conductor = qgraphicsitem_cast<Conductor *>(item))
			wires << conductor->uuid();
	}

	ElectricalModel model = m_project->electricalModel();
	const QUuid id = currentId();

	if (role == Role::BoardSymbol) {
		auto board = model.board(id);
		if (!board) return tr("Choose a board in the list first.");
		if (elements.size() != 1) return tr("Select exactly one symbol in the folio.");
		board->element = elements.first();
		model.setBoard(*board);
		return push(model, tr("Link board %1 to its symbol").arg(board->name))
			? QString() : tr("Nothing changed.");
	}

	auto circuit = model.circuit(id);
	if (!circuit) return tr("Choose a circuit in the list first.");
	if (role == Role::Wires) {
		if (wires.isEmpty()) return tr("Select one or more conductors in the folio.");
		circuit->conductor.wires = wires;
	} else {
		if (elements.size() != 1) return tr("Select exactly one symbol in the folio.");
		if (role == Role::Origin) circuit->origin = elements.first();
		else if (role == Role::Destination) circuit->destination = elements.first();
		else circuit->protection.element = elements.first();
	}
	model.setCircuit(*circuit);
	return push(model, tr("Link circuit %1 to the drawing").arg(circuitName(*circuit)))
		? QString() : tr("Nothing changed.");
}

int ElectricalCircuitsDock::showInDrawing()
{
	if (!m_project) return 0;
	Diagram *diagram = m_diagram_provider ? m_diagram_provider() : nullptr;
	if (!diagram) return 0;

	const ElectricalModel &model = m_project->electricalModel();
	QSet<QUuid> wanted;
	const QUuid id = currentId();
	if (auto board = model.board(id)) {
		wanted << board->element;
	} else if (auto circuit = model.circuit(id)) {
		wanted << circuit->origin << circuit->destination << circuit->protection.element;
		for (const QUuid &w : circuit->conductor.wires) wanted << w;
	}
	wanted.remove(QUuid());

	diagram->clearSelection();
	int count = 0;
	for (QGraphicsItem *item : diagram->items()) {
		QUuid uuid;
		if (auto *element = qgraphicsitem_cast<Element *>(item)) uuid = element->uuid();
		else if (auto *conductor = qgraphicsitem_cast<Conductor *>(item)) uuid = conductor->uuid();
		if (!uuid.isNull() && wanted.contains(uuid)) {
			item->setSelected(true);
			++count;
		}
	}
	return count;
}

void ElectricalCircuitsDock::addBoardInteractive()
{
	if (!m_project) return;
	Board board;
	board.id = QUuid::createUuid();
	BoardDialog dialog(board, m_project->electricalModel(), this);
	dialog.setWindowTitle(tr("Add board"));
	if (dialog.exec() != QDialog::Accepted) return;
	ElectricalModel model = m_project->electricalModel();
	const Board added = dialog.board();
	model.setBoard(added);
	if (push(model, tr("Add board %1").arg(added.name)))
		setCurrentId(added.id);
}

void ElectricalCircuitsDock::addCircuitInteractive()
{
	if (!m_project) return;
	QUuid board;
	const QUuid id = currentId();
	const ElectricalModel &model = m_project->electricalModel();
	if (model.board(id)) board = id;
	else if (auto c = model.circuit(id)) board = c->board;

	Circuit circuit;
	circuit.id = QUuid::createUuid();
	circuit.board = board;
	CircuitDialog dialog(circuit, model.boards(), this);
	dialog.setModel(model);
	dialog.setWindowTitle(tr("Add circuit"));
	if (dialog.exec() != QDialog::Accepted) return;
	ElectricalModel changed = model;
	const Circuit added = dialog.circuit();
	changed.setCircuit(added);
	if (push(changed, tr("Add circuit %1").arg(circuitName(added))))
		setCurrentId(added.id);
}

void ElectricalCircuitsDock::editCurrentInteractive()
{
	if (!m_project || m_project->electricalModel().isReadOnly()) return;
	const ElectricalModel &model = m_project->electricalModel();
	const QUuid id = currentId();
	if (auto board = model.board(id)) {
		BoardDialog dialog(*board, model, this);
		if (dialog.exec() == QDialog::Accepted)
			applyBoard(dialog.board());
		return;
	}
	auto circuit = model.circuit(id);
	if (!circuit) return;
	CircuitDialog dialog(*circuit, model.boards(), this);
	dialog.setModel(model);
	if (dialog.exec() == QDialog::Accepted)
		applyCircuit(dialog.circuit());
}

void ElectricalCircuitsDock::removeCurrentInteractive()
{
	if (!m_project) return;
	const QUuid id = currentId();
	const ElectricalModel &model = m_project->electricalModel();
	if (auto board = model.board(id)) {
		int count = 0;
		for (const Circuit &c : model.circuits())
			if (c.board == id) ++count;
		if (count && QMessageBox::question(
				this, tr("Remove board"),
				tr("Remove the board and its %n circuit(s)?", "", count))
			!= QMessageBox::Yes)
			return;
	}
	removeItem(id);
}

QString ElectricalCircuitsDock::loadTables(const QString &path)
{
	AmpacityTables::setFilePath(path);
	const AmpacityFile &file = AmpacityTables::current();
	refresh();
	QString text = tr("%n ampacity table(s) loaded from %1.", "",
					  int(file.library.tables().size()))
				   .arg(QFileInfo(path).fileName());
	if (!file.errors.isEmpty())
		text += QLatin1Char('\n') + tr("Left out:") + QLatin1Char('\n')
				+ file.errors.join(QLatin1Char('\n'));
	return text;
}

void ElectricalCircuitsDock::loadTablesInteractive()
{
	const QString path = QFileDialog::getOpenFileName(
		this, tr("Ampacity tables"), AmpacityTables::filePath(),
		tr("Ampacity tables (*.json)"));
	if (!path.isEmpty())
		showMessage(loadTables(path));
}

QString ElectricalCircuitsDock::loadDeviceCatalogue(const QString &path)
{
	DeviceCatalogues::setFilePath(path);
	const DeviceCatalogue &file = DeviceCatalogues::current();
	QString text = tr("%n device(s) loaded from %1.", "", int(file.devices.size()))
				   .arg(QFileInfo(path).fileName());
	if (!file.errors.isEmpty())
		text += QLatin1Char('\n') + tr("Left out:") + QLatin1Char('\n')
				+ file.errors.join(QLatin1Char('\n'));
	return text;
}

void ElectricalCircuitsDock::loadDeviceCatalogueInteractive()
{
	const QString path = QFileDialog::getOpenFileName(
		this, tr("Device catalogue"), DeviceCatalogues::filePath(),
		tr("Device catalogues (*.json)"));
	if (!path.isEmpty())
		showMessage(loadDeviceCatalogue(path));
}

QString ElectricalCircuitsDock::exportReport(const QString &path, const ReportInfo &info,
											 bool withFolios)
{
	if (!m_project) return tr("No project open.");
	ElectricalModel model = m_project->electricalModel();
	if (!(model.reportInfo() == info) && model.setReportInfo(info))
		push(model, tr("Edit the data of the calculation report"));
	QString error;
	if (CLIExport::exportCalculations(*m_project, path, AmpacityTables::filePath(),
									  withFolios, &error) != 0)
		return error.isEmpty() ? tr("Cannot write %1").arg(path) : error;
	return QString();
}

void ElectricalCircuitsDock::exportReportInteractive()
{
	if (!m_project) return;
	const TitleBlockProperties folio = m_project->defaultTitleBlockProperties();
	ReportDialog dialog(ReportDialog::proposal(m_project->electricalModel().reportInfo(),
											   folio.author, folio.indexrev), this);
	if (dialog.exec() != QDialog::Accepted) return;
	QString suggested = m_project->filePath();
	suggested = suggested.isEmpty() ? tr("calculations.pdf")
									: QFileInfo(suggested).path() + QLatin1Char('/')
									  + QFileInfo(suggested).completeBaseName()
									  + tr("_calculations.pdf");
	const QString path = QFileDialog::getSaveFileName(this, tr("Calculation report"),
													  suggested, tr("PDF (*.pdf)"));
	if (path.isEmpty()) return;
	const QString error = exportReport(path, dialog.info(), dialog.withFolios());
	showMessage(error.isEmpty() ? tr("Report written: %1").arg(QFileInfo(path).fileName())
								: error);
}
