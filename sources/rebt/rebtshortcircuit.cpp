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
#include "rebtshortcircuit.h"

#include "rebtcalc.h"

#include <algorithm>
#include <cmath>

namespace rebt {

namespace {

const double TOLERANCE = 1e-9;
const char *const REBT_2002 = "RD 842/2002, BOE 224 of 18/09/2002";
/// Guia-BT Anexo 3: voltage at the CGP during the fault, network unknown
const double GUIDE_FACTOR = 0.8;
/// IEC 60909-0: temperature coefficient for the end of the fault
const double ALPHA_60909 = 0.004;
/// UNE-EN 60898-1: instantaneous trip, in under 0.1 s
const double INSTANTANEOUS_TIME = 0.1;

void add(std::vector<Diagnostic> &list, Issue issue, const std::string &field)
{
	const Diagnostic d{issue, field};
	if (std::find(list.begin(), list.end(), d) == list.end())
		list.push_back(d);
}

void assume(std::vector<Assumption> &list, Assumption a)
{
	if (std::find(list.begin(), list.end(), a) == list.end())
		list.push_back(a);
}

bool justified(const std::string &text)
{
	return text.find_first_not_of(" \t\r\n") != std::string::npos;
}

/// a >= b, both known
CheckOutcome atLeast(const std::optional<double> &a, const std::optional<double> &b)
{
	if (!a || !b)
		return CheckOutcome::NotPerformed;
	return *a + TOLERANCE >= *b ? CheckOutcome::Passed : CheckOutcome::NotPassed;
}

/**
	Reads the fields of the input and records what is wrong with them
*/
class Reader
{
	public:
		explicit Reader(ShortCircuitResult &result) : m_result(result) {}

		bool invalid() const { return m_invalid; }

			/// Positive when given; @p missing when not and @p issue is set
		bool positive(const std::optional<double> &value, const std::string &field,
					  std::optional<Issue> missing)
		{
			if (!value) {
				if (missing)
					add(m_result.diagnostics, *missing, field);
				return false;
			}
			if (!std::isfinite(*value)) {
				add(m_result.diagnostics, Issue::NotFinite, field);
				m_invalid = true;
				return false;
			}
			if (*value <= 0.0) {
				add(m_result.diagnostics, Issue::NotPositive, field);
				m_invalid = true;
				return false;
			}
			return true;
		}

		void missing(Issue issue, const std::string &field)
		{ add(m_result.diagnostics, issue, field); }

		void negative(const std::string &field)
		{
			add(m_result.diagnostics, Issue::Negative, field);
			m_invalid = true;
		}

		void outOfRange(const std::string &field)
		{
			add(m_result.diagnostics, Issue::OutOfRange, field);
			m_invalid = true;
		}

