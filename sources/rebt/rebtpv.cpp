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
#include "rebtpv.h"

#include "rebtcalc.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace rebt {

namespace {

const double TOLERANCE = 1e-9;

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

/// Passed only when every part passed; not passed when one did not
CheckOutcome combine(std::initializer_list<CheckOutcome> parts)
{
	bool all = true;
	for (CheckOutcome o : parts) {
		if (o == CheckOutcome::NotPassed) return CheckOutcome::NotPassed;
		all = all && o == CheckOutcome::Passed;
	}
	return all ? CheckOutcome::Passed : CheckOutcome::NotPerformed;
}

/// value <= limit, both known
CheckOutcome atMost(const std::optional<double> &value, const std::optional<double> &limit)
{
	if (!value || !limit) return CheckOutcome::NotPerformed;
	return *value <= *limit + TOLERANCE ? CheckOutcome::Passed : CheckOutcome::NotPassed;
}

class Reader
{
	public:
		explicit Reader(PvStringResult &r) : m_r(r) {}
		bool invalid() const { return m_invalid; }

			/// Greater than zero; @p missing when absent
		std::optional<double> positive(const std::optional<double> &v, const std::string &field,
									   Issue missing)
		{
			if (!v) {
				add(m_r.diagnostics, missing, field);
				return std::nullopt;
			}
			if (!std::isfinite(*v)) { bad(Issue::NotFinite, field); return std::nullopt; }
			if (*v <= 0.0) { bad(Issue::NotPositive, field); return std::nullopt; }
			return v;
		}

			/// Any finite value; @p missing when absent
		std::optional<double> finite(const std::optional<double> &v, const std::string &field,
									 Issue missing)
		{
			if (!v) {
				add(m_r.diagnostics, missing, field);
				return std::nullopt;
			}
			if (!std::isfinite(*v)) { bad(Issue::NotFinite, field); return std::nullopt; }
			return v;
		}

		std::optional<int> count(const std::optional<int> &v, const std::string &field)
		{
			if (!v) {
				add(m_r.diagnostics, Issue::MissingStringLayout, field);
				return std::nullopt;
			}
			if (*v <= 0) { bad(Issue::NotPositive, field); return std::nullopt; }
			return v;
		}

		void bad(Issue issue, const std::string &field)
		{
			add(m_r.diagnostics, issue, field);
			m_invalid = true;
		}

	private:
		PvStringResult &m_r;
		bool m_invalid = false;
};

} // namespace

