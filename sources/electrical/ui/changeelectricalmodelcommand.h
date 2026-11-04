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
#ifndef CHANGEELECTRICALMODELCOMMAND_H
#define CHANGEELECTRICALMODELCOMMAND_H

#include "electrical/electricalmodel.h"

#include <QPointer>
#include <QUndoCommand>

class QETProject;

/**
	@brief Replaces the electrical model of a project, and puts it back
	when undone. Every change the user makes to boards and circuits goes
	through it, on the project's undo stack. It never merges.
*/
class ChangeElectricalModelCommand : public QUndoCommand
{
	public:
		ChangeElectricalModelCommand(QETProject *project,
									 const electrical::ElectricalModel &after,
									 const QString &text,
									 QUndoCommand *parent = nullptr);
		void undo() override;
		void redo() override;

			/// Push the change onto the undo stack of @p project.
			/// @return false, and nothing is pushed, when the model of the
			/// project is read only or would not change.
		static bool push(QETProject *project,
						 const electrical::ElectricalModel &after,
						 const QString &text);

	private:
		QPointer<QETProject> m_project;
		electrical::ElectricalModel m_before;
		electrical::ElectricalModel m_after;
};

#endif // CHANGEELECTRICALMODELCOMMAND_H
