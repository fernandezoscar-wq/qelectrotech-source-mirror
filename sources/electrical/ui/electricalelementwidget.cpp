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
#include "electricalelementwidget.h"

#include "changeelectricalmodelcommand.h"
#include "electrical/ampacityfile.h"
#include "electrical/electricalevaluation.h"
#include "devicepickerdialog.h"
#include "electricaltexts.h"
#include "../../diagram.h"
#include "../../qetgraphicsitem/element.h"
#include "../../qetproject.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QUndoStack>
#include <QVBoxLayout>

using namespace electrical;

namespace {

const char *const NUMBERS[] = {"ratedCurrent", "breakingCapacity", "sensitivity", "poles"};

QString circuitLabel(const ElectricalModel &model, const Circuit &c)
{
	const QString name = c.name.isEmpty() ? ElectricalElementWidget::tr("(unnamed circuit)")
										  : c.name;
	if (const auto b = model.board(c.board))
		return QStringLiteral("%1 › %2").arg(
			b->name.isEmpty() ? ElectricalElementWidget::tr("(unnamed board)") : b->name, name);
	return name;
}

QString boardLabel(const Board &b)
{
	return b.name.isEmpty() ? ElectricalElementWidget::tr("(unnamed board)") : b.name;
}

} // namespace

ElectricalElementWidget::ElectricalElementWidget(Element *element, QWidget *parent) :
	AbstractElementPropertiesEditorWidget(parent)
{
	buildGui();
	setElement(element);
}

QString ElectricalElementWidget::title() const
{
	return tr("Electrical");
}

QETProject *ElectricalElementWidget::project() const
{
	if (!m_element || !m_element->diagram())
		return nullptr;
	return m_element->diagram()->project();
}

void ElectricalElementWidget::setElement(Element *element)
{
	if (m_model_changed)
		disconnect(m_model_changed);
	m_element = element;
		//Changes made elsewhere (the panel, undo) show here at once
	if (QETProject *p = project())
		m_model_changed = connect(p, &QETProject::electricalModelChanged,
								  this, &ElectricalElementWidget::updateUi);
	updateUi();
}

QLineEdit *ElectricalElementWidget::addText(QFormLayout *form, const QString &name,
											const QString &label, const QString &unit)
{
	auto *edit = new QLineEdit;
	edit->setObjectName(name);
	edit->setPlaceholderText(tr("unknown"));
	m_texts.insert(name, edit);
	connect(edit, &QLineEdit::textChanged, this, &ElectricalElementWidget::updateState);
	if (unit.isEmpty()) {
		form->addRow(label, edit);
	} else {
		auto *row = new QHBoxLayout;
		row->addWidget(edit);
		row->addWidget(new QLabel(unit));
		form->addRow(label, row);
	}
	return edit;
}

QComboBox *ElectricalElementWidget::addChoice(QFormLayout *form, const QString &name,
											  const QString &label,
											  const QList<QPair<QString, int>> &choices)
{
	auto *combo = new QComboBox;
	combo->setObjectName(name);
	combo->addItem(tr("Unknown"), -1);
	for (const auto &c : choices)
		combo->addItem(c.first, c.second);
	m_choices.insert(name, combo);
	connect(combo, &QComboBox::currentIndexChanged, this, &ElectricalElementWidget::updateState);
	form->addRow(label, combo);
	return combo;
}

