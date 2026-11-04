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
#ifndef DEVICEPICKERDIALOG_H
#define DEVICEPICKERDIALOG_H

#include "electrical/devicecatalogue.h"

#include <QDialog>

#include <optional>

class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QTreeWidget;

/**
	@brief Chooses a protective device of the catalogue of the user, with
	a search by manufacturer, part number or source. Shows the values the
	catalogue gives and where they come from, as the catalogue says them.
*/
class DevicePickerDialog : public QDialog
{
		Q_OBJECT

	public:
		explicit DevicePickerDialog(const electrical::DeviceCatalogue &catalogue,
									QWidget *parent = nullptr);

			/// The device chosen; none when nothing is selected
		std::optional<electrical::DeviceEntry> entry() const;
			/// Selects the device of @p manufacturer and @p partNumber
		void select(const QString &manufacturer, const QString &partNumber);

		QLineEdit *searchField() const { return m_search; }
		QTreeWidget *list() const { return m_list; }
		QLabel *detailsLabel() const { return m_details; }
		QDialogButtonBox *buttons() const { return m_buttons; }

	private:
		void filter(const QString &text);
		void updateState();

		electrical::DeviceCatalogue m_catalogue;
		QLineEdit *m_search = nullptr;
		QTreeWidget *m_list = nullptr;
		QLabel *m_details = nullptr;
		QDialogButtonBox *m_buttons = nullptr;
};

#endif // DEVICEPICKERDIALOG_H
