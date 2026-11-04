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
#ifndef BOARDDIALOG_H
#define BOARDDIALOG_H

#include "electrical/electricalmodel.h"

#include <QDialog>
#include <QMap>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLineEdit;
class QTableWidget;

/**
	@brief Name of a board and the circuit that feeds it. A board fed by
	no circuit is the origin of the interior installation (or is fed by
	the service connection).
*/
class BoardDialog : public QDialog
{
		Q_OBJECT

	public:
		BoardDialog(const electrical::Board &board,
					const electrical::ElectricalModel &model,
					QWidget *parent = nullptr);
		electrical::Board board() const;
		QLineEdit *nameField() const { return m_name; }
		QComboBox *feederField() const { return m_feeder; }
		QComboBox *sourceField() const { return m_source; }
		QLineEdit *sourceMaxField() const { return m_source_max; }
		QLineEdit *sourceMinField() const { return m_source_min; }
		QLineEdit *sourceReasonField() const { return m_source_reason; }
		QCheckBox *ownTransformerField() const { return m_own_transformer; }
		QLineEdit *areaField() const { return m_area; }
		QLineEdit *generalSwitchField() const { return m_general_switch; }
			/// "heating", "airConditioning", "automation", "vehicleCharging",
			/// "singleFamily": Unknown, Yes, No
		QComboBox *plannedField(const QString &name) const { return m_planned.value(name); }
		QTableWidget *residualTable() const { return m_residual; }
		QCheckBox *compensateField() const { return m_compensate; }
		QComboBox *earthingField() const { return m_earthing; }
		QLineEdit *earthResistanceField() const { return m_earth_resistance; }
		QLineEdit *earthResistanceReasonField() const { return m_earth_reason; }
		QLineEdit *limitVoltageField() const { return m_limit_voltage; }
		QLineEdit *limitVoltageReasonField() const { return m_limit_reason; }
		QLineEdit *firstFaultField() const { return m_first_fault; }
		QLineEdit *firstFaultReasonField() const { return m_first_fault_reason; }
		QComboBox *massesField() const { return m_masses; }
		QComboBox *neutralDistributedField() const { return m_neutral_distributed; }
			/// The rooms as the rooms dialog left them
		void setRooms(const QList<electrical::RoomData> &rooms) { m_rooms = rooms; updateState(); }
		void addResidualDevice();

	private:
		void updateState();

		electrical::Board m_board;
		QLineEdit *m_name = nullptr;
		QComboBox *m_feeder = nullptr;
		QComboBox *m_source = nullptr;
		QLineEdit *m_source_max = nullptr;
		QLineEdit *m_source_min = nullptr;
		QLineEdit *m_source_reason = nullptr;
		QWidget *m_source_box = nullptr;
		QCheckBox *m_own_transformer = nullptr;
		QWidget *m_dwelling_box = nullptr;
		QLineEdit *m_area = nullptr;
		QLineEdit *m_general_switch = nullptr;
		QMap<QString, QComboBox *> m_planned;
		QTableWidget *m_residual = nullptr;
		QCheckBox *m_compensate = nullptr;
		QComboBox *m_earthing = nullptr;
		QLineEdit *m_earth_resistance = nullptr;
		QLineEdit *m_earth_reason = nullptr;
		QLineEdit *m_limit_voltage = nullptr;
		QLineEdit *m_limit_reason = nullptr;
		QLineEdit *m_first_fault = nullptr;
		QLineEdit *m_first_fault_reason = nullptr;
		QComboBox *m_masses = nullptr;
		QComboBox *m_neutral_distributed = nullptr;
		class QPushButton *m_rooms_button = nullptr;
		QList<electrical::RoomData> m_rooms;
		const electrical::ElectricalModel *m_model = nullptr;
		QDialogButtonBox *m_buttons = nullptr;
};

#endif // BOARDDIALOG_H