void ElectricalElementWidget::buildGui()
{
	auto *layout = new QVBoxLayout(this);

	m_read_only = new QLabel(tr("The electrical data of this project were written by "
								"a newer version: they can only be read here."));
	m_read_only->setWordWrap(true);
	m_read_only->hide();
	layout->addWidget(m_read_only);

		//What the symbol is (identifying)
	auto *identity = new QGroupBox(tr("What this symbol is"));
	auto *identity_layout = new QVBoxLayout(identity);
	m_roles = new QLabel;
	m_roles->setObjectName(QStringLiteral("roles"));
	m_roles->setWordWrap(true);
	identity_layout->addWidget(m_roles);
	auto *identity_form = new QFormLayout;
	m_board = new QComboBox;
	m_board->setObjectName(QStringLiteral("board"));
	connect(m_board, &QComboBox::currentIndexChanged, this, &ElectricalElementWidget::updateState);
	identity_form->addRow(tr("Board drawn by this symbol"), m_board);
	m_circuit = new QComboBox;
	m_circuit->setObjectName(QStringLiteral("circuit"));
		//Another circuit: its own device data, to edit them
	connect(m_circuit, &QComboBox::currentIndexChanged, this, [this] {
		if (m_filling) return;
		ProtectionData protection;
		if (QETProject *p = project())
			if (const auto c = p->electricalModel().circuit(m_circuit->currentData().toUuid()))
				protection = c->protection;
		m_filling = true;
		fillProtection(protection);
		m_filling = false;
		updateState();
	});
	identity_form->addRow(tr("Protection of the circuit"), m_circuit);
	identity_layout->addLayout(identity_form);
	auto *hint = new QLabel(tr("A board or circuit linked to another symbol is moved "
							   "to this one. Boards and circuits are created in the "
							   "Electrical circuits panel."));
	hint->setWordWrap(true);
	identity_layout->addWidget(hint);
	layout->addWidget(identity);

		//The protective device (descriptive)
	m_protection_box = new QGroupBox(tr("Protective device"));
	auto *protection_form = new QFormLayout(m_protection_box);
	m_catalogue = new QPushButton(tr("From the catalogue..."));
	m_catalogue->setObjectName(QStringLiteral("catalogue"));
	m_catalogue->setToolTip(tr("Fills the data of the device from the catalogue "
							   "file chosen in the Electrical circuits panel"));
	connect(m_catalogue, &QPushButton::clicked, this, [this] {
		DevicePickerDialog picker(DeviceCatalogues::current(), this);
		picker.select(m_texts.value(QStringLiteral("manufacturer"))->text(),
					  m_texts.value(QStringLiteral("partNumber"))->text());
		if (picker.exec() == QDialog::Accepted)
			if (const auto entry = picker.entry())
				applyDevice(*entry);
	});
	protection_form->addRow(QString(), m_catalogue);
	addText(protection_form, QStringLiteral("manufacturer"), tr("Manufacturer"));
	addText(protection_form, QStringLiteral("partNumber"), tr("Part number"));
	const ProtectionType types[] = {ProtectionType::CircuitBreaker,
									ProtectionType::ResidualCurrent, ProtectionType::Rcbo,
									ProtectionType::Fuse, ProtectionType::Switch};
	QList<QPair<QString, int>> type_choices;
	for (ProtectionType t : types)
		type_choices << qMakePair(ElectricalTexts::protection(t), int(t));
	addChoice(protection_form, QStringLiteral("protectionType"), tr("Type"), type_choices);
	addText(protection_form, QStringLiteral("ratedCurrent"), tr("Rated current"), tr("A"));
	addText(protection_form, QStringLiteral("curve"), tr("Curve"))
		->setPlaceholderText(tr("e.g. C, gG"));
	addText(protection_form, QStringLiteral("breakingCapacity"), tr("Breaking capacity"),
			tr("kA"));
	addText(protection_form, QStringLiteral("sensitivity"), tr("Sensitivity"), tr("mA"));
	addText(protection_form, QStringLiteral("poles"), tr("Poles"));
	const rebt::ProtectionStandard standards[] = {
		rebt::ProtectionStandard::EN60898, rebt::ProtectionStandard::EN61009,
		rebt::ProtectionStandard::EN60947_2, rebt::ProtectionStandard::FuseGG};
	QList<QPair<QString, int>> standard_choices;
	for (rebt::ProtectionStandard st : standards)
		standard_choices << qMakePair(ElectricalTexts::standard(st), int(st));
	addChoice(protection_form, QStringLiteral("protectionStandard"), tr("Standard"),
			  standard_choices);
	layout->addWidget(m_protection_box);

		//What the checks say of that circuit, with the data as typed
	auto *results = new QGroupBox(tr("Checks of the circuit"));
	auto *results_layout = new QVBoxLayout(results);
	m_results = new QLabel;
	m_results->setObjectName(QStringLiteral("results"));
	m_results->setWordWrap(true);
	m_results->setTextInteractionFlags(Qt::TextSelectableByMouse);
	results_layout->addWidget(m_results);
	layout->addWidget(results);
	layout->addStretch();
}

