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
#include "rebtindirect.h"

#include <algorithm>
#include <cmath>

namespace rebt {

namespace {

const double TOLERANCE = 1e-9;
/// Guia-BT Anexo 3: 0.8 U0 at the CGP
const double GUIDE_FACTOR = 0.8;

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

std::string prefixOf(std::size_t i, std::size_t count)
{
	return i + 1 == count ? std::string() : "feeder[" + std::to_string(i) + "]/";
}

CheckOutcome atMost(double value, double limit)
{
	return value <= limit + TOLERANCE ? CheckOutcome::Passed : CheckOutcome::NotPassed;
}

/// Reads the data and keeps what is wrong with them
class Check
{
	public:
		explicit Check(const IndirectContactInput &input, IndirectContactResult &r) :
			m_in(input), m_r(r) {}

		bool invalid() const { return m_invalid; }

		bool positive(const std::optional<double> &v, const std::string &field)
		{
			if (!v) return false;
			if (!std::isfinite(*v)) {
				add(m_r.diagnostics, Issue::NotFinite, field);
				m_invalid = true;
				return false;
			}
			if (*v <= 0.0) {
				add(m_r.diagnostics, Issue::NotPositive, field);
				m_invalid = true;
				return false;
			}
			return true;
		}

			/// U: the one given with its reason, or 50 V
		std::optional<double> limitVoltage()
		{
			if (!m_in.limitVoltage) {
				assume(m_r.assumptions, Assumption::NormalLimitVoltage);
				return NORMAL_LIMIT_VOLTAGE;
			}
			if (!positive(m_in.limitVoltage, "limitVoltage"))
				return std::nullopt;
			if (*m_in.limitVoltage > NORMAL_LIMIT_VOLTAGE + TOLERANCE) {
				add(m_r.diagnostics, Issue::LimitAboveNormal, "limitVoltage");
				m_invalid = true;
				return std::nullopt;
			}
			if (!justified(m_in.limitVoltageJustification)) {
				add(m_r.diagnostics, Issue::MissingJustification, "limitVoltage");
				return std::nullopt;
			}
			return m_in.limitVoltage;
		}

			/// RA, given with its source
		std::optional<double> earthResistance()
		{
			if (!m_in.earthResistance) {
				add(m_r.diagnostics, Issue::MissingEarthResistance, "earthResistance");
				return std::nullopt;
			}
			if (!positive(m_in.earthResistance, "earthResistance"))
				return std::nullopt;
			if (!justified(m_in.earthResistanceJustification)) {
				add(m_r.diagnostics, Issue::MissingJustification, "earthResistance");
				return std::nullopt;
			}
			assume(m_r.assumptions, Assumption::EarthResistanceGiven);
			return m_in.earthResistance;
		}

			/// Ia: the residual current device when there is one
		std::optional<double> operatingCurrent()
		{
			if (m_in.residualCurrent) {
				if (!positive(m_in.residualCurrent, "residualCurrent"))
					return std::nullopt;
				assume(m_r.assumptions, Assumption::ResidualOperatingCurrent);
				return m_in.residualCurrent;
			}
			std::optional<double> ia;
			if (m_in.tripCurrent) {
				if (!positive(m_in.tripCurrent, "tripCurrent"))
					return std::nullopt;
				if (!justified(m_in.tripCurrentJustification)) {
					add(m_r.diagnostics, Issue::MissingJustification, "tripCurrent");
					return std::nullopt;
				}
				ia = m_in.tripCurrent;
			} else if (m_in.standard && m_in.curve && tripFactor(*m_in.standard, *m_in.curve)
					   && positive(m_in.ratedCurrent, "ratedCurrent")) {
				ia = *tripFactor(*m_in.standard, *m_in.curve) * *m_in.ratedCurrent;
				assume(m_r.assumptions, Assumption::TripCurrentFromCurve);
				m_r.rules.push_back(rules::instantaneousTrip());
			} else if (!m_invalid) {
				add(m_r.diagnostics, Issue::MissingOperatingCurrent, "operatingCurrent");
			}
			return ia;
		}

