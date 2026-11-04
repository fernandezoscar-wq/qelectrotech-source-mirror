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
#ifndef ELECTRICALREPORT_H
#define ELECTRICALREPORT_H

#include "electrical/electricalevaluation.h"
#include "electrical/electricalmodel.h"

#include <QCoreApplication>
#include <QList>
#include <QString>
#include <QStringList>

/**
	@brief The calculation annex of a project, as data: what the PDF
	shows, built from the same model and the same evaluation as the
	panel, so the document never says anything the panel does not.
*/
namespace electrical {

enum class CellState {
	Plain,
	Passed,
	NotPassed,
	NotPerformed
};

struct ReportCell
{
	QString text;
	CellState state = CellState::Plain;
};

using ReportRow = QList<ReportCell>;

/// The characteristics table of one board
struct BoardSection
{
	QString title;       ///< the board
	QString subtitle;    ///< what feeds it
	QList<ReportRow> rows;
	QList<QUuid> circuits;   ///< the circuit of each row
};

/// Every result of one circuit, in words
struct CircuitNotes
{
	QString title;
	QStringList lines;
	bool hasProblem = false;   ///< something not passed
};

/// The checks of one dwelling as a whole (ITC-BT-25)
struct DwellingNotes
{
	QString title;       ///< the board where it starts
	QString summary;     ///< "Elevated: not passed"
	rebt::CheckOutcome outcome = rebt::CheckOutcome::NotPerformed;
	QStringList lines;
};

struct CheckCounts
{
	int passed = 0;
	int notPassed = 0;
	int notPerformed = 0;
	void add(rebt::CheckOutcome outcome);
};

struct ElectricalReport
{
	ReportInfo info;
	QString projectTitle;
	QStringList columns;
	QList<double> columnWidths;   ///< mm, the same count as columns
	QList<BoardSection> boards;
	QList<CircuitNotes> notes;
	QList<DwellingNotes> dwellingNotes;
	QList<rebt::NormRef> rules;   ///< every rule applied, once, by id
	QStringList tables;           ///< the ampacity tables read, with their source
	int circuitCount = 0;
	CheckCounts drop;
	CheckCounts ampacity;
	CheckCounts coordination;
	CheckCounts shortCircuit;
	CheckCounts indirect;         ///< ITC-BT-24, counted when a board gives its earthing
	bool earthingGiven = false;   ///< some board of the project gives its earthing
	CheckCounts dwelling;         ///< dwelling circuits only
	CheckCounts dwellings;        ///< dwellings as a whole
	QString disclaimer;
};

class ElectricalReportBuilder
{
		Q_DECLARE_TR_FUNCTIONS(ElectricalReportBuilder)

	public:
		/**
			@param tables the ampacity tables, as the panel uses them
			@param tablesFile where they were read, empty if none
		*/
		static ElectricalReport build(const ElectricalModel &model,
									  const rebt::AmpacityLibrary &tables,
									  const QString &tablesFile,
									  const QString &projectTitle);
			/// The title of the document when the user gave none
		static QString defaultDocumentTitle();
};

} // namespace electrical

#endif // ELECTRICALREPORT_H