namespace {

/// The data of a device the tab shows are the same in @p a and @p b
bool sameShownData(const ProtectionData &a, const ProtectionData &b)
{
	return a.manufacturer == b.manufacturer && a.partNumber == b.partNumber
		&& a.curve == b.curve && a.ratedCurrent == b.ratedCurrent
		&& a.breakingCapacity == b.breakingCapacity && a.sensitivity == b.sensitivity
		&& a.poles == b.poles && a.type == b.type && a.standard == b.standard;
}

} // namespace

void ElectricalElementWidget::applyDevice(const DeviceEntry &entry)
{
	ProtectionData p;
	applyDeviceEntry(p, entry);
	m_filling = true;
	fillProtection(p);
	m_pending = entry;
	m_filling = false;
	updateState();
	if (m_live_edit)
		apply();
}

void ElectricalElementWidget::fillProtection(const ProtectionData &p)
{
	m_pending.reset();
	m_texts.value(QStringLiteral("manufacturer"))->setText(p.manufacturer);
	m_texts.value(QStringLiteral("partNumber"))->setText(p.partNumber);
	m_texts.value(QStringLiteral("curve"))->setText(p.curve);
	m_texts.value(QStringLiteral("ratedCurrent"))->setText(formatNumber(p.ratedCurrent));
	m_texts.value(QStringLiteral("breakingCapacity"))->setText(formatNumber(p.breakingCapacity));
	m_texts.value(QStringLiteral("sensitivity"))->setText(formatNumber(p.sensitivity));
	m_texts.value(QStringLiteral("poles"))->setText(
		p.poles ? QString::number(*p.poles) : QString());
	auto select = [](QComboBox *combo, std::optional<int> value) {
		combo->setCurrentIndex(value ? qMax(0, combo->findData(*value)) : 0);
	};
	select(m_choices.value(QStringLiteral("protectionType")),
		   p.type ? std::optional<int>(int(*p.type)) : std::nullopt);
	select(m_choices.value(QStringLiteral("protectionStandard")),
		   p.standard ? std::optional<int>(int(*p.standard)) : std::nullopt);
}

void ElectricalElementWidget::updateUi()
{
	QETProject *p = project();
	m_filling = true;
	m_board->clear();
	m_circuit->clear();
	m_board->addItem(tr("None"), QUuid());
	m_circuit->addItem(tr("None"), QUuid());
	if (!p || !m_element) {
		setEnabled(false);
		m_filling = false;
		return;
	}
	const ElectricalModel &model = p->electricalModel();
	const QUuid uuid = m_element->uuid();
	setEnabled(true);
	m_read_only->setVisible(model.isReadOnly());

	QUuid current_board, current_circuit;
	for (const Board &b : model.boards()) {
		m_board->addItem(boardLabel(b), b.id);
		if (b.element == uuid) current_board = b.id;
	}
	for (const Circuit &c : model.circuits()) {
		m_circuit->addItem(circuitLabel(model, c), c.id);
		if (c.protection.element == uuid) current_circuit = c.id;
	}
	m_board->setCurrentIndex(qMax(0, m_board->findData(current_board)));
	m_circuit->setCurrentIndex(qMax(0, m_circuit->findData(current_circuit)));
	ProtectionData protection;
	if (const auto c = model.circuit(current_circuit))
		protection = c->protection;
	fillProtection(protection);
	m_filling = false;
	updateState();
}

