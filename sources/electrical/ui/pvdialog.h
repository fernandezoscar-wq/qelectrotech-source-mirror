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
#ifndef PVDIALOG_H
#define PVDIALOG_H

#include "electrical/electricalmodel.h"

#include <QDialog>
#include <QMap>

class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QTableWidget;

/**
	@brief The photovoltaic generator behind an inverter: its module and
	the DC input of the inverter (from their datasheets, with the source),
	the cell temperatures of the site and the groups of strings, one row
	per input. Shows the checks of the DC side as typed.
*/
class PvDialog : public QDialog
{
		Q_OBJECT

	public:
		explicit PvDialog(const electrical::PvData &pv, QWidget *parent = nullptr);

			/// As edited; empty fields not known
		electrical::PvData pv() const;
		bool isValid() const;

		QLineEdit *field(const QString &name) const { return m_fields.value(name); }
		QTableWidget *stringsTable() const { return m_strings; }
		QLabel *resultLabel() const { return m_result; }
		void addGroup();

	private:
		QLineEdit *addField(class QFormLayout *form, const QString &name, const QString &label,
							const QString &unit, const QString &text);
		void addRow(const electrical::PvStringGroup &group);
		void updateState();

		QMap<QString, QLineEdit *> m_fields;
		QTableWidget *m_strings = nullptr;
		QLabel *m_result = nullptr;
		QDialogButtonBox *m_buttons = nullptr;
};

#endif // PVDIALOG_H