			/**
				|Z| of the loop of the conductor @p first of each line
				(phase, or neutral) and its protective conductor
			*/
		std::optional<double> loop(bool neutral)
		{
			std::vector<LineStretch> lines = m_in.feeders;
			lines.push_back(m_in.circuit);
			bool complete = true;
			for (std::size_t i = 0; i < lines.size(); ++i) {
				LineStretch &l = lines[i];
				const std::string prefix = prefixOf(i, lines.size());
				if (!l.protectiveSection) {
					add(m_r.diagnostics, Issue::MissingProtectiveSection,
						prefix + "protectiveSection");
					complete = false;
				}
				if (neutral) {
					if (l.neutralSection) {
						l.section = l.neutralSection;
					} else if (!l.interior) {
						add(m_r.diagnostics, Issue::MissingNeutralSection,
							prefix + "neutralSection");
						complete = false;
					} else {
						assume(m_r.assumptions, Assumption::NeutralAsPhase);
					}
				}
				l.neutralSection = l.protectiveSection;
			}
			const LoopImpedanceResult z = hotLoopImpedance(lines);
			for (Diagnostic d : z.diagnostics) {
					// the second conductor of the loop is the protective one
				const std::string n = "neutralSection";
				if (d.field.size() >= n.size()
					&& d.field.compare(d.field.size() - n.size(), n.size(), n) == 0)
					d.field.replace(d.field.size() - n.size(), n.size(), "protectiveSection");
				if (d.issue == Issue::MissingNeutralSection)
					continue;   // already said as MissingProtectiveSection
				add(m_r.diagnostics, d.issue, d.field);
			}
			if (z.status == Status::InvalidInput)
				m_invalid = true;
			if (!complete || !z.impedance)
				return std::nullopt;
			for (Assumption a : z.assumptions)
				assume(m_r.assumptions, a);
			return z.impedance;
		}

