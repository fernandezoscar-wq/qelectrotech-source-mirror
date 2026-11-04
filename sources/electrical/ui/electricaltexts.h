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
#ifndef ELECTRICALTEXTS_H
#define ELECTRICALTEXTS_H

#include "electrical/electricalevaluation.h"

#include <QCoreApplication>
#include <QString>

/**
	@brief What the user reads of the electrical model and the
	calculations: the codes of the engine put into words, translated.
*/
class ElectricalTexts
{
		Q_DECLARE_TR_FUNCTIONS(ElectricalTexts)

	public:
		static QString system(rebt::System system);
		static QString material(rebt::Material material);
		static QString use(electrical::CircuitUse use);
		static QString insulation(electrical::Insulation insulation);
		static QString protection(electrical::ProtectionType type);
		static QString field(const std::string &field);
		static QString diagnostic(const rebt::Diagnostic &diagnostic);
		static QString assumption(rebt::Assumption assumption);
		static QString rule(const rebt::NormRef &rule);
			/// One short word for the check column
		static QString outcome(const electrical::CircuitEvaluation &evaluation);
			/// Everything behind a result: missing data, assumptions, rules
		static QString details(const electrical::CircuitEvaluation &evaluation);
		static QString factor(rebt::FactorKind kind);
			/// One short word for the IB <= IZ column
		static QString ampacityOutcome(const electrical::CircuitEvaluation &evaluation);
			/// The table read, every factor with its reason, the rules
		static QString ampacityDetails(const electrical::CircuitEvaluation &evaluation);
		static QString suggestion(const electrical::CircuitEvaluation &evaluation);
		static QString standard(rebt::ProtectionStandard standard);
		static QString earthing(rebt::EarthingSystem earthing);
			/// "Passed"..., and "0.9 V ≤ 50 V" when computed
		static QString indirectOutcome(const electrical::CircuitEvaluation &evaluation);
			/// RA, Ia, U, contact voltage, what is missing, assumptions, rules
		static QString indirectDetails(const electrical::CircuitEvaluation &evaluation);
		static QString coordinationOutcome(const electrical::CircuitEvaluation &evaluation);
			/// The three conditions one by one, I2, the rules
		static QString coordinationDetails(const electrical::CircuitEvaluation &evaluation);
		static QString source(rebt::SourceMethod method);
			/// One short word for the short circuit column
		static QString shortCircuitOutcome(const electrical::CircuitEvaluation &evaluation);
			/// The currents, the three checks, what is missing, the rules
		static QString shortCircuitDetails(const electrical::CircuitEvaluation &evaluation);
			/// @p field of the fault path: "feeder[1]/length" with the feeder's name
		static QString faultField(const std::string &field, const QStringList &feeders);
			/// "C4 Washing machine, ..." as table 1 names it
		static QString dwellingCircuit(rebt::DwellingCircuit circuit);
			/// "C1", "C4.x", "C12 (C3)"...
		static QString dwellingCode(rebt::DwellingCircuit circuit);
		static QString dwellingOutcome(const electrical::CircuitEvaluation &evaluation);
			/// Each check of table 1 against its value, the drop with IN
		static QString dwellingDetails(const electrical::CircuitEvaluation &evaluation);
			/// C1 to C5 of the dwelling that starts at a board
		static QString minimumCircuitsDetails(const rebt::MinimumCircuitsResult &result);
		static QString elevatedReason(rebt::ElevatedReason reason);
		static QString roomType(rebt::RoomType type);
		static QString roomItem(rebt::RoomItem item);
			/// A field of a room diagnostic: "area", "lightPoints"...
		static QString roomField(const std::string &field);
			/// One short text for the board row: "Elevated: not passed"
		static QString dwellingCheckOutcome(const rebt::DwellingCheck &check);
			/// The grade and why, the circuits, the IGA, the DI, the devices
		static QString dwellingCheckDetails(const rebt::DwellingCheck &check);
};

#endif // ELECTRICALTEXTS_H