bool ElectricalElementWidget::isValid() const
{
	for (const char *name : NUMBERS) {
		bool ok = false;
		parseNumber(m_texts.value(QLatin1String(name))->text(), &ok);
		if (!ok) return false;
	}
	return true;
}

ElectricalModel ElectricalElementWidget::editedModel() const
{
	QETProject *p = project();
	if (!p || !m_element)
		return ElectricalModel();
	ElectricalModel model = p->electricalModel();
	if (model.isReadOnly() || !isValid())
		return model;
	const QUuid uuid = m_element->uuid();

		//One board and one protected circuit at most per symbol, and one
		//symbol per board and per protection
	const QUuid board_id = m_board->currentData().toUuid();
	for (Board b : model.boards()) {
		const bool chosen = b.id == board_id;
		if (chosen && b.element != uuid) {
			b.element = uuid;
			model.setBoard(b);
		} else if (!chosen && b.element == uuid) {
			b.element = QUuid();
			model.setBoard(b);
		}
	}
	const QUuid circuit_id = m_circuit->currentData().toUuid();
	for (Circuit c : model.circuits()) {
		if (c.id != circuit_id && c.protection.element == uuid) {
			c.protection.element = QUuid();
			model.setCircuit(c);
		}
	}
	if (auto chosen = model.circuit(circuit_id)) {
		Circuit c = *chosen;
		ProtectionData &pd = c.protection;
		pd.element = uuid;
		pd.manufacturer = m_texts.value(QStringLiteral("manufacturer"))->text().trimmed();
		pd.partNumber = m_texts.value(QStringLiteral("partNumber"))->text().trimmed();
		pd.curve = m_texts.value(QStringLiteral("curve"))->text().trimmed();
		bool ok = false;
		pd.ratedCurrent = parseNumber(m_texts.value(QStringLiteral("ratedCurrent"))->text(), &ok);
		pd.breakingCapacity = parseNumber(
			m_texts.value(QStringLiteral("breakingCapacity"))->text(), &ok);
		pd.sensitivity = parseNumber(m_texts.value(QStringLiteral("sensitivity"))->text(), &ok);
		const auto poles = parseNumber(m_texts.value(QStringLiteral("poles"))->text(), &ok);
		pd.poles = poles ? std::optional<int>(qRound(*poles)) : std::nullopt;
		const int type = m_choices.value(QStringLiteral("protectionType"))->currentData().toInt();
		pd.type = type < 0 ? std::nullopt : std::optional<ProtectionType>(ProtectionType(type));
		const int standard =
			m_choices.value(QStringLiteral("protectionStandard"))->currentData().toInt();
		pd.standard = standard < 0 ? std::nullopt
					  : std::optional<rebt::ProtectionStandard>(rebt::ProtectionStandard(standard));
			//The data not shown of the device chosen, while the fields
			//shown are still those of the catalogue
		if (m_pending) {
			ProtectionData from_catalogue = pd;
			applyDeviceEntry(from_catalogue, *m_pending);
			if (sameShownData(from_catalogue, pd))
				pd = from_catalogue;
		}
		model.setCircuit(c);
	}
	return model;
}