	private:
		const IndirectContactInput &m_in;
		IndirectContactResult &m_r;
		bool m_invalid = false;
};

void finish(IndirectContactResult &r, bool invalid)
{
	if (invalid) {
			//No value is given from data that are not valid
		IndirectContactResult clean;
		clean.status = Status::InvalidInput;
		clean.diagnostics = r.diagnostics;
		clean.assumptions = r.assumptions;
		clean.rules = r.rules;
		r = clean;
		return;
	}
	if (r.outcome != CheckOutcome::NotPerformed)
		r.status = Status::Ok;
}

void checkTT(const IndirectContactInput &input, IndirectContactResult &r)
{
	Check c(input, r);
	r.rules.push_back(rules::indirectContactTT());
	r.limitVoltage = c.limitVoltage();
	const auto ra = c.earthResistance();
	r.operatingCurrent = c.operatingCurrent();
	if (!c.invalid() && r.limitVoltage && r.operatingCurrent)
		r.maxResistance = *r.limitVoltage / *r.operatingCurrent;
	if (!c.invalid() && ra && r.limitVoltage && r.operatingCurrent) {
		r.contactVoltage = *ra * *r.operatingCurrent;
		r.outcome = atMost(*r.contactVoltage, *r.limitVoltage);
	}
	finish(r, c.invalid());
}

void checkTN(const IndirectContactInput &input, IndirectContactResult &r)
{
	Check c(input, r);
	r.rules.push_back(rules::indirectContactTN());
	if (*input.earthing == EarthingSystem::TNC && input.residualCurrent) {
		add(r.diagnostics, Issue::ResidualInTNC, "residualDevice");
		r.outcome = CheckOutcome::NotPassed;
		r.status = Status::Ok;
		return;
	}
	r.operatingCurrent = c.operatingCurrent();
	if (r.operatingCurrent && !input.residualCurrent)
		assume(r.assumptions, Assumption::InstantaneousWithinTable);
	const bool u_ok = input.phaseVoltage ? c.positive(input.phaseVoltage, "voltage")
										 : (add(r.diagnostics, Issue::MissingVoltage, "voltage"), false);
	std::optional<double> source;   // ohm; guide: none, the factor instead
	bool source_ok = false;
	if (!input.pathKnown) {
		// the chain was not followed: said by the caller
	} else if (!input.source.method) {
		add(r.diagnostics, Issue::MissingSource, "source");
	} else if (*input.source.method == SourceMethod::GuideFactor) {
		source_ok = true;
		assume(r.assumptions, Assumption::GuideLoop);
		r.rules.push_back(rules::guideShortCircuit());
	} else if (!input.source.minCurrent) {
		add(r.diagnostics, Issue::MissingSourceCurrent, "sourceMinCurrent");
	} else if (c.positive(input.source.minCurrent, "sourceMinCurrent") && u_ok) {
		source = *input.phaseVoltage / *input.source.minCurrent;
		source_ok = true;
		assume(r.assumptions, Assumption::SourceCurrentGiven);
	}
	const std::optional<double> line = input.pathKnown ? c.loop(false) : std::nullopt;
	if (c.invalid() || !u_ok) {
		finish(r, c.invalid());
		return;
	}
	r.loopLimit = input.phaseVoltage;
	if (r.operatingCurrent)
		r.maxImpedance = *input.phaseVoltage / *r.operatingCurrent;
	if (source_ok && line) {
		r.loopImpedance = source ? *source + *line : *line / GUIDE_FACTOR;
		if (r.operatingCurrent) {
			r.loopVoltage = *r.loopImpedance * *r.operatingCurrent;
			r.outcome = atMost(*r.loopVoltage, *r.loopLimit);
		}
	}
	finish(r, false);
}

void checkIT(const IndirectContactInput &input, IndirectContactResult &r)
{
	Check c(input, r);
	r.rules.push_back(rules::indirectContactITFirst());
	r.rules.push_back(rules::indirectContactITSecond());
	r.limitVoltage = c.limitVoltage();
	const auto ra = c.earthResistance();

		//First fault: RA x Id <= UL
	std::optional<double> id;
	if (!input.firstFaultCurrent)
		add(r.diagnostics, Issue::MissingFirstFault, "firstFaultCurrent");
	else if (c.positive(input.firstFaultCurrent, "firstFaultCurrent")) {
		if (justified(input.firstFaultJustification))
			id = input.firstFaultCurrent;
		else
			add(r.diagnostics, Issue::MissingJustification, "firstFaultCurrent");
	}
	if (ra && id && r.limitVoltage) {
		r.firstFaultVoltage = *ra * *id;
		r.firstFault = atMost(*r.firstFaultVoltage, *r.limitVoltage);
	}

		//Second fault
	r.operatingCurrent = c.operatingCurrent();
	if (!input.massesInterconnected) {
		add(r.diagnostics, Issue::MissingMassesArrangement, "massesInterconnected");
	} else if (!*input.massesInterconnected) {
			//masses earthed apart: the condition of TT
		if (r.limitVoltage && r.operatingCurrent)
			r.maxResistance = *r.limitVoltage / *r.operatingCurrent;
		if (ra && r.limitVoltage && r.operatingCurrent) {
			r.contactVoltage = *ra * *r.operatingCurrent;
			r.secondFault = atMost(*r.contactVoltage, *r.limitVoltage);
		}
	} else if (!input.neutralDistributed) {
		add(r.diagnostics, Issue::MissingNeutralArrangement, "neutralDistributed");
	} else {
		if (r.operatingCurrent && !input.residualCurrent)
			assume(r.assumptions, Assumption::InstantaneousWithinTable);
		const bool u_ok = input.phaseVoltage
						  ? c.positive(input.phaseVoltage, "voltage")
						  : (add(r.diagnostics, Issue::MissingVoltage, "voltage"), false);
		const bool distributed = *input.neutralDistributed;
		const std::optional<double> z = input.pathKnown ? c.loop(distributed) : std::nullopt;
		if (z) assume(r.assumptions, Assumption::LoopFromItOrigin);
		if (u_ok) {
			r.loopLimit = distributed ? *input.phaseVoltage
									  : std::sqrt(3.0) * *input.phaseVoltage;
			if (r.operatingCurrent)
				r.maxImpedance = *r.loopLimit / (2.0 * *r.operatingCurrent);
			if (z) {
				r.loopImpedance = z;
				if (r.operatingCurrent) {
					r.loopVoltage = 2.0 * *z * *r.operatingCurrent;
					r.secondFault = atMost(*r.loopVoltage, *r.loopLimit);
				}
			}
		}
	}

	if (r.firstFault == CheckOutcome::NotPassed || r.secondFault == CheckOutcome::NotPassed)
		r.outcome = CheckOutcome::NotPassed;
	else if (r.firstFault == CheckOutcome::Passed && r.secondFault == CheckOutcome::Passed)
		r.outcome = CheckOutcome::Passed;
	finish(r, c.invalid());
}

} // namespace

IndirectContactResult checkIndirectContact(const IndirectContactInput &input)
{
	IndirectContactResult r;
	if (!input.earthing) {
		add(r.diagnostics, Issue::MissingEarthing, "earthing");
		return r;
	}
	switch (*input.earthing) {
		case EarthingSystem::TT: checkTT(input, r); break;
		case EarthingSystem::TN:
		case EarthingSystem::TNC: checkTN(input, r); break;
		case EarthingSystem::IT: checkIT(input, r); break;
	}
	return r;
}

namespace rules {

NormRef indirectContactTT()
{
	return {
		"REBT-CIN-001",
		"ITC-BT-24",
		"4.1, 4.1.2",
		"RD 842/2002 (BOE 18-09-2002): RA x Ia <= U, U 50 V in normal conditions",
		"RA is the value given (earth electrode plus protective conductors of "
		"the masses), not worked out from ITC-BT-18. Ia: IDn of the residual "
		"current device, or the instantaneous trip of the overcurrent device; "
		"the 5 s current of an inverse time device is not used. Not checked: "
		"that the masses of a device share one electrode, the operating time "
		"of selective devices (at most 1 s), direct contacts and the other "
		"measures of ITC-BT-24."
	};
}

NormRef indirectContactTN()
{
	return {
		"REBT-CIN-002",
		"ITC-BT-24",
		"4.1.1, table 1",
		"RD 842/2002 (BOE 18-09-2002): Zs x Ia <= U0; TN-C without residual "
		"current devices",
		"Zs: the source (U0 / Icc min given at the origin; with the guide "
		"factor Zs = R / 0.8) plus the phase and protective conductors of every "
		"line from the origin, resistances at the end of the fault, reactances "
		"added when given. Ia: IDn of the residual current device, or the "
		"instantaneous trip (under 0.1 s, within every time of table 1); the "
		"current for a longer time of table 1 is not used, nor the longer times "
		"of UNE 20460-4-41. Not checked: the earthing of the protective "
		"conductor, the PEN arrangement of TN-C-S."
	};
}

NormRef indirectContactITFirst()
{
	return {
		"REBT-CIN-003",
		"ITC-BT-24",
		"4.1.3",
		"RD 842/2002 (BOE 18-09-2002): first fault RA x Id <= UL",
		"Id is the value given: it depends on the leakage capacitances and is "
		"not worked out. The insulation monitor and its signal are not checked."
	};
}

NormRef indirectContactITSecond()
{
	return {
		"REBT-CIN-004",
		"ITC-BT-24",
		"4.1.3, table 2",
		"RD 842/2002 (BOE 18-09-2002): masses earthed apart, as TT; "
		"interconnected, 2 Zs x Ia <= U (neutral not distributed) or "
		"2 Z's x Ia <= U0 (distributed)",
		"Neutral not distributed: U = sqrt(3) U0. Zs: phase and protective "
		"conductors, Z's: neutral and protective conductors, of every line from "
		"the board where the IT system is given (feeders included, on the safe "
		"side), no source, resistances at the end of the fault. Ia as in TN, "
		"within every time of table 2; the longer times of UNE 20460-4-41 are "
		"not used. The neutral of the IT system is taken as not earthed."
	};
}

} // namespace rules
} // namespace rebt
