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
#ifndef ROOMSDIALOG_H
#define ROOMSDIALOG_H

#include "electrical/electricalmodel.h"

#include <QDialog>

class QDialogButtonBox;
class QTableWidget;

/**
	@brief The rooms of a dwelling, one row each, with what each has:
	what table 2 of ITC-BT-25 asks of them. An empty cell is not known.
*/
class RoomsDialog : public QDialog
{
		Q_OBJECT

	public:
		RoomsDialog(const QList<electrical::RoomData> &rooms, QWidget *parent = nullptr);
		QList<electrical::RoomData> rooms() const;
		QTableWidget *table() const { return m_table; }
		void addRoom();
			/// Columns after name, type, area, length and accesses
		static const QList<rebt::RoomItem> &items();

	private:
		void addRow(const electrical::RoomData &room);
		void updateState();

		QTableWidget *m_table = nullptr;
		QDialogButtonBox *m_buttons = nullptr;
};

#endif // ROOMSDIALOG_H
