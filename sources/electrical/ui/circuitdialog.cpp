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
#include "circuitdialog.h"

#include "electrical/ampacityfile.h"
#include "electrical/electricalevaluation.h"
#include "devicepickerdialog.h"
#include "electricaltexts.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include <cmath>

using namespace electrical;

namespace {

/// Combo box data of "not known"
const int UNKNOWN = -1;

template <typename E>
int indexOf(const std::optional<E> &value)
{
	return value ? static_cast<int>(*value) : UNKNOWN;
}

template <typename E>
std::optional<E> valueOf(int data)
{
	if (data == UNKNOWN)
		return std::nullopt;
	return static_cast<E>(data);
}

} // namespace

CircuitDialog::CircuitDialog(const Circuit &circuit,
							 const QList<Board> &boards,
							 QWidget *parent) :
	QDialog(parent),
	m_circuit(circuit),
	m_tables(AmpacityTables::current().library)
{
	setWindowTitle(tr("Circuit"));
	setObjectName(QStringLiteral("circuit_dialog"));

	auto *content = new QWidget;
	auto *columns = new QHBoxLayout(content);
	auto *left = new QVBoxLayout;
	auto *right = new QVBoxLayout;
	columns->addLayout(left);
	columns->addLayout(right);

		//General
	auto *general_box = new QGroupBox(tr("Circuit"));
	auto *general = new QFormLayout(general_box);
	m_name = new QLineEdit(circuit.name);
	m_name->setObjectName(QStringLiteral("name"));
	general->addRow(tr("Name"), m_name);

	QList<QPair<QString, int>> board_choices;
	int current_board = UNKNOWN;
	for (int i = 0; i < boards.size(); ++i) {
		m_board_ids << boards.at(i).id;
		board_choices << qMakePair(boards.at(i).name.isEmpty()
									   ? tr("Board %1").arg(i + 1)
									   : boards.at(i).name, i);
		if (boards.at(i).id == circuit.board)
			current_board = i;
	}
	addChoice(general, QStringLiteral("board"), tr("Board"), board_choices,
			  current_board);

	addChoice(general, QStringLiteral("system"), tr("System"),
			  {{ElectricalTexts::system(rebt::System::SinglePhase),
				int(rebt::System::SinglePhase)},
			   {ElectricalTexts::system(rebt::System::ThreePhase),
				int(rebt::System::ThreePhase)}},
			  indexOf(circuit.system));
	addNumber(general, QStringLiteral("voltage"), tr("Voltage"), tr("V"),
			  circuit.voltage)->setToolTip(
		tr("Phase to neutral in single phase, line to line in three phase"));
	addChoice(general, QStringLiteral("use"), tr("Use"),
			  {{ElectricalTexts::use(CircuitUse::DwellingInterior),
				int(CircuitUse::DwellingInterior)},
			   {ElectricalTexts::use(CircuitUse::Lighting), int(CircuitUse::Lighting)},
			   {ElectricalTexts::use(CircuitUse::OtherUses), int(CircuitUse::OtherUses)},
			   {ElectricalTexts::use(CircuitUse::LgaTotallyCentralized),
				int(CircuitUse::LgaTotallyCentralized)},
			   {ElectricalTexts::use(CircuitUse::LgaPartlyCentralized),
				int(CircuitUse::LgaPartlyCentralized)},
			   {ElectricalTexts::use(CircuitUse::DiMetersInSeveralPlaces),
				int(CircuitUse::DiMetersInSeveralPlaces)},
			   {ElectricalTexts::use(CircuitUse::DiMetersTotallyGrouped),
				int(CircuitUse::DiMetersTotallyGrouped)},
			   {ElectricalTexts::use(CircuitUse::DiSingleUser),
				int(CircuitUse::DiSingleUser)}},
			  indexOf(circuit.use));
	{
		const rebt::DwellingCircuit kinds[] = {
			rebt::DwellingCircuit::C1, rebt::DwellingCircuit::C2,
			rebt::DwellingCircuit::C3, rebt::DwellingCircuit::C4,
			rebt::DwellingCircuit::C4Single, rebt::DwellingCircuit::C5,
			rebt::DwellingCircuit::C6, rebt::DwellingCircuit::C7,
			rebt::DwellingCircuit::C8, rebt::DwellingCircuit::C9,
			rebt::DwellingCircuit::C10, rebt::DwellingCircuit::C11,
			rebt::DwellingCircuit::C12C3, rebt::DwellingCircuit::C12C4,
			rebt::DwellingCircuit::C12C5, rebt::DwellingCircuit::C13};
		QList<QPair<QString, int>> choices;
		for (rebt::DwellingCircuit k : kinds)
			choices << qMakePair(ElectricalTexts::dwellingCircuit(k), int(k));
		addChoice(general, QStringLiteral("dwellingCircuit"),
				  tr("Dwelling circuit"), choices,
				  indexOf(circuit.dwellingCircuit))->setToolTip(
			tr("Circuit of table 1 of ITC-BT-25: its breaker, smallest section "
			   "and points are checked. Unknown: not a dwelling circuit"));
		std::optional<double> points;
		if (circuit.points)
			points = *circuit.points;
		addNumber(general, QStringLiteral("points"), tr("Points of use"), QString(),
				  points)->setToolTip(
			tr("Points of light or sockets of the circuit"));
	}
	left->addWidget(general_box);

		//Load
	auto *load_box = new QGroupBox(tr("Load"));
	auto *load = new QFormLayout(load_box);
	addNumber(load, QStringLiteral("power"), tr("Power"), tr("W"), circuit.load.power);
	addNumber(load, QStringLiteral("powerFactor"), tr("Power factor (cos φ)"),
			  QString(), circuit.load.powerFactor);
	addNumber(load, QStringLiteral("efficiency"), tr("Efficiency"), QString(),
			  circuit.load.efficiency)->setToolTip(
		tr("Motors and the like, from 0 to 1. Empty: the power is taken as absorbed"));
	addNumber(load, QStringLiteral("designCurrent"), tr("Design current"), tr("A"),
			  circuit.load.designCurrent)->setToolTip(
		tr("When known: used instead of the current worked out from the power"));
	left->addWidget(load_box);

		//Protection
	auto *prot_box = new QGroupBox(tr("Protection"));
	auto *prot = new QFormLayout(prot_box);
	const ProtectionType types[] = {ProtectionType::CircuitBreaker,
									ProtectionType::ResidualCurrent,
									ProtectionType::Rcbo, ProtectionType::Fuse,
									ProtectionType::Switch};
	QList<QPair<QString, int>> type_choices;
	for (ProtectionType t : types)
		type_choices << qMakePair(ElectricalTexts::protection(t), int(t));
	m_catalogue = new QPushButton(tr("From the catalogue..."));
	m_catalogue->setObjectName(QStringLiteral("catalogue"));
	m_catalogue->setToolTip(tr("Fills the data of the device from the catalogue "
							   "file chosen in the Electrical circuits panel"));
	connect(m_catalogue, &QPushButton::clicked, this, [this] {
		const DeviceCatalogue &catalogue = DeviceCatalogues::current();
		DevicePickerDialog picker(catalogue, this);
		picker.select(m_manufacturer->text(), m_part_number->text());
		if (picker.exec() == QDialog::Accepted)
			if (const auto entry = picker.entry())
				applyDevice(*entry);
	});
	prot->addRow(QString(), m_catalogue);
	m_manufacturer = new QLineEdit(circuit.protection.manufacturer);
	m_manufacturer->setObjectName(QStringLiteral("manufacturer"));
	prot->addRow(tr("Manufacturer"), m_manufacturer);
	m_part_number = new QLineEdit(circuit.protection.partNumber);
	m_part_number->setObjectName(QStringLiteral("partNumber"));
	prot->addRow(tr("Part number"), m_part_number);
	addChoice(prot, QStringLiteral("protectionType"), tr("Type"), type_choices,
			  indexOf(circuit.protection.type));
	addNumber(prot, QStringLiteral("ratedCurrent"), tr("Rated current"), tr("A"),
			  circuit.protection.ratedCurrent);
	m_curve = new QLineEdit(circuit.protection.curve);
	m_curve->setObjectName(QStringLiteral("curve"));
	m_curve->setPlaceholderText(tr("e.g. C, gG"));
	prot->addRow(tr("Curve"), m_curve);
	m_residual = new QComboBox;
	m_residual->setObjectName(QStringLiteral("residualDevice"));
	m_residual->setEditable(true);
	m_residual->addItem(QString());
	{
		QStringList names;
		for (const Board &b : boards)
			for (const ResidualDeviceData &d : b.residualDevices)
				if (!d.name.trimmed().isEmpty() && !names.contains(d.name.trimmed()))
					names << d.name.trimmed();
		m_residual->addItems(names);
	}
	m_residual->setCurrentText(circuit.protection.residualDevice);
	m_residual->setToolTip(tr("The residual current device of the board that "
							  "protects the circuit, by its name. Empty with an "
							  "RCBO: it protects the circuit by itself"));
	prot->addRow(tr("Residual current device"), m_residual);
	addNumber(prot, QStringLiteral("breakingCapacity"), tr("Breaking capacity"),
			  tr("kA"), circuit.protection.breakingCapacity);
	addNumber(prot, QStringLiteral("sensitivity"), tr("Sensitivity"), tr("mA"),
			  circuit.protection.sensitivity);
	std::optional<double> poles;
	if (circuit.protection.poles)
		poles = *circuit.protection.poles;
	addNumber(prot, QStringLiteral("poles"), tr("Poles"), QString(), poles);
	const rebt::ProtectionStandard standards[] = {
		rebt::ProtectionStandard::EN60898, rebt::ProtectionStandard::EN61009,
		rebt::ProtectionStandard::EN60947_2, rebt::ProtectionStandard::FuseGG};
	QList<QPair<QString, int>> standard_choices;
	for (rebt::ProtectionStandard st : standards)
		standard_choices << qMakePair(ElectricalTexts::standard(st), int(st));
	addChoice(prot, QStringLiteral("protectionStandard"), tr("Standard"),
			  standard_choices, indexOf(circuit.protection.standard))->setToolTip(
		tr("Sets the conventional tripping current I2 of the device"));
	addFactor(prot, QStringLiteral("conventionalCurrent"), tr("I2 (A)"),
			  circuit.protection.conventionalCurrent,
			  circuit.protection.conventionalCurrentReason)->setToolTip(
		tr("Empty: from the standard. Given (A), with its source: for a gG fuse "
		   "under 16 A or any other device"));
	addFactor(prot, QStringLiteral("tripCurrent"), tr("Instantaneous trip (A)"),
			  circuit.protection.tripCurrent,
			  circuit.protection.tripCurrentReason)->setToolTip(
		tr("Empty: from the curve B, C or D of a UNE-EN 60898 or 61009 device. "
		   "Given (A), with its source: industrial breakers (Im) and fuses"));
	addFactor(prot, QStringLiteral("letThrough"), tr("I²t (A²s)"),
			  circuit.protection.letThrough,
			  circuit.protection.letThroughReason)->setToolTip(
		tr("Let-through energy of the manufacturer at the maximum Icc of the "
		   "board, with its source (catalogue curve)"));
	addNumber(prot, QStringLiteral("letThroughAt"), tr("I²t given at"), tr("kA"),
			  circuit.protection.letThroughAt)->setToolTip(
		tr("Prospective current of that I²t. If it is below the maximum Icc "
		   "of the board, the I²t is not used. Empty: the I²t is taken as "
		   "given at the maximum Icc"));
	left->addWidget(prot_box);
	left->addStretch();

		//Conductor
	auto *cond_box = new QGroupBox(tr("Conductor"));
	auto *cond = new QFormLayout(cond_box);
	addChoice(cond, QStringLiteral("material"), tr("Material"),
			  {{ElectricalTexts::material(rebt::Material::Copper),
				int(rebt::Material::Copper)},
			   {ElectricalTexts::material(rebt::Material::Aluminium),
				int(rebt::Material::Aluminium)}},
			  indexOf(circuit.conductor.material));
	addChoice(cond, QStringLiteral("insulation"), tr("Insulation"),
			  {{ElectricalTexts::insulation(Insulation::PVC), int(Insulation::PVC)},
			   {ElectricalTexts::insulation(Insulation::XLPE), int(Insulation::XLPE)},
			   {ElectricalTexts::insulation(Insulation::EPR), int(Insulation::EPR)}},
			  indexOf(circuit.conductor.insulation));
	m_method = new QLineEdit(circuit.conductor.installationMethod);
	m_method->setObjectName(QStringLiteral("installationMethod"));
	m_method->setPlaceholderText(tr("e.g. B1"));
	cond->addRow(tr("Installation method"), m_method);
	addNumber(cond, QStringLiteral("section"), tr("Phase section"), tr("mm²"),
			  circuit.conductor.section);
	addNumber(cond, QStringLiteral("neutralSection"), tr("Neutral section"),
			  tr("mm²"), circuit.conductor.neutralSection);

	addNumber(cond, QStringLiteral("peSection"), tr("Protective conductor section"),
			  tr("mm²"), circuit.conductor.peSection)->setToolTip(
		tr("The PE (the PEN in TN-C): needed for the indirect contacts in TN "
		   "and IT, never assumed"));
	addNumber(cond, QStringLiteral("length"), tr("Length"), tr("m"),
			  circuit.conductor.length);
	addNumber(cond, QStringLiteral("temperature"), tr("Conductor temperature"),
			  tr("°C"), circuit.conductor.temperature)->setToolTip(
		tr("Used with the material to work out the resistivity"));
	addNumber(cond, QStringLiteral("conductivity"), tr("Conductivity"),
			  tr("m/(Ω·mm²)"), circuit.conductor.conductivity)->setToolTip(
		tr("When given, used instead of the material and the temperature"));
	addNumber(cond, QStringLiteral("conduit"), tr("Conduit"), tr("mm"),
			  circuit.conductor.conduit)->setToolTip(
		tr("Outer diameter of the conduit (ITC-BT-19, ITC-BT-21)"));
	addNumber(cond, QStringLiteral("reactance"), tr("Reactance"), tr("Ω/km"),
			  circuit.conductor.reactance)->setToolTip(
		tr("Empty: the reactance is neglected"));
	right->addWidget(cond_box);

		//Admissible current (ITC-BT-19 2.2.3)
	auto *iz_box = new QGroupBox(tr("Admissible current"));
	auto *iz = new QFormLayout(iz_box);
	std::optional<double> loaded;
	if (circuit.conductor.loadedConductors)
		loaded = *circuit.conductor.loadedConductors;
	addNumber(iz, QStringLiteral("loadedConductors"), tr("Loaded conductors"),
			  QString(), loaded)->setToolTip(
		tr("2 or 3. Empty: 2 in single phase, 3 in three phase"));
	addNumber(iz, QStringLiteral("ambientTemperature"), tr("Ambient temperature"),
			  tr("°C"), circuit.conductor.ambientTemperature)->setToolTip(
		tr("Of the air or the ground around the cable: the temperature factor "
		   "is worked out from it"));
	addFactor(iz, QStringLiteral("temperatureFactor"), tr("Temperature factor"),
			  circuit.conductor.temperatureFactor,
			  circuit.conductor.temperatureFactorReason)->setToolTip(
		tr("Empty: computed from the ambient temperature. Given: it needs its reason"));
	addFactor(iz, QStringLiteral("groupingFactor"), tr("Grouping factor"),
			  circuit.conductor.groupingFactor,
			  circuit.conductor.groupingFactorReason)->setToolTip(
		tr("Always needed, with its reason: 1 for a circuit on its own"));
	addFactor(iz, QStringLiteral("otherFactor"), tr("Other factor"),
			  circuit.conductor.otherFactor, circuit.conductor.otherFactorReason)
		->setToolTip(tr("Any other correction that applies, with its reason"));
	m_use_suggestion = new QPushButton(tr("Use the suggested section"));
	m_use_suggestion->setObjectName(QStringLiteral("use_suggestion"));
	connect(m_use_suggestion, &QPushButton::clicked, this, [this] {
		if (m_suggested)
			m_numbers.value(QStringLiteral("section"))->setText(formatNumber(m_suggested));
	});
	iz->addRow(m_use_suggestion);
	right->addWidget(iz_box);

	auto *notes_box = new QGroupBox(tr("Notes, assumptions, pending data"));
	auto *notes_layout = new QVBoxLayout(notes_box);
	m_notes = new QPlainTextEdit(circuit.notes);
	m_notes->setObjectName(QStringLiteral("notes"));
	m_notes->setMaximumHeight(80);
	notes_layout->addWidget(m_notes);
	right->addWidget(notes_box);

	auto *result_box = new QGroupBox(tr("Calculation"));
	auto *result_layout = new QVBoxLayout(result_box);
	m_result = new QLabel;
	m_result->setObjectName(QStringLiteral("result"));
	m_result->setWordWrap(true);
	m_result->setTextInteractionFlags(Qt::TextSelectableByMouse);
	result_layout->addWidget(m_result);
	right->addWidget(result_box);
	right->addStretch();

	auto *scroll = new QScrollArea;
	scroll->setWidget(content);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto *layout = new QVBoxLayout(this);
	auto *hint = new QLabel(tr("Leave a field empty when the value is not "
							   "known: it is never filled in for you."));
	hint->setWordWrap(true);
	layout->addWidget(hint);
	layout->addWidget(scroll);
	layout->addWidget(m_buttons);

	updateState();
}

