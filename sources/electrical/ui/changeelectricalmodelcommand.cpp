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
#include "changeelectricalmodelcommand.h"

#include "../../qetproject.h"

ChangeElectricalModelCommand::ChangeElectricalModelCommand(
		QETProject *project,
		const electrical::ElectricalModel &after,
		const QString &text,
		QUndoCommand *parent) :
	QUndoCommand(text, parent),
	m_project(project),
	m_before(project->electricalModel()),
	m_after(after)
{}

void ChangeElectricalModelCommand::undo()
{
	if (m_project)
		m_project->setElectricalModel(m_before, false);
}

void ChangeElectricalModelCommand::redo()
{
	if (m_project)
		m_project->setElectricalModel(m_after, false);
}

bool ChangeElectricalModelCommand::push(QETProject *project,
										const electrical::ElectricalModel &after,
										const QString &text)
{
	if (!project
		|| project->electricalModel().isReadOnly()
		|| project->electricalModel() == after)
		return false;
	project->undoStack()->push(
		new ChangeElectricalModelCommand(project, after, text));
	return true;
}