void ElectricalElementWidget::updateState()
{
	if (m_filling)
		return;
	QETProject *p = project();
	if (!p || !m_element)
		return;
	const ElectricalModel &current = p->electricalModel();
	const bool read_only = current.isReadOnly();
	m_board->setEnabled(!read_only);
	m_circuit->setEnabled(!read_only);

		//The circuit chosen: its device data, as they are or as typed
	const QUuid circuit_id = m_circuit->currentData().toUuid();
	m_protection_box->setEnabled(!read_only && !circuit_id.isNull());
	for (const char *name : NUMBERS) {
		QLineEdit *edit = m_texts.value(QLatin1String(name));
		bool ok = false;
		parseNumber(edit->text(), &ok);
		edit->setStyleSheet(ok ? QString()
							   : QStringLiteral("QLineEdit { background: #ffd6d6; }"));
	}

		//What the symbol is, in the model as it would be
	const ElectricalModel model = editedModel();
	const QUuid uuid = m_element->uuid();
	QStringList roles;
	for (const Board &b : model.boards())
		if (b.element == uuid)
			roles << tr("Board %1").arg(boardLabel(b));
	for (const Circuit &c : model.circuits()) {
		if (c.protection.element == uuid)
			roles << tr("Protection of %1").arg(circuitLabel(model, c));
		if (c.origin == uuid)
			roles << tr("Origin of %1").arg(circuitLabel(model, c));
		if (c.destination == uuid)
			roles << tr("Destination of %1").arg(circuitLabel(model, c));
	}
	m_roles->setText(roles.isEmpty()
		? tr("Not in the electrical model of the project.")
		: roles.join(QLatin1Char('\n')));

	QStringList results;
	if (const auto c = model.circuit(circuit_id)) {
		const CircuitEvaluation e = evaluate(*c, model, AmpacityTables::current().library);
		results << tr("Voltage drop: %1").arg(ElectricalTexts::outcome(e));
		results << tr("IB ≤ IZ: %1").arg(ElectricalTexts::ampacityOutcome(e));
		results << tr("Protection ↔ cable: %1").arg(ElectricalTexts::coordinationOutcome(e));
		results << tr("Short circuit: %1").arg(ElectricalTexts::shortCircuitOutcome(e));
		if (c->dwellingCircuit)
			results << tr("ITC-BT-25, %1: %2")
						   .arg(ElectricalTexts::dwellingCode(*c->dwellingCircuit),
								ElectricalTexts::dwellingOutcome(e));
		results << QString();
		results << ElectricalTexts::coordinationDetails(e) << QString();
		results << ElectricalTexts::shortCircuitDetails(e);
	} else {
		results << tr("Choose the circuit this symbol protects to see its checks.");
	}
	m_results->setText(results.join(QLatin1Char('\n')));
}

QUndoCommand *ElectricalElementWidget::associatedUndo() const
{
	QETProject *p = project();
		//editedModel() already leaves the model as it is with a field
		//that is not a number; said here too, plainly
	if (!p || p->electricalModel().isReadOnly() || !isValid())
		return nullptr;
	const ElectricalModel model = editedModel();
	if (model == p->electricalModel())
		return nullptr;
	return new ChangeElectricalModelCommand(p, model, tr("Electrical data of a symbol"));
}

void ElectricalElementWidget::apply()
{
	if (QUndoCommand *undo = associatedUndo())
		project()->undoStack()->push(undo);
}

bool ElectricalElementWidget::setLiveEdit(bool live_edit)
{
	if (m_live_edit == live_edit)
		return true;
	m_live_edit = live_edit;
	if (live_edit)
		enableLiveEdit();
	else
		disableLiveEdit();
	return true;
}

void ElectricalElementWidget::enableLiveEdit()
{
	m_live << connect(m_board, &QComboBox::activated, this, &ElectricalElementWidget::apply);
	m_live << connect(m_circuit, &QComboBox::activated, this, &ElectricalElementWidget::apply);
	for (QComboBox *combo : std::as_const(m_choices))
		m_live << connect(combo, &QComboBox::activated, this, &ElectricalElementWidget::apply);
	for (QLineEdit *edit : std::as_const(m_texts))
		m_live << connect(edit, &QLineEdit::editingFinished, this,
						  &ElectricalElementWidget::apply);
}

void ElectricalElementWidget::disableLiveEdit()
{
	for (const QMetaObject::Connection &c : std::as_const(m_live))
		disconnect(c);
	m_live.clear();
}
