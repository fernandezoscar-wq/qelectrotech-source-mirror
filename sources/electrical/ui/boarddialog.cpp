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
#include "boarddialog.h"

#include "electrical/electricalevaluation.h"
#include "electricaltexts.h"
#include "roomsdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>

using namespace electrical;

BoardDialog::BoardDialog(const Board &board, const ElectricalModel &model,
						 QWidget *parent) :
	QDialog(parent),
	m_board(board)
{
	setWindowTitle(tr("Board"));
	setObjectName(QStringLiteral("board_dialog"));
	setMinimumWidth(900);

	m_name = new QLineEdit(board.name);
	m_name->setObjectName(QStringLiteral("name"));

	m_feeder = new QComboBox;
	m_feeder->setObjectName(QStringLiteral("feeder"));
	m_feeder->addItem(tr("None: origin of the installation"), QUuid());
		//Any circuit but its own: a board cannot feed itself
	for (const Board &b : model.boards()) {
		for (const Circuit &c : model.circuits()) {
			if (c.board != b.id || b.id == board.id)
				continue;
			const QString circuit_name = c.name.isEmpty()
				? tr("(unnamed circuit)") : c.name;
			m_feeder->addItem(QStringLiteral("%1 › %2").arg(
								  b.name.isEmpty() ? tr("(unnamed board)") : b.name,
								  circuit_name), c.id);
		}
	}
	m_feeder->setCurrentIndex(qMax(0, m_feeder->findData(board.feeder)));

	auto *form = new QFormLayout;
	form->addRow(tr("Name"), m_name);
	form->addRow(tr("Fed by"), m_feeder);
	auto *hint = new QLabel(tr("The voltage drop of the circuit that feeds the "
							   "board is added to that of its circuits, back to "
							   "the origin of the interior installation."));
	hint->setWordWrap(true);

		//Supply: only for the origin, whose network the user must state
	auto *source_box = new QGroupBox(tr("Supply of the installation"));
	m_source_box = source_box;
	m_source = new QComboBox;
	m_source->setObjectName(QStringLiteral("source"));
	m_source->addItem(tr("Not known"), -1);
	m_source->addItem(ElectricalTexts::source(rebt::SourceMethod::GuideFactor),
					  int(rebt::SourceMethod::GuideFactor));
	m_source->addItem(ElectricalTexts::source(rebt::SourceMethod::KnownCurrent),
					  int(rebt::SourceMethod::KnownCurrent));
	m_source->setCurrentIndex(board.source ? m_source->findData(int(*board.source)) : 0);
	m_source_max = new QLineEdit(formatNumber(board.sourceMaxCurrent));
	m_source_max->setObjectName(QStringLiteral("sourceMaxCurrent"));
	m_source_max->setPlaceholderText(tr("unknown"));
	m_source_min = new QLineEdit(formatNumber(board.sourceMinCurrent));
	m_source_min->setObjectName(QStringLiteral("sourceMinCurrent"));
	m_source_min->setPlaceholderText(tr("unknown"));
	m_source_reason = new QLineEdit(board.sourceReason);
	m_source_reason->setObjectName(QStringLiteral("sourceReason"));
	m_source_reason->setPlaceholderText(tr("source: letter of the distributor, study..."));
	auto *source_layout = new QVBoxLayout(source_box);
	auto *source_form = new QFormLayout;
	source_layout->addLayout(source_form);
	source_form->addRow(tr("Network"), m_source);
	source_form->addRow(tr("Maximum Icc (kA)"), m_source_max);
	source_form->addRow(tr("Minimum Icc (kA)"), m_source_min);
	source_form->addRow(tr("Source of the values"), m_source_reason);
	auto *source_hint = new QLabel(tr(
		"0.8 U / R (Guía-BT Anexo 3) only when the transformer is outside the "
		"building and this board is the CGP: the LGA and the DI must then be "
		"circuits of the model. Otherwise give the Icc at this board, phase to "
		"neutral and the greatest of any fault, with their source."));
	source_hint->setWordWrap(true);
	source_layout->addWidget(source_hint);
	m_own_transformer = new QCheckBox(tr("Industrial installation fed in high "
										 "voltage through its own transformer"));
	m_own_transformer->setObjectName(QStringLiteral("ownTransformer"));
	m_own_transformer->setChecked(board.ownTransformer);
	m_own_transformer->setToolTip(tr("ITC-BT-19 2.2.2: the interior installation "
									 "starts at the transformer output; voltage "
									 "drop at most 4.5 % for lighting and 6.5 % "
									 "for other uses"));
	source_layout->addWidget(m_own_transformer);
		//Earthing (ITC-BT-08, ITC-BT-24 4.1.2)
	auto *earth_box = new QGroupBox(tr("Earthing and indirect contacts"));
	m_earthing = new QComboBox;
	m_earthing->setObjectName(QStringLiteral("earthing"));
	m_earthing->addItem(tr("As the board that feeds it"), -1);
	m_earthing->addItem(ElectricalTexts::earthing(rebt::EarthingSystem::TT),
						int(rebt::EarthingSystem::TT));
	m_earthing->addItem(ElectricalTexts::earthing(rebt::EarthingSystem::TN),
						int(rebt::EarthingSystem::TN));
	m_earthing->addItem(ElectricalTexts::earthing(rebt::EarthingSystem::TNC),
						int(rebt::EarthingSystem::TNC));
	m_earthing->addItem(ElectricalTexts::earthing(rebt::EarthingSystem::IT),
						int(rebt::EarthingSystem::IT));
	m_earthing->setCurrentIndex(board.earthing ? m_earthing->findData(int(*board.earthing)) : 0);
	m_earth_resistance = new QLineEdit(formatNumber(board.earthResistance));
	m_earth_resistance->setObjectName(QStringLiteral("earthResistance"));
	m_earth_resistance->setPlaceholderText(tr("unknown"));
	m_earth_reason = new QLineEdit(board.earthResistanceReason);
	m_earth_reason->setObjectName(QStringLiteral("earthResistanceReason"));
	m_earth_reason->setPlaceholderText(tr("source: measured, estimated per ITC-BT-18..."));
	m_limit_voltage = new QLineEdit(formatNumber(board.limitVoltage));
	m_limit_voltage->setObjectName(QStringLiteral("limitVoltage"));
	m_limit_voltage->setPlaceholderText(tr("50, normal conditions"));
	m_limit_reason = new QLineEdit(board.limitVoltageReason);
	m_limit_reason->setObjectName(QStringLiteral("limitVoltageReason"));
	m_limit_reason->setPlaceholderText(tr("reason of a lower value"));
	m_first_fault = new QLineEdit(formatNumber(board.firstFaultCurrent));
	m_first_fault->setObjectName(QStringLiteral("firstFaultCurrent"));
	m_first_fault->setPlaceholderText(tr("unknown"));
	m_first_fault_reason = new QLineEdit(board.firstFaultReason);
	m_first_fault_reason->setObjectName(QStringLiteral("firstFaultReason"));
	m_first_fault_reason->setPlaceholderText(tr("source: measured, insulation monitor..."));
	auto yes_no = [](const QString &name, const QString &yes, const QString &no,
					 const std::optional<bool> &value) {
		auto *combo = new QComboBox;
		combo->setObjectName(name);
		combo->addItem(tr("Unknown"), -1);
		combo->addItem(yes, 1);
		combo->addItem(no, 0);
		combo->setCurrentIndex(value ? combo->findData(*value ? 1 : 0) : 0);
		return combo;
	};
	m_masses = yes_no(QStringLiteral("massesInterconnected"),
					  tr("Interconnected, one earthing"), tr("Earthed apart"),
					  board.massesInterconnected);
	m_neutral_distributed = yes_no(QStringLiteral("neutralDistributed"),
								   tr("Distributed"), tr("Not distributed"),
								   board.neutralDistributed);
	auto *earth_layout = new QVBoxLayout(earth_box);
	auto *earth_form = new QFormLayout;
	earth_layout->addLayout(earth_form);
	earth_form->addRow(tr("Earthing system"), m_earthing);
	earth_form->addRow(tr("RA (Ω)"), m_earth_resistance);
	earth_form->addRow(tr("Source of RA"), m_earth_reason);
	earth_form->addRow(tr("Limit voltage U (V)"), m_limit_voltage);
	earth_form->addRow(tr("Reason of U"), m_limit_reason);
	earth_form->addRow(tr("IT: first fault Id (A)"), m_first_fault);
	earth_form->addRow(tr("Source of Id"), m_first_fault_reason);
	earth_form->addRow(tr("IT: masses"), m_masses);
	earth_form->addRow(tr("IT: neutral"), m_neutral_distributed);
	auto *earth_hint = new QLabel(tr(
		"TT: RA × Ia ≤ U. TN: Zs × Ia ≤ U0, with the PE of every line. IT: "
		"RA × Id ≤ U, and the second fault as TT or with 2 Zs × Ia (ITC-BT-24 "
		"4.1). RA, the earth electrode plus the protective conductors of the "
		"masses, and Id, as measured or estimated: they are not worked out here."));
	earth_hint->setWordWrap(true);
	earth_layout->addWidget(earth_hint);

	m_compensate = new QCheckBox(tr("Compensate the voltage drop with the DI"));
	m_compensate->setObjectName(QStringLiteral("compensateWithIndividualLine"));
	m_compensate->setChecked(board.compensateWithIndividualLine);
	m_compensate->setToolTip(tr("ITC-BT-19 2.2.2: the drops of the interior "
								"installation and of the DI added, lower than the "
								"sum of both limits"));

		//The dwelling that starts here (ITC-BT-25)
	auto *dwelling_box = new QGroupBox(tr("Dwelling (ITC-BT-25)"));
	m_dwelling_box = dwelling_box;
	auto *dwelling_layout = new QVBoxLayout(dwelling_box);
	auto *dwelling_form = new QFormLayout;
	dwelling_layout->addLayout(dwelling_form);
	m_area = new QLineEdit(formatNumber(board.dwellingArea));
	m_area->setObjectName(QStringLiteral("dwellingArea"));
	m_area->setPlaceholderText(tr("unknown"));
	dwelling_form->addRow(tr("Useful area (m²)"), m_area);
	const QList<QPair<QString, std::optional<bool>>> planned{
		{QStringLiteral("heating"), board.plannedHeating},
		{QStringLiteral("airConditioning"), board.plannedAirConditioning},
		{QStringLiteral("automation"), board.plannedAutomation},
		{QStringLiteral("vehicleCharging"), board.plannedVehicleCharging},
		{QStringLiteral("dryer"), board.plannedDryer},
		{QStringLiteral("singleFamily"), board.singleFamily}};
	const QStringList planned_labels{tr("Electric heating planned"),
									 tr("Air conditioning planned"),
									 tr("Automation or energy management planned"),
									 tr("Electric vehicle charging planned"),
									 tr("Independent dryer planned"),
									 tr("Single family dwelling")};
	for (int i = 0; i < planned.size(); ++i) {
		auto *combo = new QComboBox;
		combo->setObjectName(planned.at(i).first);
		combo->addItem(tr("Unknown"), -1);
		combo->addItem(tr("Yes"), 1);
		combo->addItem(tr("No"), 0);
		const auto &value = planned.at(i).second;
		combo->setCurrentIndex(!value ? 0 : (*value ? 1 : 2));
		m_planned.insert(planned.at(i).first, combo);
		dwelling_form->addRow(planned_labels.at(i), combo);
	}
	m_general_switch = new QLineEdit(formatNumber(board.generalSwitch));
	m_general_switch->setObjectName(QStringLiteral("generalSwitch"));
	m_general_switch->setPlaceholderText(tr("unknown"));
	dwelling_form->addRow(tr("General switch IGA (A)"), m_general_switch);
	m_rooms = board.rooms;
	m_rooms_button = new QPushButton;
	m_rooms_button->setObjectName(QStringLiteral("roomsButton"));
	connect(m_rooms_button, &QPushButton::clicked, this, [this] {
		RoomsDialog dialog(m_rooms, this);
		if (dialog.exec() == QDialog::Accepted) {
			m_rooms = dialog.rooms();
			updateState();
		}
	});
	dwelling_form->addRow(tr("Rooms (table 2)"), m_rooms_button);
	auto *dwelling_hint = new QLabel(tr(
		"For the board where the dwelling starts: fed by nothing, or by its "
		"DI. The grade of electrification follows from these data and the "
		"circuits; an unknown datum leaves it unknown."));
	dwelling_hint->setWordWrap(true);
	dwelling_layout->addWidget(dwelling_hint);

		//Residual current devices of this board
	auto *residual_box = new QGroupBox(tr("Residual current devices"));
	auto *residual_layout = new QVBoxLayout(residual_box);
	m_residual = new QTableWidget(0, 3);
	m_residual->setObjectName(QStringLiteral("residualDevices"));
	m_residual->setHorizontalHeaderLabels({tr("Name"), tr("Sensitivity (mA)"),
										   tr("Rated current (A)")});
	m_residual->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	m_residual->horizontalHeader()->setStretchLastSection(true);
	m_residual->verticalHeader()->hide();
	m_residual->setMinimumHeight(100);
	m_residual->setMaximumHeight(140);
	for (const ResidualDeviceData &d : board.residualDevices) {
		const int row = m_residual->rowCount();
		m_residual->insertRow(row);
		m_residual->setItem(row, 0, new QTableWidgetItem(d.name));
		m_residual->setItem(row, 1, new QTableWidgetItem(formatNumber(d.sensitivity)));
		m_residual->setItem(row, 2, new QTableWidgetItem(formatNumber(d.rated)));
	}
	residual_layout->addWidget(m_residual);
	auto *residual_buttons = new QHBoxLayout;
	auto *add_residual = new QPushButton(tr("Add"));
	auto *remove_residual = new QPushButton(tr("Remove"));
	residual_buttons->addWidget(add_residual);
	residual_buttons->addWidget(remove_residual);
	residual_buttons->addStretch();
	residual_layout->addLayout(residual_buttons);
	connect(add_residual, &QPushButton::clicked, this, &BoardDialog::addResidualDevice);
	connect(remove_residual, &QPushButton::clicked, this, [this] {
		if (m_residual->currentRow() >= 0)
			m_residual->removeRow(m_residual->currentRow());
		updateState();
	});
	connect(m_residual, &QTableWidget::itemChanged, this, &BoardDialog::updateState);
	auto *residual_hint = new QLabel(tr("Circuits refer to these devices by name."));
	residual_hint->setWordWrap(true);
	residual_layout->addWidget(residual_hint);

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

		//Two columns: the board and its supply, then the dwelling
	auto *left = new QVBoxLayout;
	left->addLayout(form);
	left->addWidget(hint);
	left->addWidget(m_compensate);
	left->addWidget(source_box);
	left->addWidget(residual_box);
	left->addStretch();
	auto *right = new QVBoxLayout;
	right->addWidget(dwelling_box);
	right->addWidget(earth_box);
	right->addStretch();
	auto *columns = new QHBoxLayout;
	columns->addLayout(left, 1);
	columns->addLayout(right, 1);
	auto *layout = new QVBoxLayout(this);
	layout->setSizeConstraint(QLayout::SetMinimumSize);
	layout->addLayout(columns);
	layout->addWidget(m_buttons);

	connect(m_feeder, &QComboBox::currentIndexChanged, this, &BoardDialog::updateState);
	connect(m_source, &QComboBox::currentIndexChanged, this, &BoardDialog::updateState);
	connect(m_source_max, &QLineEdit::textChanged, this, &BoardDialog::updateState);
	connect(m_source_min, &QLineEdit::textChanged, this, &BoardDialog::updateState);
	connect(m_area, &QLineEdit::textChanged, this, &BoardDialog::updateState);
	connect(m_general_switch, &QLineEdit::textChanged, this, &BoardDialog::updateState);
	connect(m_earthing, &QComboBox::currentIndexChanged, this, &BoardDialog::updateState);
	connect(m_earth_resistance, &QLineEdit::textChanged, this, &BoardDialog::updateState);
	connect(m_limit_voltage, &QLineEdit::textChanged, this, &BoardDialog::updateState);
	connect(m_first_fault, &QLineEdit::textChanged, this, &BoardDialog::updateState);
	m_model = &model;
	updateState();
		//Wrapped texts: Qt does not size a window for their height by itself
	resize(minimumWidth(), heightForWidth(minimumWidth()) > 0
						   ? heightForWidth(minimumWidth()) : sizeHint().height());
	setMinimumHeight(height());
}

