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
#ifndef ELECTRICALCIRCUITSDOCK_H
#define ELECTRICALCIRCUITSDOCK_H

#include "electrical/electricalmodel.h"

#include <QDockWidget>
#include <QPointer>

#include <functional>

class Diagram;
class QAction;
class QETProject;
class QLabel;
class QTreeWidget;
class QTreeWidgetItem;

/**
	@brief The boards of a project and the circuits leaving them, with
	what the calculations say of each: design current, voltage drop and
	its check. Every change goes through the undo stack of the project.

	Shown only when "Electrical circuits panel" is on in the settings.
*/
class ElectricalCircuitsDock : public QDockWidget
{
		Q_OBJECT

	public:
		enum Column {
			NameColumn,
			SystemColumn,
			VoltageColumn,
			PowerColumn,
			LengthColumn,
			SectionColumn,
			ProtectionColumn,
			CurrentColumn,
			DropColumn,
			DropTotalColumn,
			CheckColumn,
			AmpacityColumn,
			AmpacityCheckColumn,
			CoordinationColumn,
			MaxFaultColumn,
			MinFaultColumn,
			ShortCircuitColumn,
			IndirectColumn,
			PvColumn,
			DwellingColumn,
			ColumnCount
		};

		enum class Role {
			Origin,       ///< one symbol: where the circuit starts
			Destination,  ///< one symbol: where it ends
			Protection,   ///< one symbol: its protection device
			Wires,        ///< conductors: those drawing the circuit
			BoardSymbol   ///< one symbol: the board itself
		};

		/// The settings key that shows the panel; off by default
		static QString settingKey();
		static bool isEnabledInSettings();

		explicit ElectricalCircuitsDock(QWidget *parent = nullptr);

		void setProject(QETProject *project);
		QETProject *project() const { return m_project; }
			/// The folio shown in the editor, for the selection
		void setDiagramProvider(std::function<Diagram *()> provider);

		QTreeWidget *tree() const { return m_tree; }
		QLabel *messageLabel() const { return m_message; }
		QLabel *summaryLabel() const { return m_summary; }

			/// The board or circuit of the current row, null if none
		QUuid currentId() const;
		void setCurrentId(const QUuid &id);

			// What the actions do, without a dialog (also for tests)
		QUuid createBoard(const QString &name);
		QUuid createCircuit(const QUuid &board, const QString &name);
		bool applyCircuit(const electrical::Circuit &circuit);
		bool renameBoard(const QUuid &board, const QString &name);
			/// Name and feeder of a board, as BoardDialog edits them
		bool applyBoard(const electrical::Board &board);
		bool removeItem(const QUuid &id);
			/// Link the selected symbols or conductors to the current row.
			/// @return why it could not, empty when done
		QString assignSelection(Role role);
			/// Select in the folio shown what the current row points at.
			/// @return how many items were selected
		int showInDrawing();
			/// Use the ampacity tables of @p path, saved in the settings.
			/// @return what reading them found, for the user
		QString loadTables(const QString &path);
			/// Chooses the device catalogue file; what was read, for the user
		QString loadDeviceCatalogue(const QString &path);
			/// Keep @p info with the project (one undoable step) and write the
			/// calculation report. @return why it could not, empty when done
		QString exportReport(const QString &path, const electrical::ReportInfo &info,
							 bool withFolios);

	public slots:
		void refresh();

	private slots:
		void addBoardInteractive();
		void addCircuitInteractive();
		void editCurrentInteractive();
		void removeCurrentInteractive();
		void loadTablesInteractive();
		void loadDeviceCatalogueInteractive();
		void exportReportInteractive();
		void updateActions();

	private:
		void fillCircuitRow(QTreeWidgetItem *item, const electrical::Circuit &circuit);
		QString circuitName(const electrical::Circuit &circuit) const;
		bool push(const electrical::ElectricalModel &model, const QString &text);
		void showMessage(const QString &text);

		QPointer<QETProject> m_project;
		std::function<Diagram *()> m_diagram_provider;
		QTreeWidget *m_tree = nullptr;
		QLabel *m_message = nullptr;
		QLabel *m_summary = nullptr;
		QAction *m_add_board = nullptr;
		QAction *m_add_circuit = nullptr;
		QAction *m_edit = nullptr;
		QAction *m_remove = nullptr;
		QAction *m_assign = nullptr;
		QAction *m_show = nullptr;
		QAction *m_tables = nullptr;
		QAction *m_report = nullptr;
		QMetaObject::Connection m_model_connection;
};

#endif // ELECTRICALCIRCUITSDOCK_H
