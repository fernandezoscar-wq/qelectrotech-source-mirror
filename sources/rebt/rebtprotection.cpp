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
#include "rebtprotection.h"

#include <algorithm>
#include <cmath>

namespace rebt {

namespace {

const double TOLERANCE = 1e-9;
const char *const REBT_2002 = "RD 842/2002, BOE 224 of 18/09/2002";

void add(std::vector<Diagnostic> &list, Issue issue, const char *field)
{
	const Diagnostic d{issue, field};
	if (std::find(list.begin(), list.end(), d) == list.end())
		list.push_back(d);
}

bool positive(const std::optional<double> &value)
{
	return value && std::isfinite(*value) && *value > 0.0;
}

/// a <= b, both known
CheckOutcome atMost(const std::optional<double> &a, const std::optional<double> &b)
{
	if (!a || !b)
		return CheckOutcome::NotPerformed;
	return *a <= *b + TOLERANCE ? CheckOutcome::Passed : CheckOutcome::NotPassed;
}

VoltageDropLimit connectionLimit(const char *id, const char *itc, double percent,
								 const char *scope)
{
	VoltageDropLimit limit;
	limit.percent = percent;
	limit.strict = false;   // the text says "maxima": equal is allowed
	limit.rule = {id, itc, "3", REBT_2002,
				  std::string(scope)
				  + " The drop of this line alone: it is not added to that "
					"of the interior installation."};
	return limit;
}

} // namespace

std::optional<double> conventionalFactor(ProtectionStandard standard, double rated)
{
	switch (standard) {
		case ProtectionStandard::EN60898:
		case ProtectionStandard::EN61009:
			return 1.45;
		case ProtectionStandard::EN60947_2:
			return 1.30;
		case ProtectionStandard::FuseGG:
			if (rated + TOLERANCE >= 16.0)
				return 1.6;
			return std::nullopt;
	}
	return std::nullopt;
}

CoordinationResult checkCoordination(const CoordinationInput &input)
{
	CoordinationResult result;
	result.rules.push_back(rules::overloadCoordination());

	if (!input.protectsAgainstOverload) {
		add(result.diagnostics, Issue::NoOverloadProtection, "protectionType");
		result.status = Status::NotComputable;
		return result;
	}

	bool invalid = false;
	auto check_value = [&](const std::optional<double> &value, const char *field,
						   Issue missing) {
		if (!value)
			add(result.diagnostics, missing, field);
		else if (!positive(value)) {
			add(result.diagnostics, Issue::NotPositive, field);
			invalid = true;
		}
	};
	check_value(input.designCurrent, "designCurrent", Issue::MissingPower);
	check_value(input.ratedCurrent, "ratedCurrent", Issue::MissingRatedCurrent);
	check_value(input.admissible, "admissibleCurrent", Issue::MissingAdmissibleCurrent);
	if (input.conventionalCurrent) {
		if (!positive(input.conventionalCurrent)) {
			add(result.diagnostics, Issue::NotPositive, "conventionalCurrent");
			invalid = true;
		}
		if (input.conventionalJustification.find_first_not_of(" \t\r\n")
			== std::string::npos)
			add(result.diagnostics, Issue::MissingJustification, "conventionalCurrent");
	}
	if (invalid) {
		result.status = Status::InvalidInput;
		return result;
	}

		//I2: given, or from the standard of the device
	std::optional<double> i2;
	if (input.conventionalCurrent && input.conventionalJustification
			.find_first_not_of(" \t\r\n") != std::string::npos) {
		i2 = input.conventionalCurrent;
	} else if (!input.conventionalCurrent) {
		if (!input.standard) {
			add(result.diagnostics, Issue::MissingProtectionStandard, "protectionStandard");
		} else if (input.ratedCurrent) {
			const auto factor = conventionalFactor(*input.standard, *input.ratedCurrent);
			if (factor) {
				i2 = *factor * *input.ratedCurrent;
				result.assumptions.push_back(Assumption::ConventionalCurrentFromStandard);
				result.rules.push_back(rules::conventionalCurrent());
			} else {
				add(result.diagnostics, Issue::MissingConventionalCurrent,
					"conventionalCurrent");
			}
		}
	}
	result.conventionalCurrent = i2;

		//Each condition that can be checked is
	result.ibIn = atMost(input.designCurrent, input.ratedCurrent);
	result.inIz = atMost(input.ratedCurrent, input.admissible);
	std::optional<double> limit;
	if (input.admissible)
		limit = 1.45 * *input.admissible;
	result.i2Iz = atMost(i2, limit);

	const CheckOutcome all[] = {result.ibIn, result.inIz, result.i2Iz};
	if (std::any_of(std::begin(all), std::end(all),
					[](CheckOutcome o) { return o == CheckOutcome::NotPassed; }))
		result.outcome = CheckOutcome::NotPassed;
	else if (std::all_of(std::begin(all), std::end(all),
						 [](CheckOutcome o) { return o == CheckOutcome::Passed; }))
		result.outcome = CheckOutcome::Passed;
	else
		result.outcome = CheckOutcome::NotPerformed;

	result.status = result.diagnostics.empty() ? Status::Ok : Status::NotComputable;
	return result;
}

namespace rules {

NormRef overloadCoordination()
{
	return {
		"REBT-PROT-001",
		"ITC-BT-22",
		"1.1",
		std::string(REBT_2002) + "; UNE 20460-4-43 433, now UNE-HD 60364-4-43; "
								 "conditions as in Guia-BT-22",
		"Overload only. IN is the rated current or, for an adjustable device, "
		"its setting. Short circuit (breaking capacity, I2t) is not checked here."
	};
}

NormRef conventionalCurrent()
{
	return {
		"REBT-PROT-002",
		"Guia-BT-22; UNE-EN 60269-1",
		"1.1",
		"I2 = 1.45 IN (UNE-EN 60898, UNE-EN 61009), 1.30 IN (UNE-EN 60947-2), "
		"1.6 IN for gG fuses from 16 A (UNE-EN 60269-1)",
		"gG fuses under 16 A and any other device: give the I2 of the "
		"manufacturer, with its source."
	};
}

VoltageDropLimit lgaTotallyCentralized()
{
	return connectionLimit("REBT-CDT-010", "ITC-BT-14", 0.5,
						   "General supply line (LGA), meters totally centralized.");
}

VoltageDropLimit lgaPartlyCentralized()
{
	return connectionLimit("REBT-CDT-011", "ITC-BT-14", 1.0,
						   "General supply line (LGA), meters partly centralized.");
}

VoltageDropLimit diMetersInSeveralPlaces()
{
	return connectionLimit("REBT-CDT-012", "ITC-BT-15", 0.5,
						   "Individual line (DI), meters grouped in more than one place.");
}

VoltageDropLimit diMetersTotallyGrouped()
{
	return connectionLimit("REBT-CDT-013", "ITC-BT-15", 1.0,
						   "Individual line (DI), meters totally grouped.");
}

VoltageDropLimit diSingleUser()
{
	return connectionLimit("REBT-CDT-014", "ITC-BT-15", 1.5,
						   "Individual line (DI) of a single user, with no LGA.");
}

} // namespace rules

} // namespace rebt
