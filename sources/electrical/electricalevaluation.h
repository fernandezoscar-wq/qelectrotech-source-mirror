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
#ifndef ELECTRICALEVALUATION_H
#define ELECTRICALEVALUATION_H

#include "electricalmodel.h"
#include "rebt/rebtampacity.h"
#include "rebt/rebtcalc.h"
#include "rebt/rebtdwelling.h"
#include "rebt/rebtprotection.h"
#include "rebt/rebtrules.h"
#include "rebt/rebtshortcircuit.h"

#include <QString>

#include <optional>

namespace electrical {

/**
	@brief What the calculations say of one circuit, as the panel shows it
*/
struct CircuitEvaluation
{
	rebt::CurrentResult current;     ///< from the power, when no current is given
	rebt::VoltageDropResult drop;
	std::optional<rebt::VoltageDropLimit> limit;  ///< none when the use is unknown
	rebt::CheckResult check;         ///< NotPerformed without a limit
	rebt::AmpacityResult ampacity;   ///< Iz, from the tables loaded
	rebt::CheckResult ampacityCheck; ///< IB <= IZ
	rebt::SectionSuggestion suggestion;  ///< smallest section that will do
	rebt::CoordinationResult coordination;   ///< with its protection

		// Voltage drop from the origin of the interior installation,
		// through the feeders of the boards upstream. Only with a model.
	bool chainEvaluated = false;     ///< a model was given
	std::optional<double> cumulativePercent;   ///< own + interior feeders
	QList<QUuid> feeders;            ///< the feeders added, nearest first
	QUuid originBoard;               ///< where the interior installation starts
	std::vector<rebt::Diagnostic> chainDiagnostics;

		// Short circuit, from the board fed by nothing down to the far
		// end of the circuit, through every feeder (LGA and DI too).
		// Only with a model: the supply is a datum of the origin board.
	rebt::ShortCircuitResult shortCircuit;
	QList<QUuid> faultFeeders;       ///< feeders of the fault path, origin first
	QStringList faultFeederNames;    ///< their names, same order
	QUuid faultOriginBoard;          ///< board whose supply data are used
	std::vector<rebt::Diagnostic> faultDiagnostics;   ///< of the path itself
		// Where the values given by the user come from, for the report
	QString sourceText;              ///< Icc at the origin and its source
	QString tripCurrentReason;
	QString letThroughReason;

		// Dwelling circuit (ITC-BT-25), when the circuit says which one
	rebt::DwellingResult dwelling;
	rebt::VoltageDropResult dropAtRated;   ///< own drop with IN of the breaker
	std::optional<double> cumulativeAtRated;   ///< and from the origin, %
		// Drop compensated with the DI (ITC-BT-19 2.2.2), when stated
	bool compensated = false;
	std::optional<double> individualPercent;   ///< drop of the DI, %
	std::optional<double> compensatedPercent;  ///< interior from the origin + DI

		// Indirect contacts (ITC-BT-24 4.1.2), with the earthing of the
		// nearest board up the chain that gives one. Only with a model.
	rebt::IndirectContactResult indirect;
	QUuid earthingBoard;             ///< board whose earthing data are used
	QString earthResistanceReason;
	QString limitVoltageReason;
	QString firstFaultReason;        ///< IT, where Id comes from
	QString residualDeviceName;      ///< the RCD whose IDn is Ia, when one

		/// The design current to show: computed or given, empty if neither
	std::optional<double> designCurrent() const;
};

/**
	@brief The residual current device named by the protection of
	@p circuit, on its board or on one upstream (it then protects the
	feeder of the board); @p holder, when given, gets the board it is on
*/
std::optional<ResidualDeviceData> residualDeviceOf(const Circuit &circuit,
												   const ElectricalModel &model,
												   QUuid *holder = nullptr);

/// The voltage drop limit of the use of @p circuit, none when unknown
std::optional<rebt::VoltageDropLimit> limitFor(const Circuit &circuit);

/**
	@brief Every calculation of @p circuit
	@param tables the ampacity tables; none: IZ and the suggested
	section are not computed, and the result says why
*/
CircuitEvaluation evaluate(const Circuit &circuit,
						   const rebt::AmpacityLibrary &tables = rebt::AmpacityLibrary());
/**
	@brief Every calculation of @p circuit, the drop added up from the
	origin through the feeders of @p model. Interior circuits are then
	checked against their limit with that drop: a complete check, not a
	partial one. Lines of the service connection (LGA, DI) are checked
	with their own drop, as their ITC says.
*/
CircuitEvaluation evaluate(const Circuit &circuit, const ElectricalModel &model,
						   const rebt::AmpacityLibrary &tables = rebt::AmpacityLibrary());

/**
	@brief Basic electrification of the dwelling that starts at @p origin
	(ITC-BT-25 2.3.1): the dwelling circuits of every board whose
	interior installation starts there. Not performed when it has none.
*/
rebt::MinimumCircuitsResult minimumCircuits(const QUuid &origin,
											const ElectricalModel &model);

/**
	@brief Everything of ITC-BT-25 about the dwelling that starts at
	@p origin: grade, circuits required, IGA, DI, residual devices.
	@param tables to work out the IZ of the DI that feeds it
*/
rebt::DwellingCheck dwellingCheck(const QUuid &origin, const ElectricalModel &model,
								  const rebt::AmpacityLibrary &tables = rebt::AmpacityLibrary());

/**
	@brief The board where the interior installation of @p circuit
	starts, from the boards and their feeders only: the first one fed by
	nothing, or by an LGA or a DI. None when a feeder is missing, of
	unknown use, or the boards feed each other in a loop.
*/
std::optional<Board> interiorOrigin(const Circuit &circuit, const ElectricalModel &model);

/// @p board starts an interior installation: fed by nothing, or by an
/// LGA or a DI
bool isInteriorOrigin(const Board &board, const ElectricalModel &model);

/**
	@brief A number typed by the user
	Accepts a decimal point or a decimal comma, not both, and spaces
	around. An empty text is a value not known yet.
	@param ok false when the text is not empty and is not a number
*/
std::optional<double> parseNumber(const QString &text, bool *ok);

/// @p value in the user's language, no group separator; empty when unknown
QString formatNumber(const std::optional<double> &value, int decimals = -1);

} // namespace electrical

#endif // ELECTRICALEVALUATION_H