QLineEdit *CircuitDialog::addNumber(QFormLayout *form, const QString &name,
									const QString &label, const QString &unit,
									const std::optional<double> &value)
{
	auto *edit = new QLineEdit(formatNumber(value));
	edit->setObjectName(name);
	edit->setPlaceholderText(tr("unknown"));
	m_numbers.insert(name, edit);
	connect(edit, &QLineEdit::textChanged, this, &CircuitDialog::updateState);
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

QLineEdit *CircuitDialog::addFactor(QFormLayout *form, const QString &name,
									const QString &label,
									const std::optional<double> &value,
									const QString &reason)
{
	auto *edit = new QLineEdit(formatNumber(value));
	edit->setObjectName(name);
	edit->setPlaceholderText(tr("unknown"));
	edit->setMaximumWidth(80);
	m_numbers.insert(name, edit);
	connect(edit, &QLineEdit::textChanged, this, &CircuitDialog::updateState);
	auto *why = new QLineEdit(reason);
	why->setObjectName(name + QLatin1String("Reason"));
	why->setPlaceholderText(tr("justification: table, number of circuits..."));
	m_reasons.insert(name, why);
	connect(why, &QLineEdit::textChanged, this, &CircuitDialog::updateState);
	auto *row = new QHBoxLayout;
	row->addWidget(edit);
	row->addWidget(why, 1);
	form->addRow(label, row);
	return edit;
}

void CircuitDialog::setTables(const rebt::AmpacityLibrary &tables)
{
	m_tables = tables;
	updateState();
}

void CircuitDialog::setModel(const ElectricalModel &model)
{
	m_model = model;
	updateState();
}

QComboBox *CircuitDialog::addChoice(QFormLayout *form, const QString &name,
									const QString &label,
									const QList<QPair<QString, int>> &choices,
									int current)
{
	auto *combo = new QComboBox;
	combo->setObjectName(name);
	combo->addItem(tr("Unknown"), UNKNOWN);
	for (const auto &c : choices)
		combo->addItem(c.first, c.second);
	combo->setCurrentIndex(qMax(0, combo->findData(current)));
	m_choices.insert(name, combo);
	connect(combo, &QComboBox::currentIndexChanged, this, &CircuitDialog::updateState);
	form->addRow(label, combo);
	return combo;
}

QLineEdit *CircuitDialog::numberField(const QString &name) const
{
	return m_numbers.value(name);
}

QComboBox *CircuitDialog::choiceField(const QString &name) const
{
	return m_choices.value(name);
}

std::optional<double> CircuitDialog::number(const QString &name) const
{
	bool ok = false;
	const auto value = parseNumber(m_numbers.value(name)->text(), &ok);
	return ok ? value : std::nullopt;
}

int CircuitDialog::choice(const QString &name) const
{
	return m_choices.value(name)->currentData().toInt();
}

bool CircuitDialog::isValid() const
{
	for (auto it = m_numbers.cbegin(); it != m_numbers.cend(); ++it) {
		bool ok = false;
		const auto value = parseNumber(it.value()->text(), &ok);
		if (!ok)
			return false;
		if (it.key() == QLatin1String("poles") && value
			&& (*value < 1 || *value != static_cast<int>(*value)))
			return false;
		if (it.key() == QLatin1String("loadedConductors") && value
			&& *value != 2.0 && *value != 3.0)
			return false;
	}
	return true;
}

Circuit CircuitDialog::circuit() const
{
	Circuit c = m_circuit;
	c.name = m_name->text().trimmed();
	const int board = choice(QStringLiteral("board"));
	c.board = board == UNKNOWN ? QUuid() : m_board_ids.value(board);
	c.system = valueOf<rebt::System>(choice(QStringLiteral("system")));
	c.voltage = number(QStringLiteral("voltage"));
	c.use = valueOf<CircuitUse>(choice(QStringLiteral("use")));
	c.dwellingCircuit = valueOf<rebt::DwellingCircuit>(choice(QStringLiteral("dwellingCircuit")));
	{
		const auto points = number(QStringLiteral("points"));
		c.points = points ? std::optional<int>(static_cast<int>(std::lround(*points)))
						  : std::nullopt;
	}

	c.load.power = number(QStringLiteral("power"));
	c.load.powerFactor = number(QStringLiteral("powerFactor"));
	c.load.efficiency = number(QStringLiteral("efficiency"));
	c.load.designCurrent = number(QStringLiteral("designCurrent"));

	c.conductor.material = valueOf<rebt::Material>(choice(QStringLiteral("material")));
	c.conductor.insulation = valueOf<Insulation>(choice(QStringLiteral("insulation")));
	c.conductor.installationMethod = m_method->text().trimmed();
	c.conductor.section = number(QStringLiteral("section"));
	c.conductor.neutralSection = number(QStringLiteral("neutralSection"));
	c.conductor.peSection = number(QStringLiteral("peSection"));
	c.conductor.length = number(QStringLiteral("length"));
	c.conductor.temperature = number(QStringLiteral("temperature"));
	c.conductor.conductivity = number(QStringLiteral("conductivity"));
	c.conductor.reactance = number(QStringLiteral("reactance"));
	c.conductor.conduit = number(QStringLiteral("conduit"));
	const auto loaded = number(QStringLiteral("loadedConductors"));
	c.conductor.loadedConductors = loaded
		? std::optional<int>(static_cast<int>(*loaded)) : std::nullopt;
	c.conductor.ambientTemperature = number(QStringLiteral("ambientTemperature"));
	c.conductor.temperatureFactor = number(QStringLiteral("temperatureFactor"));
	c.conductor.temperatureFactorReason =
		m_reasons.value(QStringLiteral("temperatureFactor"))->text().trimmed();
	c.conductor.groupingFactor = number(QStringLiteral("groupingFactor"));
	c.conductor.groupingFactorReason =
		m_reasons.value(QStringLiteral("groupingFactor"))->text().trimmed();
	c.conductor.otherFactor = number(QStringLiteral("otherFactor"));
	c.conductor.otherFactorReason =
		m_reasons.value(QStringLiteral("otherFactor"))->text().trimmed();

	c.protection.type = valueOf<ProtectionType>(choice(QStringLiteral("protectionType")));
	c.protection.ratedCurrent = number(QStringLiteral("ratedCurrent"));
	c.protection.curve = m_curve->text().trimmed();
	c.protection.manufacturer = m_manufacturer->text().trimmed();
	c.protection.partNumber = m_part_number->text().trimmed();
	c.protection.residualDevice = m_residual->currentText().trimmed();
	c.protection.breakingCapacity = number(QStringLiteral("breakingCapacity"));
	c.protection.sensitivity = number(QStringLiteral("sensitivity"));
	const auto poles = number(QStringLiteral("poles"));
	c.protection.poles = poles ? std::optional<int>(static_cast<int>(*poles))
							   : std::nullopt;
	c.protection.standard = valueOf<rebt::ProtectionStandard>(
		choice(QStringLiteral("protectionStandard")));
	c.protection.conventionalCurrent = number(QStringLiteral("conventionalCurrent"));
	c.protection.conventionalCurrentReason =
		m_reasons.value(QStringLiteral("conventionalCurrent"))->text().trimmed();
	c.protection.tripCurrent = number(QStringLiteral("tripCurrent"));
	c.protection.tripCurrentReason =
		m_reasons.value(QStringLiteral("tripCurrent"))->text().trimmed();
	c.protection.letThrough = number(QStringLiteral("letThrough"));
	c.protection.letThroughAt = number(QStringLiteral("letThroughAt"));
	c.protection.letThroughReason =
		m_reasons.value(QStringLiteral("letThrough"))->text().trimmed();
	c.notes = m_notes->toPlainText().trimmed();
	return c;
}

void CircuitDialog::updateState()
{
	for (QLineEdit *edit : std::as_const(m_numbers)) {
		bool ok = false;
		parseNumber(edit->text(), &ok);
		edit->setStyleSheet(ok ? QString()
							   : QStringLiteral("QLineEdit { background: #ffd6d6; }"));
	}
	const bool valid = isValid();
	m_buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
	m_suggested.reset();
	m_use_suggestion->setEnabled(false);
	if (!valid) {
		m_result->setText(tr("Correct the fields in red: they are not numbers."));
		return;
	}

	const Circuit edited = circuit();
	CircuitEvaluation e;
	if (m_model) {
			//The model as it would be with this circuit, for the drop from the origin
		ElectricalModel model = *m_model;
		model.setCircuit(edited);
		e = evaluate(edited, model, m_tables);
	} else {
		e = evaluate(edited, m_tables);
	}
	QStringList lines;
	const auto current = e.designCurrent();
	lines << tr("Design current: %1")
			 .arg(current ? tr("%1 A").arg(formatNumber(current, 2)) : tr("not computable"));
	if (e.drop.status == rebt::Status::Ok)
		lines << tr("Voltage drop: %1 V (%2 %)")
				 .arg(formatNumber(e.drop.drop, 2), formatNumber(e.drop.percent, 2));
	if (e.chainEvaluated && e.cumulativePercent && !e.feeders.isEmpty())
		lines << tr("Voltage drop from the origin: %1 %")
				 .arg(formatNumber(e.cumulativePercent, 2));
	else
		lines << tr("Voltage drop: not computable");
	lines << tr("Check: %1").arg(ElectricalTexts::outcome(e));
	lines << ElectricalTexts::details(e);
	lines << QString();
	lines << tr("IB ≤ IZ: %1").arg(ElectricalTexts::ampacityOutcome(e));
	lines << ElectricalTexts::ampacityDetails(e);
	lines << ElectricalTexts::suggestion(e);
	lines << QString();
	lines << tr("Protection and cable: %1").arg(ElectricalTexts::coordinationOutcome(e));
	lines << ElectricalTexts::coordinationDetails(e);
	m_result->setText(lines.join(QLatin1Char('\n')));

	m_suggested = e.suggestion.section;
	m_use_suggestion->setEnabled(m_suggested.has_value()
								 && m_suggested != number(QStringLiteral("section")));
}

void CircuitDialog::applyDevice(const DeviceEntry &entry)
{
	ProtectionData p = circuit().protection;
	applyDeviceEntry(p, entry);
	m_manufacturer->setText(p.manufacturer);
	m_part_number->setText(p.partNumber);
	m_curve->setText(p.curve);
	auto setChoice = [this](const QString &name, int data) {
		QComboBox *combo = m_choices.value(name);
		combo->setCurrentIndex(qMax(0, combo->findData(data)));
	};
	setChoice(QStringLiteral("protectionType"), indexOf(p.type));
	setChoice(QStringLiteral("protectionStandard"), indexOf(p.standard));
	std::optional<double> poles;
	if (p.poles) poles = *p.poles;
	const QList<QPair<QString, std::optional<double>>> numbers = {
		{QStringLiteral("ratedCurrent"), p.ratedCurrent},
		{QStringLiteral("breakingCapacity"), p.breakingCapacity},
		{QStringLiteral("sensitivity"), p.sensitivity},
		{QStringLiteral("poles"), poles},
		{QStringLiteral("conventionalCurrent"), p.conventionalCurrent},
		{QStringLiteral("tripCurrent"), p.tripCurrent},
		{QStringLiteral("letThrough"), p.letThrough},
		{QStringLiteral("letThroughAt"), p.letThroughAt}};
	for (const auto &n : numbers)
		m_numbers.value(n.first)->setText(formatNumber(n.second));
	m_reasons.value(QStringLiteral("conventionalCurrent"))->setText(p.conventionalCurrentReason);
	m_reasons.value(QStringLiteral("tripCurrent"))->setText(p.tripCurrentReason);
	m_reasons.value(QStringLiteral("letThrough"))->setText(p.letThroughReason);
	updateState();
}
