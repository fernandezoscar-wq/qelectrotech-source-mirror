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
#include "rebtampacity.h"
#include "rebtcalc.h"

#include <algorithm>
#include <cmath>

namespace rebt {

namespace {

const double SECTION_TOLERANCE = 1e-9;
const double MAX_FACTOR = 2.0;   ///< no correction factor is larger

/**
	The problems found, field by field; invalid wins over missing
*/
struct Problems
{
	std::vector<Diagnostic> list;
	bool invalid = false;
	bool missing = false;

	void add(Issue issue, const char *field, bool is_invalid)
	{
		const Diagnostic d{issue, field};
		if (std::find(list.begin(), list.end(), d) == list.end())
			list.push_back(d);
		(is_invalid ? invalid : missing) = true;
	}
	Status status() const
	{
		return invalid ? Status::InvalidInput
					   : missing ? Status::NotComputable : Status::Ok;
	}
};

/// A correction factor given by the user: in (0, 2] and with its reason
void checkFactor(const std::optional<double> &value,
				 const std::string &justification,
				 const char *field, Problems &problems)
{
	if (!value)
		return;
	if (!std::isfinite(*value))
		problems.add(Issue::NotFinite, field, true);
	else if (*value <= 0.0 || *value > MAX_FACTOR)
		problems.add(Issue::OutOfRange, field, true);
	if (justification.find_first_not_of(" \t\r\n") == std::string::npos)
		problems.add(Issue::MissingJustification, field, false);
}

/// Every check but the table and the section
void checkConditions(const AmpacityInput &input, Problems &problems)
{
	if (!input.material)
		problems.add(Issue::MissingMaterial, "material", false);
	if (!input.insulation)
		problems.add(Issue::MissingInsulation, "insulation", false);
	if (input.method.find_first_not_of(" \t") == std::string::npos)
		problems.add(Issue::MissingMethod, "installationMethod", false);
	if (input.loadedConductors) {
		if (*input.loadedConductors != 2 && *input.loadedConductors != 3)
			problems.add(Issue::OutOfRange, "loadedConductors", true);
	} else if (!input.system) {
		problems.add(Issue::MissingLoadedConductors, "loadedConductors", false);
	}

	checkFactor(input.temperatureFactor, input.temperatureJustification,
				"temperatureFactor", problems);
	if (!input.temperatureFactor) {
		if (!input.ambient)
			problems.add(Issue::MissingAmbient, "ambientTemperature", false);
		else if (!std::isfinite(*input.ambient))
			problems.add(Issue::NotFinite, "ambientTemperature", true);
		else if (input.insulation
				 && *input.ambient >= maxConductorTemperature(*input.insulation))
			problems.add(Issue::OutOfRange, "ambientTemperature", true);
	}

	if (!input.groupingFactor)
		problems.add(Issue::MissingGroupingFactor, "groupingFactor", false);
	checkFactor(input.groupingFactor, input.groupingJustification,
				"groupingFactor", problems);
	checkFactor(input.otherFactor, input.otherJustification,
				"otherFactor", problems);
}

int loadedConductorsOf(const AmpacityInput &input)
{
	if (input.loadedConductors)
		return *input.loadedConductors;
	return *input.system == System::SinglePhase ? 2 : 3;
}

std::string trimmed(const std::string &text)
{
	const auto first = text.find_first_not_of(" \t");
	if (first == std::string::npos)
		return std::string();
	return text.substr(first, text.find_last_not_of(" \t") - first + 1);
}

} // namespace

std::optional<double> AmpacityTable::currentAt(double section) const
{
	for (const AmpacityEntry &e : entries)
		if (std::fabs(e.section - section) <= SECTION_TOLERANCE)
			return e.current;
	return std::nullopt;
}

bool AmpacityLibrary::add(const AmpacityTable &table)
{
	if (find(table.material, table.insulation, table.loadedConductors, table.method))
		return false;
	AmpacityTable sorted = table;
	std::sort(sorted.entries.begin(), sorted.entries.end(),
			  [](const AmpacityEntry &a, const AmpacityEntry &b) {
		return a.section < b.section; });
	m_tables.push_back(sorted);
	return true;
}

const AmpacityTable *AmpacityLibrary::find(Material material,
										   Insulation insulation,
										   int loaded_conductors,
										   const std::string &method) const
{
	const std::string wanted = trimmed(method);
	for (const AmpacityTable &t : m_tables)
		if (t.material == material && t.insulation == insulation
			&& t.loadedConductors == loaded_conductors
			&& trimmed(t.method) == wanted)
			return &t;
	return nullptr;
}

double maxConductorTemperature(Insulation insulation)
{
	return insulation == Insulation::PVC ? 70.0 : 90.0;
}

std::optional<double> temperatureFactor(Insulation insulation,
										double ambient, double reference)
{
	const double max = maxConductorTemperature(insulation);
	if (ambient >= max || reference >= max)
		return std::nullopt;
	return std::sqrt((max - ambient) / (max - reference));
}

AmpacityResult admissibleCurrent(const AmpacityInput &input,
								 const AmpacityLibrary &library)
{
	AmpacityResult result;
	result.rules.push_back(rules::admissibleCurrent());

	Problems problems;
	checkConditions(input, problems);
	if (!input.section)
		problems.add(Issue::MissingSection, "section", false);
	else if (!std::isfinite(*input.section) || *input.section <= 0.0)
		problems.add(Issue::NotPositive, "section", true);

	const AmpacityTable *table = nullptr;
	if (problems.status() == Status::Ok) {
		table = library.find(*input.material, *input.insulation,
							 loadedConductorsOf(input), input.method);
		if (!table)
			problems.add(Issue::NoTable, "table", false);
		else if (!table->currentAt(*input.section))
			problems.add(Issue::SectionNotInTable, "section", false);
		else if (!input.temperatureFactor
				 && table->referenceAmbient >= maxConductorTemperature(*input.insulation))
			problems.add(Issue::OutOfRange, "referenceAmbient", true);
	}

	result.diagnostics = problems.list;
	result.status = problems.status();
	if (result.status != Status::Ok)
		return result;

	if (!input.loadedConductors)
		result.assumptions.push_back(Assumption::LoadedConductorsFromSystem);
	result.tableId = table->id;
	result.tableSource = table->source;
	result.tableCurrent = table->currentAt(*input.section);

	CorrectionFactor temperature{FactorKind::Temperature, 1.0,
								 input.temperatureJustification, false};
	if (input.temperatureFactor) {
		temperature.value = *input.temperatureFactor;
	} else {
		temperature.value = *temperatureFactor(*input.insulation, *input.ambient,
											   table->referenceAmbient);
		temperature.computed = true;
		result.assumptions.push_back(Assumption::TemperatureFactorComputed);
		result.rules.push_back(rules::temperatureFactor());
	}
	result.factors.push_back(temperature);
	result.factors.push_back({FactorKind::Grouping, *input.groupingFactor,
							  input.groupingJustification, false});
	if (input.otherFactor)
		result.factors.push_back({FactorKind::Other, *input.otherFactor,
								  input.otherJustification, false});

	double iz = *result.tableCurrent;
	for (const CorrectionFactor &f : result.factors)
		iz *= f.value;
	result.admissible = iz;
	return result;
}

CheckResult checkAmpacity(const std::optional<double> &design_current,
						  const AmpacityResult &ampacity)
{
	CheckResult result;
	result.rule = rules::admissibleCurrent();
	if (ampacity.admissible)
		result.limit = *ampacity.admissible;
	if (!design_current || ampacity.status != Status::Ok || !ampacity.admissible)
		return result;
	result.value = design_current;
		//IB <= IZ: equal passes
	const bool passed = *design_current <= *ampacity.admissible + 1e-9;
	result.outcome = passed ? CheckOutcome::Passed : CheckOutcome::NotPassed;
	return result;
}

SectionSuggestion suggestSection(const CircuitInput &circuit,
								 const AmpacityInput &ampacity,
								 const AmpacityLibrary &library,
								 const std::optional<VoltageDropLimit> &limit)
{
	SectionSuggestion suggestion;
	suggestion.rules.push_back(rules::admissibleCurrent());
	if (limit)
		suggestion.rules.push_back(limit->rule);

		//The current to carry
	std::optional<double> current = circuit.designCurrent;
	Problems problems;
	if (!current) {
		const CurrentResult computed = designCurrent(circuit);
		for (const Diagnostic &d : computed.diagnostics)
			problems.add(d.issue, d.field.c_str(),
						 computed.status == Status::InvalidInput);
		current = computed.current;
	} else if (!std::isfinite(*current) || *current <= 0.0) {
		problems.add(Issue::NotPositive, "designCurrent", true);
	}

	checkConditions(ampacity, problems);
	const AmpacityTable *table = nullptr;
	if (problems.status() == Status::Ok) {
		table = library.find(*ampacity.material, *ampacity.insulation,
							 loadedConductorsOf(ampacity), ampacity.method);
		if (!table)
			problems.add(Issue::NoTable, "table", false);
	}
	suggestion.diagnostics = problems.list;
	suggestion.status = problems.status();
	if (suggestion.status != Status::Ok)
		return suggestion;

	bool drop_known_for_all = true;
	for (const AmpacityEntry &row : table->entries) {
		AmpacityInput at_row = ampacity;
		at_row.section = row.section;
		const AmpacityResult iz = admissibleCurrent(at_row, library);
		if (iz.status != Status::Ok || *iz.admissible + 1e-9 < *current)
			continue;

		std::optional<double> drop_percent;
		if (limit) {
			CircuitInput drop_input = circuit;
			drop_input.section = row.section;
			drop_input.designCurrent = current;
			const VoltageDropResult drop = voltageDrop(drop_input);
			if (drop.status == Status::Ok) {
				drop_percent = drop.percent;
				if (checkVoltageDrop(drop, *limit).outcome != CheckOutcome::Passed)
					continue;
			} else {
				drop_known_for_all = false;
			}
		}

		suggestion.section = row.section;
		suggestion.admissible = iz.admissible;
		suggestion.dropPercent = drop_percent;
		suggestion.dropConsidered = limit.has_value() && drop_known_for_all;
		return suggestion;
	}
	suggestion.noneEnough = true;
	return suggestion;
}

namespace rules {

NormRef admissibleCurrent()
{
	return {
		"REBT-IZ-001",
		"ITC-BT-19",
		"2.2.3",
		"RD 842/2002, BOE 224 of 18/09/2002; refers to UNE 20460-5-523, "
		"now UNE-HD 60364-5-52",
		"IZ read in the table the user loaded: the program holds none. "
		"Checks IB <= IZ only; the coordination with the protection "
		"(IN, I2) is not checked here."
	};
}

NormRef temperatureFactor()
{
	return {
		"REBT-IZ-002",
		"UNE-HD 60364-5-52 (IEC 60364-5-52)",
		"Anexo B",
		"k = sqrt((theta max - theta a) / (theta max - theta ref)); "
		"theta max PVC 70 degC, XLPE and EPR 90 degC",
		"Computed: the tables of the standard round to two decimals and "
		"may differ by 0.01. Give the factor instead to use the table's."
	};
}

} // namespace rules

} // namespace rebt