PvStringResult checkPvString(const PvStringInput &in)
{
	PvStringResult r;
	Reader read(r);
	const PvModule &m = in.module;
	const PvInverterInput &inv = in.inverter;

		//The datasheets, with their source
	const bool module_source = justified(m.source);
	const bool inverter_source = justified(inv.source);
	if (!module_source) add(r.diagnostics, Issue::MissingJustification, "module");
	if (!inverter_source) add(r.diagnostics, Issue::MissingJustification, "inverter");
	auto mod = [&](const std::optional<double> &v, const char *field) {
		const auto value = read.positive(v, field, Issue::MissingModuleData);
		return module_source ? value : std::nullopt;
	};
	auto coef = [&](const std::optional<double> &v, const char *field) {
		const auto value = read.finite(v, field, Issue::MissingModuleData);
		return module_source ? value : std::nullopt;
	};
	auto inverter = [&](const std::optional<double> &v, const char *field) {
		const auto value = read.positive(v, field, Issue::MissingInverterData);
		return inverter_source ? value : std::nullopt;
	};
	const auto voc = mod(m.openCircuitVoltage, "openCircuitVoltage");
	const auto vmpp = mod(m.mppVoltage, "mppVoltage");
	const auto isc = mod(m.shortCircuitCurrent, "shortCircuitCurrent");
	const auto impp = mod(m.mppCurrent, "mppCurrent");
	const auto b_voc = coef(m.vocCoefficient, "vocCoefficient");
	const auto b_vmpp = coef(m.vmppCoefficient, "vmppCoefficient");
	const auto module_max = mod(m.maxSystemVoltage, "maxSystemVoltage");
	const auto series_fuse = mod(m.maxSeriesFuse, "maxSeriesFuse");
	const auto dc_max = inverter(inv.maxDcVoltage, "maxDcVoltage");
	const auto mppt_min = inverter(inv.mpptMinVoltage, "mpptMinVoltage");
	const auto mppt_max = inverter(inv.mpptMaxVoltage, "mpptMaxVoltage");
	const auto input_max = inverter(inv.maxInputCurrent, "maxInputCurrent");
	const auto input_isc = inverter(inv.maxShortCircuitCurrent, "maxShortCircuitCurrent");
	if (mppt_min && mppt_max && *mppt_min > *mppt_max)
		read.bad(Issue::OutOfRange, "mpptMinVoltage");

	const auto nm = read.count(in.modulesPerString, "modulesPerString");
	const auto ns = read.count(in.parallelStrings, "parallelStrings");

		//Temperatures of the site, with their source
	std::optional<double> t_min, t_max;
	{
		const auto lo = read.finite(in.minCellTemperature, "minCellTemperature",
									Issue::MissingCellTemperature);
		const auto hi = read.finite(in.maxCellTemperature, "maxCellTemperature",
									Issue::MissingCellTemperature);
		if (lo && hi && *lo >= *hi)
			read.bad(Issue::OutOfRange, "minCellTemperature");
		if ((lo || hi) && !justified(in.temperatureJustification))
			add(r.diagnostics, Issue::MissingJustification, "cellTemperature");
		else {
			t_min = lo;
			t_max = hi;
		}
	}

	std::optional<double> fuse;
	if (in.stringFuse)
		fuse = read.positive(in.stringFuse, "stringFuse", Issue::MissingStringProtection);

		//The DC cable
	std::optional<double> length, section, conductor_t, admissible;
	if (in.cableLength) length = read.positive(in.cableLength, "cableLength", Issue::MissingLength);
	else add(r.diagnostics, Issue::MissingLength, "cableLength");
	if (in.cableSection) section = read.positive(in.cableSection, "cableSection", Issue::MissingSection);
	else add(r.diagnostics, Issue::MissingSection, "cableSection");
	if (!in.cableMaterial) add(r.diagnostics, Issue::MissingMaterial, "cableMaterial");
	conductor_t = read.finite(in.conductorTemperature, "conductorTemperature",
							  Issue::MissingTemperature);
	if (in.cableAdmissible)
		admissible = read.positive(in.cableAdmissible, "cableAdmissible",
								   Issue::MissingAdmissibleCurrent);
	else
		add(r.diagnostics, Issue::MissingAdmissibleCurrent, "cableAdmissible");

	if (read.invalid()) {
		r.status = Status::InvalidInput;
		return r;
	}

		//Voltages
	r.rules.push_back(rules::pvOpenCircuitVoltage());
	r.rules.push_back(rules::pvMpptWindow());
	if (nm && voc && b_voc && t_min)
		r.openCircuitMax = *nm * *voc * (1.0 + *b_voc / 100.0 * (*t_min - 25.0));
	if (nm && vmpp && b_vmpp && t_max)
		r.mppMin = *nm * *vmpp * (1.0 + *b_vmpp / 100.0 * (*t_max - 25.0));
	if (nm && vmpp && b_vmpp && t_min)
		r.mppMax = *nm * *vmpp * (1.0 + *b_vmpp / 100.0 * (*t_min - 25.0));
	r.voltage = combine({atMost(r.openCircuitMax, dc_max), atMost(r.openCircuitMax, module_max)});
	r.mppt = combine({atMost(mppt_min, r.mppMin), atMost(r.mppMax, mppt_max)});

		//Input currents, at STC
	r.rules.push_back(rules::pvInputCurrents());
	if (ns && isc) r.inputShortCircuit = *ns * *isc;
	if (ns && impp) r.inputMppCurrent = *ns * *impp;
	if (r.inputShortCircuit || r.inputMppCurrent)
		assume(r.assumptions, Assumption::CurrentsAtStc);
	r.current = combine({atMost(r.inputShortCircuit, input_isc),
						 atMost(r.inputMppCurrent, input_max)});

		//String protection
	r.rules.push_back(rules::pvStringProtection());
	if (isc) {
		r.iscMax = PV_ISC_MAX_FACTOR * *isc;
		r.fuseMin = 1.5 * *isc;
		r.fuseMax = 2.4 * *isc;
		if (series_fuse) r.fuseMax = std::min(*r.fuseMax, *series_fuse);
	}
	if (ns && r.iscMax && series_fuse) {
		r.protectionRequired = (*ns - 1) * *r.iscMax > *series_fuse + TOLERANCE;
		assume(r.assumptions, Assumption::IscMaxFactor);
	}
	if (fuse && isc) {
			// 1.5 Isc < In < 2.4 Isc and In <= IMOD_MAX_OCPR
		const bool in_range = *fuse > 1.5 * *isc + TOLERANCE && *fuse + TOLERANCE < 2.4 * *isc
							  && (!series_fuse || *fuse <= *series_fuse + TOLERANCE);
		if (!in_range)
			r.protection = CheckOutcome::NotPassed;
		else if (series_fuse)
			r.protection = CheckOutcome::Passed;
	} else if (r.protectionRequired) {
		if (*r.protectionRequired) {
			add(r.diagnostics, Issue::MissingStringProtection, "stringFuse");
			r.protection = CheckOutcome::NotPassed;
		} else {
			r.protection = CheckOutcome::Passed;
		}
	}

		//String cable: table 712.2
	r.rules.push_back(rules::pvStringCable());
	if (fuse) {
		r.cableRequired = fuse;
	} else if (isc && ns) {
		if (*ns <= 2) assume(r.assumptions, Assumption::SingleStringCable);
		r.cableRequired = PV_ISC_MAX_FACTOR * *isc * std::max(1, *ns - 1);
	}
	r.cable = atMost(r.cableRequired, admissible);

		//DC voltage drop of the string cable
	r.rules.push_back(rules::pvDcVoltageDrop());
	if (length && section && in.cableMaterial && conductor_t && impp && vmpp && nm) {
		const double rho = resistivity(*in.cableMaterial, *conductor_t);
		const double du = 2.0 * *length * *impp * rho / *section;
		r.dropPercent = 100.0 * du / (*nm * *vmpp);
		r.drop = *r.dropPercent + TOLERANCE < PV_DC_DROP_LIMIT ? CheckOutcome::Passed
															   : CheckOutcome::NotPassed;
	}

	r.outcome = combine({r.voltage, r.mppt, r.current, r.protection, r.cable, r.drop});
	if (r.outcome != CheckOutcome::NotPerformed || r.openCircuitMax || r.dropPercent)
		r.status = Status::Ok;
	return r;
}

