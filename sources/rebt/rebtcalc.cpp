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
#include "rebtcalc.h"
#include "rebtrules.h"

#include <algorithm>
#include <cmath>

namespace rebt {

namespace {

const double SQRT3 = std::sqrt(3.0);

/**
	Collects what is missing or wrong in the input, field by field.
*/
class Checker
{
	public:
		std::vector<Diagnostic> m_diagnostics;
		bool m_invalid = false;
		bool m_missing = false;

		/// A value that must be known and greater than zero
		bool positive(const std::optional<double> &value, const char *field,
					  Issue missing)
		{
			if (!value) {
				add(missing, field);
				m_missing = true;
				return false;
			}
			return positive(*value, field);
		}

		/// A value, when known, must be greater than zero
		bool positiveIfSet(const std::optional<double> &value,
						   const char *field)
		{
			return !value || positive(*value, field);
		}

		/// A value, when known, must be zero or more
		bool notNegativeIfSet(const std::optional<double> &value,
							  const char *field)
		{
			if (!value)
				return true;
			if (!finite(*value, field))
				return false;
			if (*value < 0.0) {
				invalid(Issue::Negative, field);
				return false;
			}
			return true;
		}

		/// A value, when known, must be in (0, 1]
		bool fractionIfSet(const std::optional<double> &value,
						   const char *field)
		{
			if (!value)
				return true;
			if (!finite(*value, field))
				return false;
			if (*value <= 0.0 || *value > 1.0) {
				invalid(Issue::OutOfRange, field);
				return false;
			}
			return true;
		}

		/// A value, when known, must be finite
		bool finiteIfSet(const std::optional<double> &value, const char *field)
		{
			return !value || finite(*value, field);
		}

		void missing(Issue issue, const char *field)
		{
			add(issue, field);
			m_missing = true;
		}

		Status status() const
		{
			if (m_invalid)
				return Status::InvalidInput;
			if (m_missing)
				return Status::NotComputable;
			return Status::Ok;
		}

	private:
		bool positive(double value, const char *field)
		{
			if (!finite(value, field))
				return false;
			if (value <= 0.0) {
				invalid(Issue::NotPositive, field);
				return false;
			}
			return true;
		}

		bool finite(double value, const char *field)
		{
			if (!std::isfinite(value)) {
				invalid(Issue::NotFinite, field);
				return false;
			}
			return true;
		}

		void invalid(Issue issue, const char *field)
		{
			add(issue, field);
			m_invalid = true;
		}

