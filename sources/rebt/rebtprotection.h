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
#ifndef REBTPROTECTION_H
#define REBTPROTECTION_H

#include "rebttypes.h"

/**
	@brief Coordination of a conductor with its overload protection
	(ITC-BT-22 1.1, UNE-HD 60364-4-43 433):
		IB <= IN <= IZ  and  I2 <= 1.45 IZ
	and the voltage drop limits of the service connection (LGA, DI).
*/
namespace rebt {

/// The product standard of the device, which sets its I2
enum class ProtectionStandard {
	EN60898,     ///< circuit breaker, household: I2 = 1.45 IN
	EN61009,     ///< RCBO, household: I2 = 1.45 IN
	EN60947_2,   ///< circuit breaker, industrial: I2 = 1.30 IN
	FuseGG       ///< gG fuse, UNE-EN 60269: I2 = 1.6 IN from 16 A
};

struct CoordinationInput
{
	std::optional<double> designCurrent;   ///< IB, A
	std::optional<double> ratedCurrent;    ///< IN (or setting IR), A
	std::optional<double> admissible;      ///< IZ, A
	std::optional<ProtectionStandard> standard;
	std::optional<double> conventionalCurrent;   ///< I2 given, A
	std::string conventionalJustification;
		/// false for a device that does not protect against overload
		/// (residual current device alone, switch)
	bool protectsAgainstOverload = true;
};

struct CoordinationResult
{
	Status status = Status::NotComputable;
	std::optional<double> conventionalCurrent;   ///< I2 used, A
	CheckOutcome ibIn = CheckOutcome::NotPerformed;   ///< IB <= IN
	CheckOutcome inIz = CheckOutcome::NotPerformed;   ///< IN <= IZ
	CheckOutcome i2Iz = CheckOutcome::NotPerformed;   ///< I2 <= 1.45 IZ
	CheckOutcome outcome = CheckOutcome::NotPerformed;   ///< the three together
	std::vector<Diagnostic> diagnostics;
	std::vector<Assumption> assumptions;
	std::vector<NormRef> rules;
};

/**
	@brief I2 / IN of a device of @p standard and rating @p rated
	@return nothing when the standard alone does not set it (gG fuse
	under 16 A: the manufacturer's I2 is needed)
*/
std::optional<double> conventionalFactor(ProtectionStandard standard, double rated);

/**
	@brief The three conditions. Each one that can be checked is, even
	when another cannot: the outcome is NotPassed as soon as one fails,
	Passed only when the three pass.
*/
CoordinationResult checkCoordination(const CoordinationInput &input);

namespace rules {
/// ITC-BT-22 1.1: IB <= IN <= IZ, I2 <= 1.45 IZ
NormRef overloadCoordination();
/// I2 of each kind of device
NormRef conventionalCurrent();
/// ITC-BT-14 3: LGA, meters totally centralized, at most 0.5 %
VoltageDropLimit lgaTotallyCentralized();
/// ITC-BT-14 3: LGA, meters partly centralized, at most 1 %
VoltageDropLimit lgaPartlyCentralized();
/// ITC-BT-15 3: DI, meters grouped in more than one place, at most 0.5 %
VoltageDropLimit diMetersInSeveralPlaces();
/// ITC-BT-15 3: DI, meters totally grouped, at most 1 %
VoltageDropLimit diMetersTotallyGrouped();
/// ITC-BT-15 3: DI of a single user with no LGA, at most 1.5 %
VoltageDropLimit diSingleUser();
}

} // namespace rebt

#endif // REBTPROTECTION_H