namespace rules {

NormRef pvOpenCircuitVoltage()
{
	return {
		"REBT-FV-001", "UNE-HD 60364-7-712", "712.512.1.1.102.1 (IEC 60364-7-712:2017 numbering)",
		"UNE-HD 60364-7-712:2017: equipment rated at least UOC MAX; limits of the inverter and "
		"the module from their datasheets",
		"UOC MAX = Nm Voc [1 + bVoc (Tmin - 25)] with the module's coefficient and the lowest "
		"cell temperature given; the method of Annex B of the standard is not applied. The "
		"standard is not among those of ITC-BT-02: good practice, not binding by the REBT."
	};
}

NormRef pvMpptWindow()
{
	return {
		"REBT-FV-002", "Inverter datasheet", "MPPT voltage range",
		"Manufacturer's data: not a requirement of the REBT",
		"UMPP of the string at the highest and lowest cell temperatures given, with the "
		"module's Vmpp coefficient, inside the MPPT range. Shading, mismatch and ageing are not "
		"considered."
	};
}

NormRef pvInputCurrents()
{
	return {
		"REBT-FV-003", "Inverter datasheet", "maximum input and short circuit current per MPPT",
		"Manufacturer's data: not a requirement of the REBT",
		"Ns Isc and Ns Impp at STC, no correction for temperature or irradiance above 1000 W/m2."
	};
}

NormRef pvStringProtection()
{
	return {
		"REBT-FV-004", "UNE-HD 60364-7-712", "712.430.3.103, 712.433.1.101.2 (IEC numbering)",
		"UNE-HD 60364-7-712:2017: needed when (Ns - 1) ISC MAX > IMOD_MAX_OCPR; "
		"1.5 ISC < In < 2.4 ISC, In <= IMOD_MAX_OCPR",
		"ISC MAX taken as 1.25 ISC STC, on the safe side (Annex B not applied). The grouped "
		"option of 712.433.1.101.2, sub-array and array protection, and the DC rating and "
		"type (gPV) of the device are not checked. Good practice, not binding by the REBT."
	};
}

NormRef pvStringCable()
{
	return {
		"REBT-FV-005", "UNE-HD 60364-7-712", "table 712.2 (IEC numbering)",
		"UNE-HD 60364-7-712:2017: string cable at least the In of its protection, or "
		"1.25 ISC MOD (Ns - 1) without it",
		"With one or two strings and no protection, at least 1.25 ISC (an interpretation: the "
		"table assumes a downstream device). Iz is the value given for the string cable. Good "
		"practice, not binding by the REBT."
	};
}

NormRef pvDcVoltageDrop()
{
	return {
		"REBT-FV-006", "IDAE PCT-C-REV", "5.5.2",
		"IDAE, Pliego de Condiciones Tecnicas de Instalaciones Conectadas a Red, julio 2011: "
		"\"inferior del 1,5 %\"",
		"String cable only, 2 L Impp rho / S over Nm Vmpp at STC, resistivity at the conductor "
		"temperature given. A recommendation of the IDAE, not a requirement of the REBT."
	};
}

NormRef pvAcLine()
{
	return {
		"REBT-FV-007", "ITC-BT-40", "5",
		"RD 842/2002 (BOE 18-09-2002): cables for at least 125 % of the maximum current of the "
		"generator; drop between the generator and the connection point at most 1.5 % at "
		"nominal current",
		"The generator taken as the inverter output: the AC line from it to the point of "
		"connection. The DC side is checked with REBT-FV-006."
	};
}

} // namespace rules
} // namespace rebt