void BoardDialog::addResidualDevice()
{
	const int row = m_residual->rowCount();
	m_residual->insertRow(row);
	m_residual->setItem(row, 0, new QTableWidgetItem(tr("ID%1").arg(row + 1)));
	m_residual->setItem(row, 1, new QTableWidgetItem);
	m_residual->setItem(row, 2, new QTableWidgetItem);
	updateState();
}

void BoardDialog::updateState()
{
		//A board fed by a circuit takes its Icc from the line
	const bool origin = m_feeder->currentData().toUuid().isNull();
	m_source_box->setEnabled(origin);
		//The dwelling starts where nothing, or an LGA or a DI, feeds it
	bool dwelling_origin = origin;
	if (!origin && m_model) {
		const auto feeder = m_model->circuit(m_feeder->currentData().toUuid());
		dwelling_origin = feeder && feeder->use && !isInterior(*feeder->use);
	}
	m_dwelling_box->setEnabled(dwelling_origin);
		//Compensation: only for a board fed by a DI or an LGA
	m_compensate->setEnabled(dwelling_origin && !origin);
	m_rooms_button->setText(tr("%n room(s)...", "", m_rooms.size()));
	const bool known = m_source->currentData().toInt()
					   == int(rebt::SourceMethod::KnownCurrent);
	m_source_max->setEnabled(known);
	m_source_min->setEnabled(known);
	m_source_reason->setEnabled(known);
	const int earthing = m_earthing->currentData().toInt();
	const bool it = earthing == int(rebt::EarthingSystem::IT);
	const bool with_ra = earthing == int(rebt::EarthingSystem::TT) || it;
	for (QWidget *w : std::initializer_list<QWidget *>{m_earth_resistance, m_earth_reason,
													   m_limit_voltage, m_limit_reason})
		w->setEnabled(with_ra);
	for (QWidget *w : std::initializer_list<QWidget *>{m_first_fault, m_first_fault_reason,
													   m_masses, m_neutral_distributed})
		w->setEnabled(it);
	bool valid = true;
	for (QLineEdit *edit : {m_source_max, m_source_min, m_area, m_general_switch,
							m_earth_resistance, m_limit_voltage, m_first_fault}) {
		bool ok = false;
		parseNumber(edit->text(), &ok);
		edit->setStyleSheet(ok ? QString()
							   : QStringLiteral("QLineEdit { background: #ffd6d6; }"));
		valid = valid && ok;
	}
	for (int row = 0; row < m_residual->rowCount(); ++row) {
		for (int column : {1, 2}) {
			QTableWidgetItem *item = m_residual->item(row, column);
			if (!item) continue;
			bool ok = false;
			parseNumber(item->text(), &ok);
			const QSignalBlocker blocker(m_residual);
			item->setBackground(ok ? QBrush() : QBrush(QColor(0xff, 0xd6, 0xd6)));
			valid = valid && ok;
		}
	}
	m_buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

Board BoardDialog::board() const
{
	Board b = m_board;
	b.name = m_name->text().trimmed();
	b.feeder = m_feeder->currentData().toUuid();
		//Kept only for an origin: a fed board has no supply data of its own
	b.source.reset();
	b.sourceMaxCurrent.reset();
	b.sourceMinCurrent.reset();
	b.sourceReason.clear();
	b.ownTransformer = false;
	b.dwellingArea.reset();
	b.plannedHeating.reset();
	b.plannedAirConditioning.reset();
	b.plannedAutomation.reset();
	b.plannedVehicleCharging.reset();
	b.singleFamily.reset();
	b.generalSwitch.reset();
	b.plannedDryer.reset();
	b.rooms.clear();
	b.compensateWithIndividualLine = m_compensate->isEnabled() && m_compensate->isChecked();
	if (m_dwelling_box->isEnabled()) {
		bool ok = false;
		b.dwellingArea = parseNumber(m_area->text(), &ok);
		b.generalSwitch = parseNumber(m_general_switch->text(), &ok);
		auto planned = [this](const char *name) -> std::optional<bool> {
			const int v = m_planned.value(QLatin1String(name))->currentData().toInt();
			if (v < 0) return std::nullopt;
			return v == 1;
		};
		b.plannedHeating = planned("heating");
		b.plannedAirConditioning = planned("airConditioning");
		b.plannedAutomation = planned("automation");
		b.plannedVehicleCharging = planned("vehicleCharging");
		b.plannedDryer = planned("dryer");
		b.rooms = m_rooms;
		b.singleFamily = planned("singleFamily");
	}
	b.residualDevices.clear();
	for (int row = 0; row < m_residual->rowCount(); ++row) {
		ResidualDeviceData d;
		bool ok = false;
		auto text = [&](int column) {
			return m_residual->item(row, column) ? m_residual->item(row, column)->text()
												 : QString();
		};
		d.name = text(0).trimmed();
		d.sensitivity = parseNumber(text(1), &ok);
		d.rated = parseNumber(text(2), &ok);
		if (!d.name.isEmpty() || d.sensitivity || d.rated)
			b.residualDevices << d;
	}
		//RA and U for TT and IT; Id and the arrangement for IT only
	b.earthing.reset();
	b.earthResistance.reset();
	b.earthResistanceReason.clear();
	b.limitVoltage.reset();
	b.limitVoltageReason.clear();
	b.firstFaultCurrent.reset();
	b.firstFaultReason.clear();
	b.massesInterconnected.reset();
	b.neutralDistributed.reset();
	const int earthing = m_earthing->currentData().toInt();
	if (earthing >= 0)
		b.earthing = static_cast<rebt::EarthingSystem>(earthing);
	if (b.earthing == rebt::EarthingSystem::IT) {
		bool ok = false;
		b.firstFaultCurrent = parseNumber(m_first_fault->text(), &ok);
		b.firstFaultReason = m_first_fault_reason->text().trimmed();
		auto yes = [](QComboBox *combo) -> std::optional<bool> {
			const int v = combo->currentData().toInt();
			if (v < 0) return std::nullopt;
			return v == 1;
		};
		b.massesInterconnected = yes(m_masses);
		b.neutralDistributed = yes(m_neutral_distributed);
	}
	if (b.earthing == rebt::EarthingSystem::TT || b.earthing == rebt::EarthingSystem::IT) {
		bool ok = false;
		b.earthResistance = parseNumber(m_earth_resistance->text(), &ok);
		b.earthResistanceReason = m_earth_reason->text().trimmed();
		b.limitVoltage = parseNumber(m_limit_voltage->text(), &ok);
		b.limitVoltageReason = m_limit_reason->text().trimmed();
	}
	if (b.feeder.isNull()) {
		b.ownTransformer = m_own_transformer->isChecked();
		const int source = m_source->currentData().toInt();
		if (source >= 0)
			b.source = static_cast<rebt::SourceMethod>(source);
		if (b.source == rebt::SourceMethod::KnownCurrent) {
			bool ok = false;
			b.sourceMaxCurrent = parseNumber(m_source_max->text(), &ok);
			b.sourceMinCurrent = parseNumber(m_source_min->text(), &ok);
			b.sourceReason = m_source_reason->text().trimmed();
		}
	}
	return b;
}
