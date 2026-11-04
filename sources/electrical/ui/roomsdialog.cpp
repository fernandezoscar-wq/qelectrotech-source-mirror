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
#include "roomsdialog.h"

#include "electrical/electricalevaluation.h"
#include "electricaltexts.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>

using namespace electrical;

namespace {
const int NAME = 0, TYPE = 1, AREA = 2, LENGTH = 3, ACCESSES = 4, FIRST_ITEM = 5;
}

const QList<rebt::RoomItem> &RoomsDialog::items()
{
	static const QList<rebt::RoomItem> list{
		rebt::RoomItem::Bell, rebt::RoomItem::LightPoints, rebt::RoomItem::Switches,
		rebt::RoomItem::Sockets, rebt::RoomItem::Cooker, rebt::RoomItem::ApplianceSockets,
		rebt::RoomItem::WetSockets, rebt::RoomItem::Heating,
		rebt::RoomItem::AirConditioning, rebt::RoomItem::Dryer,
		rebt::RoomItem::VehicleCharging};
	return list;
}

RoomsDialog::RoomsDialog(const QList<RoomData> &rooms, QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(tr("Rooms of the dwelling"));
	setObjectName(QStringLiteral("rooms_dialog"));
	resize(1250, 420);

	QStringList headers{tr("Name"), tr("Type"), tr("Area (m²)"), tr("Length (m)"),
						tr("Accesses")};
	for (rebt::RoomItem item : items())
		headers << ElectricalTexts::roomItem(item);
	m_table = new QTableWidget(0, headers.size());
	m_table->setObjectName(QStringLiteral("rooms"));
	m_table->setHorizontalHeaderLabels(headers);
	m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	m_table->verticalHeader()->hide();
	m_table->setWordWrap(true);
	for (const RoomData &room : rooms)
		addRow(room);
	connect(m_table, &QTableWidget::itemChanged, this, &RoomsDialog::updateState);

	auto *add = new QPushButton(tr("Add room"));
	auto *remove = new QPushButton(tr("Remove"));
	connect(add, &QPushButton::clicked, this, &RoomsDialog::addRoom);
	connect(remove, &QPushButton::clicked, this, [this] {
		if (m_table->currentRow() >= 0)
			m_table->removeRow(m_table->currentRow());
		updateState();
	});
	auto *row_buttons = new QHBoxLayout;
	row_buttons->addWidget(add);
	row_buttons->addWidget(remove);
	row_buttons->addStretch();

	auto *hint = new QLabel(tr(
		"Table 2 of ITC-BT-25 asks the minimum points of each room by its type "
		"and size: give the area, and for a corridor its length and accesses. "
		"An empty cell is not known: the check is then not performed. Heating, "
		"air conditioning, dryer and charging points count only when planned "
		"for the dwelling."));
	hint->setWordWrap(true);

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(hint);
	layout->addWidget(m_table, 1);
	layout->addLayout(row_buttons);
	layout->addWidget(m_buttons);
	updateState();
}

void RoomsDialog::addRow(const RoomData &room)
{
	const QSignalBlocker blocker(m_table);
	const int row = m_table->rowCount();
	m_table->insertRow(row);
	m_table->setItem(row, NAME, new QTableWidgetItem(room.name));
	auto *type = new QComboBox;
	type->addItem(tr("Unknown"), -1);
	const rebt::RoomType types[] = {
		rebt::RoomType::Access, rebt::RoomType::Hall, rebt::RoomType::LivingRoom,
		rebt::RoomType::Bedroom, rebt::RoomType::Bathroom, rebt::RoomType::Corridor,
		rebt::RoomType::Kitchen, rebt::RoomType::TerraceOrDressing,
		rebt::RoomType::GarageOrOther};
	for (rebt::RoomType t : types)
		type->addItem(ElectricalTexts::roomType(t), int(t));
	type->setCurrentIndex(room.type ? type->findData(int(*room.type)) : 0);
	m_table->setCellWidget(row, TYPE, type);
	m_table->setItem(row, AREA, new QTableWidgetItem(formatNumber(room.area)));
	m_table->setItem(row, LENGTH, new QTableWidgetItem(formatNumber(room.length)));
	m_table->setItem(row, ACCESSES, new QTableWidgetItem(
		room.accesses ? QString::number(*room.accesses) : QString()));
	for (int i = 0; i < items().size(); ++i) {
		const rebt::RoomItem item = items().at(i);
		m_table->setItem(row, FIRST_ITEM + i, new QTableWidgetItem(
			room.counts.contains(item) ? QString::number(room.counts.value(item))
									   : QString()));
	}
}

void RoomsDialog::addRoom()
{
	RoomData room;
	room.name = tr("Room %1").arg(m_table->rowCount() + 1);
	addRow(room);
	updateState();
}

void RoomsDialog::updateState()
{
	bool valid = true;
	const QSignalBlocker blocker(m_table);
	for (int row = 0; row < m_table->rowCount(); ++row) {
		for (int column = AREA; column < m_table->columnCount(); ++column) {
			QTableWidgetItem *cell = m_table->item(row, column);
			if (!cell) continue;
			bool ok = true;
			const QString text = cell->text().trimmed();
			if (column <= LENGTH) {
				parseNumber(text, &ok);
			} else if (!text.isEmpty()) {
					//Counts: whole numbers, zero or more
				const int n = text.toInt(&ok);
				ok = ok && n >= 0;
			}
			cell->setBackground(ok ? QBrush() : QBrush(QColor(0xff, 0xd6, 0xd6)));
			valid = valid && ok;
		}
	}
	m_buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

QList<RoomData> RoomsDialog::rooms() const
{
	QList<RoomData> list;
	for (int row = 0; row < m_table->rowCount(); ++row) {
		auto text = [&](int column) {
			const QTableWidgetItem *cell = m_table->item(row, column);
			return cell ? cell->text().trimmed() : QString();
		};
		RoomData room;
		room.name = text(NAME);
		const auto *type = qobject_cast<QComboBox *>(m_table->cellWidget(row, TYPE));
		if (type && type->currentData().toInt() >= 0)
			room.type = static_cast<rebt::RoomType>(type->currentData().toInt());
		bool ok = false;
		room.area = parseNumber(text(AREA), &ok);
		room.length = parseNumber(text(LENGTH), &ok);
		if (!text(ACCESSES).isEmpty()) {
			const int n = text(ACCESSES).toInt(&ok);
			if (ok) room.accesses = n;
		}
		for (int i = 0; i < items().size(); ++i) {
			const QString t = text(FIRST_ITEM + i);
			if (t.isEmpty()) continue;
			const int n = t.toInt(&ok);
			if (ok) room.counts.insert(items().at(i), n);
		}
		list << room;
	}
	return list;
}