		void add(Issue issue, const char *field)
		{
			Diagnostic diagnostic{issue, field};
			for (const auto &d : m_diagnostics)
				if (d == diagnostic)
					return;
			m_diagnostics.push_back(diagnostic);
		}
};

void checkCurrentInputs(const CircuitInput &input, Checker &checker)
{
	if (!input.system)
		checker.missing(Issue::MissingSystem, "system");
	checker.positive(input.voltage, "voltage", Issue::MissingVoltage);
	checker.positive(input.power, "power", Issue::MissingPower);
	if (input.powerFactor)
		checker.fractionIfSet(input.powerFactor, "powerFactor");
	else
		checker.missing(Issue::MissingPowerFactor, "powerFactor");
	checker.fractionIfSet(input.efficiency, "efficiency");
}

} // namespace

CurrentResult designCurrent(const CircuitInput &input)
{
	CurrentResult result;
	result.rules.push_back(rules::designCurrent());

	Checker checker;
	checkCurrentInputs(input, checker);
	result.diagnostics = checker.m_diagnostics;
	result.status = checker.status();
	if (result.status != Status::Ok)
		return result;

	double absorbed = *input.power;
	if (input.efficiency)
		absorbed /= *input.efficiency;
	else
		result.assumptions.push_back(Assumption::EfficiencyNotApplied);

	const double cos_phi = *input.powerFactor;
	if (*input.system == System::SinglePhase) {
		result.current = absorbed / (*input.voltage * cos_phi);
	} else {
		result.current = absorbed / (SQRT3 * *input.voltage * cos_phi);
		result.assumptions.push_back(Assumption::BalancedLoad);
	}
	return result;
}

double resistivity(Material material, double temperature)
{
	double rho20 = 1.0 / 58.0;
	double alpha20 = 0.00393;
	if (material == Material::Aluminium) {
		rho20 = 0.028264;
		alpha20 = 0.00403;
	}
	return rho20 * (1.0 + alpha20 * (temperature - 20.0));
}

VoltageDropResult voltageDrop(const CircuitInput &input)
{
	VoltageDropResult result;
	result.rules.push_back(rules::voltageDrop());

	Checker checker;
	if (!input.system)
		checker.missing(Issue::MissingSystem, "system");
	checker.positive(input.voltage, "voltage", Issue::MissingVoltage);
	if (input.powerFactor)
		checker.fractionIfSet(input.powerFactor, "powerFactor");
	else
		checker.missing(Issue::MissingPowerFactor, "powerFactor");
	checker.positive(input.length, "length", Issue::MissingLength);
	checker.positive(input.section, "section", Issue::MissingSection);
	checker.notNegativeIfSet(input.reactance, "reactance");

	if (input.conductivity) {
		checker.positiveIfSet(input.conductivity, "conductivity");
	} else {
		if (!input.material)
			checker.missing(Issue::MissingMaterial, "material");
		if (!input.conductorTemperature)
			checker.missing(Issue::MissingTemperature, "conductorTemperature");
		else
			checker.finiteIfSet(input.conductorTemperature,
								"conductorTemperature");
	}

	// The current: given, or worked out from the power. Its inputs are
	// checked here too, so one result lists everything missing.
	if (input.designCurrent)
		checker.positiveIfSet(input.designCurrent, "designCurrent");
	else
		checkCurrentInputs(input, checker);

	result.diagnostics = checker.m_diagnostics;
	result.status = checker.status();
	if (result.status != Status::Ok)
		return result;

	std::optional<double> current;
	if (input.designCurrent) {
		current = input.designCurrent;
		result.assumptions.push_back(Assumption::CurrentGiven);
	} else {
		const CurrentResult computed = designCurrent(input);
		current = computed.current;
		result.rules.insert(result.rules.end(), computed.rules.begin(),
							computed.rules.end());
		result.assumptions = computed.assumptions;
	}
	result.assumptions.push_back(Assumption::LoadAtEnd);

	double conductivity = 0.0;
	if (input.conductivity) {
		conductivity = *input.conductivity;
		result.assumptions.push_back(Assumption::ConductivityGiven);
	} else {
		conductivity = 1.0 / resistivity(*input.material,
										 *input.conductorTemperature);
		result.assumptions.push_back(Assumption::ResistivityFromMaterial);
		result.rules.push_back(rules::resistivity());
	}

	double x_per_metre = 0.0;
	if (input.reactance)
		x_per_metre = *input.reactance / 1000.0;
	else
		result.assumptions.push_back(Assumption::ReactanceNeglected);

	const double r_per_metre = 1.0 / (conductivity * *input.section);
	const double cos_phi = *input.powerFactor;
	const double sin_phi = std::sqrt(std::max(0.0, 1.0 - cos_phi * cos_phi));
	const double factor = *input.system == System::SinglePhase ? 2.0 : SQRT3;

	result.current = current;
	result.drop = factor * *input.length * *current
				  * (r_per_metre * cos_phi + x_per_metre * sin_phi);
	result.percent = 100.0 * *result.drop / *input.voltage;
	return result;
}

CheckResult checkVoltageDrop(const VoltageDropResult &drop,
							 const VoltageDropLimit &limit)
{
	CheckResult result;
	result.limit = limit.percent;
	result.rule = limit.rule;
	if (drop.status != Status::Ok || !drop.percent)
		return result;

	const double value = *drop.percent;
	result.value = value;
	const double tolerance = 1e-9;
	const bool equal = std::fabs(value - limit.percent) <= tolerance;
	bool passed = false;
	if (equal)
		passed = !limit.strict;
	else
		passed = value < limit.percent;
	result.outcome = passed ? CheckOutcome::Passed : CheckOutcome::NotPassed;
	return result;
}

} // namespace rebt