	private:
		ShortCircuitResult &m_result;
		bool m_invalid = false;
};

/// Resistances and reactance of one line, as far as they are known
struct Line
{
	std::optional<double> phase20;    ///< ohm
	std::optional<double> neutral20;  ///< ohm, may be assumed equal
	bool neutralAssumed = false;
	std::optional<double> hotFactor;  ///< R(theta_e) / R20
	std::optional<double> loopReactance;  ///< ohm, phase and neutral
};

/**
	@param forMinimum the neutral is needed exactly, not just bounded
*/
Line readLine(const LineStretch &stretch, const std::string &prefix,
			  Reader &reader, bool forMinimum)
{
	Line line;
	const bool l_ok = reader.positive(stretch.length, prefix + "length",
									  Issue::MissingLength);
	const bool s_ok = reader.positive(stretch.section, prefix + "section",
									  Issue::MissingSection);
	if (!stretch.material)
		reader.missing(Issue::MissingMaterial, prefix + "material");
	bool n_ok = false;
	if (stretch.neutralSection)
		n_ok = reader.positive(stretch.neutralSection, prefix + "neutralSection",
							   std::nullopt);
	if (stretch.reactance) {
		if (!std::isfinite(*stretch.reactance))
			reader.positive(stretch.reactance, prefix + "reactance", std::nullopt);
		else if (*stretch.reactance < 0.0)
			reader.negative(prefix + "reactance");
	}

	if (!l_ok || !s_ok || !stretch.material)
		return line;

	const double rho = resistivity(*stretch.material, 20.0);
	line.phase20 = rho * *stretch.length / *stretch.section;
	if (n_ok) {
		line.neutral20 = rho * *stretch.length / *stretch.neutralSection;
	} else if (!stretch.neutralSection) {
			// Maximum: a smaller neutral only lowers the current, equal is
			// the upper bound. Minimum: only where ITC-BT-19 2.2.2 makes
			// the neutral at least the phase.
		if (!forMinimum || stretch.interior) {
			line.neutral20 = *line.phase20;
			line.neutralAssumed = true;
		} else {
			reader.missing(Issue::MissingNeutralSection, prefix + "neutralSection");
		}
	}

	if (stretch.insulation)
		line.hotFactor = 1.0 + ALPHA_60909
						 * (finalTemperature(*stretch.insulation, *stretch.section)
							- 20.0);
	else if (forMinimum)
		reader.missing(Issue::MissingInsulation, prefix + "insulation");

	if (stretch.reactance && std::isfinite(*stretch.reactance)
		&& *stretch.reactance >= 0.0)
		line.loopReactance = 2.0 * *stretch.reactance / 1000.0 * *stretch.length;
	return line;
}

} // namespace

std::optional<double> kFactor(Material material, Insulation insulation,
							  double section)
{
	if (!std::isfinite(section) || section <= 0.0)
		return std::nullopt;
	const bool copper = material == Material::Copper;
	if (insulation == Insulation::PVC) {
		if (section <= 300.0 + TOLERANCE)
			return copper ? 115.0 : 76.0;
		return copper ? 103.0 : 68.0;
	}
		// XLPE, EPR
	return copper ? 143.0 : 94.0;
}

double finalTemperature(Insulation insulation, double section)
{
	if (insulation == Insulation::PVC)
		return section <= 300.0 + TOLERANCE ? 160.0 : 140.0;
	return 250.0;
}

std::optional<double> tripFactor(ProtectionStandard standard, TripCurve curve)
{
	if (standard != ProtectionStandard::EN60898
		&& standard != ProtectionStandard::EN61009)
		return std::nullopt;
	switch (curve) {
		case TripCurve::B: return 5.0;
		case TripCurve::C: return 10.0;
		case TripCurve::D: return 20.0;
	}
	return std::nullopt;
}

ShortCircuitResult checkShortCircuit(const ShortCircuitInput &input)
{
	ShortCircuitResult result;
	Reader reader(result);

	if (!input.protectsAgainstShortCircuit) {
		add(result.diagnostics, Issue::NoShortCircuitProtection, "protectionType");
		result.rules.push_back(rules::breakingCapacity());
		return result;
	}

		// Supply
	const bool u_ok = reader.positive(input.phaseVoltage, "voltage",
									  Issue::MissingVoltage);
	bool source_ok = false;
	if (!input.source.method) {
		reader.missing(Issue::MissingSource, "source");
	} else if (*input.source.method == SourceMethod::GuideFactor) {
		source_ok = true;
		result.rules.push_back(rules::guideShortCircuit());
		assume(result.assumptions, Assumption::GuideSourceFactor);
	} else {
		result.rules.push_back(rules::knownSourceShortCircuit());
		const bool max_ok = reader.positive(input.source.maxCurrent,
											"sourceMaxCurrent",
											Issue::MissingSourceCurrent);
		const bool min_ok = reader.positive(input.source.minCurrent,
											"sourceMinCurrent",
											Issue::MissingSourceCurrent);
		if (max_ok && min_ok
			&& *input.source.minCurrent > *input.source.maxCurrent + TOLERANCE)
			reader.outOfRange("sourceMinCurrent");
		source_ok = max_ok || min_ok;
		assume(result.assumptions, Assumption::SourceCurrentGiven);
	}

		// Lines, each read once for the maximum and once for the minimum
	std::vector<Line> max_lines, min_lines;
	for (std::size_t i = 0; i < input.feeders.size(); ++i) {
		const std::string prefix = "feeder[" + std::to_string(i) + "]/";
		max_lines.push_back(readLine(input.feeders[i], prefix, reader, false));
		min_lines.push_back(readLine(input.feeders[i], prefix, reader, true));
	}
	const Line circuit = readLine(input.circuit, "", reader, true);
	min_lines.push_back(circuit);

	if (reader.invalid()) {
		result.status = Status::InvalidInput;
		return result;
	}

	const bool guide = input.source.method == SourceMethod::GuideFactor;

		// Maximum, at the board the circuit leaves from
	if (u_ok && source_ok) {
		bool known = true;
		double r = 0.0;
		for (const Line &line : max_lines) {
			if (!line.phase20 || !line.neutral20) {
				known = false;
				break;
			}
				// The guide adds outgoing and return conductors. With a
				// known source the phase alone gives the upper bound for
				// every kind of fault.
			r += guide ? *line.phase20 + *line.neutral20 : *line.phase20;
			if (guide && line.neutralAssumed)
				assume(result.assumptions, Assumption::NeutralAsPhase);
		}
		if (known && guide) {
			if (r <= 0.0)
				reader.missing(Issue::NoSourceImpedance, "feeders");
			else
				result.maxCurrent = GUIDE_FACTOR * *input.phaseVoltage / r;
		} else if (known && input.source.maxCurrent) {
			const double zs = *input.phaseVoltage / *input.source.maxCurrent;
			result.maxCurrent = *input.phaseVoltage / std::sqrt(zs * zs + r * r);
		}
		if (result.maxCurrent)
			assume(result.assumptions, Assumption::ResistanceAt20);
	}

		// Minimum, phase to neutral at the far end
	if (u_ok && source_ok && (guide || input.source.minCurrent)) {
		bool known = true;
		bool reactance_missing = false;
		double r = 0.0, x = 0.0;
		for (const Line &line : min_lines) {
			if (!line.phase20 || !line.neutral20 || !line.hotFactor) {
				known = false;
				break;
			}
			r += (*line.phase20 + *line.neutral20) * *line.hotFactor;
			if (line.loopReactance)
				x += *line.loopReactance;
			else
				reactance_missing = true;
		}
		if (known) {
			const double z_line = std::sqrt(r * r + x * x);
			if (guide)
				result.minCurrent = GUIDE_FACTOR * *input.phaseVoltage / z_line;
			else
				result.minCurrent = *input.phaseVoltage
									/ (*input.phaseVoltage / *input.source.minCurrent
									   + z_line);
			assume(result.assumptions, Assumption::ResistanceAtFinalTemperature);
			if (reactance_missing)
				assume(result.assumptions, Assumption::ReactanceNeglected);
			for (const Line &line : min_lines)
				if (line.neutralAssumed)
					assume(result.assumptions, Assumption::NeutralAsPhase);
		}
	}

		// Breaking capacity, at the start of the circuit
	result.rules.push_back(rules::breakingCapacity());
	if (reader.positive(input.breakingCapacity, "breakingCapacity",
						Issue::MissingBreakingCapacity))
		result.breaking = atLeast(input.breakingCapacity, result.maxCurrent);

		// Instantaneous trip for the minimum current
	bool trip_from_curve = false;
	const bool rated_ok = input.ratedCurrent
						  && reader.positive(input.ratedCurrent, "ratedCurrent",
											 std::nullopt);
	if (input.tripCurrent) {
		if (!justified(input.tripCurrentJustification))
			reader.missing(Issue::MissingJustification, "tripCurrent");
		else if (reader.positive(input.tripCurrent, "tripCurrent", std::nullopt))
			result.tripCurrent = input.tripCurrent;
	} else if (!input.standard) {
		reader.missing(Issue::MissingProtectionStandard, "protectionStandard");
	} else if (input.standard == ProtectionStandard::EN60898
			   || input.standard == ProtectionStandard::EN61009) {
		if (!input.curve)
			reader.missing(Issue::MissingTripCurrent, "curve");
		if (!rated_ok)
			reader.missing(Issue::MissingRatedCurrent, "ratedCurrent");
		if (input.curve && rated_ok) {
			result.tripCurrent = *tripFactor(*input.standard, *input.curve)
								 * *input.ratedCurrent;
			trip_from_curve = true;
			assume(result.assumptions, Assumption::TripCurrentFromCurve);
			result.rules.push_back(rules::instantaneousTrip());
		}
	} else {
			// Industrial breakers and fuses: no range set by the standard
		reader.missing(Issue::MissingTripCurrent, "tripCurrent");
	}
	result.trip = atLeast(result.minCurrent, result.tripCurrent);

		// Thermal withstand of the circuit conductor
	result.rules.push_back(rules::thermalWithstand());
	if (input.circuit.material && input.circuit.section
		&& std::isfinite(*input.circuit.section) && *input.circuit.section > 0.0) {
		if (!input.circuit.insulation) {
			reader.missing(Issue::MissingInsulation, "insulation");
		} else {
			result.k = kFactor(*input.circuit.material, *input.circuit.insulation,
							   *input.circuit.section);
			if (!result.k)
				reader.missing(Issue::NoKFactor, "insulation");
		}
	}
	if (result.k) {
		const double ks = *result.k * *input.circuit.section;
		result.conductorEnergy = ks * ks;
		if (result.maxCurrent)
			result.allowedTime = (ks / *result.maxCurrent)
								 * (ks / *result.maxCurrent);
	}
	if (input.letThrough) {
		if (!justified(input.letThroughJustification))
			reader.missing(Issue::MissingJustification, "letThrough");
		else if (input.letThroughAt
				 && (!reader.positive(input.letThroughAt, "letThroughAt", std::nullopt)
					 || !result.maxCurrent
					 || *input.letThroughAt + TOLERANCE < *result.maxCurrent)) {
				// The I2t grows with the current: one given for a lower one,
				// or one that cannot be compared, is not used. A test current
				// that is not a value is reported by the reader.
			if (result.maxCurrent && *input.letThroughAt > 0.0)
				reader.missing(Issue::LetThroughBelowFault, "letThrough");
		} else if (reader.positive(input.letThrough, "letThrough", std::nullopt)
				 && result.conductorEnergy) {
			result.letThrough = input.letThrough;
			result.thermal = *input.letThrough <= *result.conductorEnergy + TOLERANCE
							 ? CheckOutcome::Passed : CheckOutcome::NotPassed;
		}
	} else if (result.trip == CheckOutcome::Passed) {
			// Every fault current is in the instantaneous range: the
			// device of the standard clears it in under 0.1 s.
		if (trip_from_curve && result.allowedTime
			&& *result.allowedTime + TOLERANCE >= INSTANTANEOUS_TIME) {
			result.thermal = CheckOutcome::Passed;
			assume(result.assumptions, Assumption::InstantaneousTripTime);
		} else if (result.conductorEnergy) {
			reader.missing(Issue::MissingLetThrough, "letThrough");
		}
	}

	if (reader.invalid()) {
		result.status = Status::InvalidInput;
		result.breaking = result.trip = result.thermal = CheckOutcome::NotPerformed;
		return result;
	}

	const CheckOutcome all[] = {result.breaking, result.trip, result.thermal};
	if (std::any_of(std::begin(all), std::end(all),
					[](CheckOutcome o) { return o == CheckOutcome::NotPassed; }))
		result.outcome = CheckOutcome::NotPassed;
	else if (std::all_of(std::begin(all), std::end(all),
						 [](CheckOutcome o) { return o == CheckOutcome::Passed; }))
		result.outcome = CheckOutcome::Passed;

	result.status = result.diagnostics.empty() ? Status::Ok : Status::NotComputable;
	return result;
}

LoopImpedanceResult hotLoopImpedance(const std::vector<LineStretch> &lines)
{
	LoopImpedanceResult out;
	ShortCircuitResult scratch;
	Reader reader(scratch);
	std::vector<Line> read;
	for (std::size_t i = 0; i < lines.size(); ++i) {
		LineStretch stretch = lines[i];
		stretch.interior = false;    // the second conductor is never assumed
		const std::string prefix = i + 1 == lines.size()
								   ? std::string() : "feeder[" + std::to_string(i) + "]/";
		read.push_back(readLine(stretch, prefix, reader, true));
	}
	out.diagnostics = scratch.diagnostics;
	if (reader.invalid()) {
		out.status = Status::InvalidInput;
		return out;
	}
	double r = 0.0, x = 0.0;
	bool reactance_missing = false;
	for (const Line &line : read) {
		if (!line.phase20 || !line.neutral20 || !line.hotFactor)
			return out;
		r += (*line.phase20 + *line.neutral20) * *line.hotFactor;
		if (line.loopReactance) x += *line.loopReactance;
		else reactance_missing = true;
	}
	if (read.empty())
		return out;
	out.impedance = std::sqrt(r * r + x * x);
	out.status = Status::Ok;
	assume(out.assumptions, Assumption::ResistanceAtFinalTemperature);
	if (reactance_missing)
		assume(out.assumptions, Assumption::ReactanceNeglected);
	return out;
}

namespace rules {

NormRef guideShortCircuit()
{
	return {
		"REBT-ICC-001",
		"Guia-BT Anexo 3",
		"Icc = 0.8 U / R",
		"Guia tecnica de aplicacion, Anexo 3, ed. sep 2003 rev. 1",
		"Only when the transformer is outside the building and the network is "
		"not known. R from the CGP: outgoing and return conductors at 20 degC, "
		"reactances neglected. Resistivity from IEC 60228 (copper 1/58), not "
		"the rounded 0.018 of the guide. The minimum applies the same factor "
		"to resistances at the end of the fault: an extension, not a figure "
		"of the guide."
	};
}

NormRef knownSourceShortCircuit()
{
	return {
		"REBT-ICC-002",
		"UNE-EN 60909-0",
		"resistances at 20 degC (maximum), at the end of the fault (minimum)",
		"IEC 60909-0:2016; Icc at the origin given by the user",
		"The angle of the source impedance is not known: the maximum takes it "
		"as a pure reactance and the minimum adds moduli, so both are bounds. "
		"The voltage factor c is the one already in the given currents. Motor "
		"contribution, asymmetry and peak current are not computed."
	};
}

NormRef breakingCapacity()
{
	return {
		"REBT-ICC-003",
		"ITC-BT-22",
		"1.1 b",
		std::string(REBT_2002) + "; Icu >= Icc max as in Guia-BT-22",
		"Maximum Icc at the board the circuit leaves from. Back-up "
		"(cascading) with an upstream device is not considered. The 4500 A "
		"minimum of the general switch (ITC-BT-17 1.3) is not checked here."
	};
}

NormRef instantaneousTrip()
{
	return {
		"REBT-ICC-004",
		"UNE-EN 60898-1; UNE-EN 61009-1",
		"instantaneous tripping ranges",
		"B 3 to 5 IN, C 5 to 10 IN, D 10 to 20 IN; upper limit used",
		"The minimum Icc at the far end must reach the upper limit, so that "
		"any device of the range trips without delay. Other devices: give the "
		"current of instantaneous trip of the manufacturer."
	};
}

NormRef thermalWithstand()
{
	return {
		"REBT-ICC-005",
		"UNE-HD 60364-4-43",
		"434.5.2, table 43A",
		"k: Cu PVC 115 (103 above 300 mm2), Cu XLPE/EPR 143, Al PVC 76 (68), "
		"Al XLPE/EPR 94",
		"Phase conductor of the circuit. With the manufacturer's I2t: "
		"I2t <= k2 S2, the I2t given for a prospective current at least the "
		"maximum Icc of the board (it grows with the current). Without it, passed only for UNE-EN 60898/61009 devices "
		"whose minimum fault is in the instantaneous range and whose "
		"(k S / Icc max)^2 is at least 0.1 s; asymmetry is not accounted for. "
		"Protective conductor not checked."
	};
}

} // namespace rules

} // namespace rebt
