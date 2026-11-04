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
#ifndef CIRCUITDIALOG_H
#define CIRCUITDIALOG_H

#include "electrical/devicecatalogue.h"
#include "electrical/electricalmodel.h"
#include "rebt/rebtampacity.h"

#include <QDialog>
#include <QMap>

class QComboBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

/**
	@brief Edits the data of one circuit. Every field may be left empty,
	which means not known: nothing is filled in for the user. Numbers
	take a decimal comma or point. The calculation is shown as the data
	is typed.
*/
class CircuitDialog : public QDialog
{
		Q_OBJECT

	public:
		CircuitDialog(const electrical::Circuit &circuit,
					  const QList<electrical::Board> &boards,
					  QWidget *parent = nullptr);

			/// The circuit as edited; fields not shown here are kept
		electrical::Circuit circuit() const;
			/// No field holds text that is not a number
		bool isValid() const;
			/// The field named @p name (objectName), for tests
		QLineEdit *numberField(const QString &name) const;
		QComboBox *choiceField(const QString &name) const;
		QComboBox *residualDeviceField() const { return m_residual; }
		QLabel *resultLabel() const { return m_result; }
		QPushButton *useSuggestionButton() const { return m_use_suggestion; }
		QLineEdit *reasonField(const QString &name) const { return m_reasons.value(name); }
		QPushButton *catalogueButton() const { return m_catalogue; }
			/// Puts the data of @p entry in the fields of the device, all of
			/// them replaced; the residual current device named stays
		void applyDevice(const electrical::DeviceEntry &entry);
			/// The ampacity tables to compute with; by default those of the settings
		void setTables(const rebt::AmpacityLibrary &tables);
			/// The model the circuit belongs to, for the drop from the origin
		void setModel(const electrical::ElectricalModel &model);

	private:
		QLineEdit *addFactor(class QFormLayout *form, const QString &name,
							 const QString &label,
							 const std::optional<double> &value,
							 const QString &reason);
		QLineEdit *addNumber(class QFormLayout *form, const QString &name,
							 const QString &label, const QString &unit,
							 const std::optional<double> &value);
		QComboBox *addChoice(class QFormLayout *form, const QString &name,
							 const QString &label,
							 const QList<QPair<QString, int>> &choices,
							 int current);
		std::optional<double> number(const QString &name) const;
		int choice(const QString &name) const;
		void updateState();

		electrical::Circuit m_circuit;
		QList<QUuid> m_board_ids;
		QMap<QString, QLineEdit *> m_numbers;
		QMap<QString, QComboBox *> m_choices;
		QMap<QString, QLineEdit *> m_reasons;
		rebt::AmpacityLibrary m_tables;
		std::optional<electrical::ElectricalModel> m_model;
		QPushButton *m_use_suggestion = nullptr;
		QPushButton *m_catalogue = nullptr;
		std::optional<double> m_suggested;
		QLineEdit *m_name = nullptr;
		QLineEdit *m_method = nullptr;
		QLineEdit *m_curve = nullptr;
		QLineEdit *m_manufacturer = nullptr;
		QLineEdit *m_part_number = nullptr;
		QComboBox *m_residual = nullptr;   ///< name of the device of its board
		QPlainTextEdit *m_notes = nullptr;
		QLabel *m_result = nullptr;
		QDialogButtonBox *m_buttons = nullptr;
};

#endif // CIRCUITDIALOG_H
