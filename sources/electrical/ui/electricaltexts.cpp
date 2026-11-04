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
#include "electricaltexts.h"

#include <algorithm>

#include <QStringList>

using namespace electrical;

QString ElectricalTexts::system(rebt::System system)
{
	switch (system) {
		case rebt::System::SinglePhase: return tr("Single phase");
		case rebt::System::ThreePhase: return tr("Three phase");
	}
	return QString();
}

QString ElectricalTexts::material(rebt::Material material)
{
	switch (material) {
		case rebt::Material::Copper: return tr("Copper");
		case rebt::Material::Aluminium: return tr("Aluminium");
	}
	return QString();
}

QString ElectricalTexts::use(CircuitUse use)
{
	switch (use) {
		case CircuitUse::DwellingInterior: return tr("Dwelling, interior circuit");
		case CircuitUse::Lighting: return tr("Lighting");
		case CircuitUse::OtherUses: return tr("Other uses");
		case CircuitUse::LgaTotallyCentralized:
			return tr("LGA, meters totally centralized");
		case CircuitUse::LgaPartlyCentralized:
			return tr("LGA, meters partly centralized");
		case CircuitUse::DiMetersInSeveralPlaces:
			return tr("DI, meters grouped in more than one place");
		case CircuitUse::DiMetersTotallyGrouped:
			return tr("DI, meters totally grouped");
		case CircuitUse::DiSingleUser:
			return tr("DI of a single user, no LGA");
	}
	return QString();
}

QString ElectricalTexts::insulation(Insulation insulation)
{
	switch (insulation) {
		case Insulation::PVC: return QStringLiteral("PVC");
		case Insulation::XLPE: return QStringLiteral("XLPE");
		case Insulation::EPR: return QStringLiteral("EPR");
	}
	return QString();
}

QString ElectricalTexts::protection(ProtectionType type)
{
	switch (type) {
		case ProtectionType::CircuitBreaker: return tr("Circuit breaker");
		case ProtectionType::ResidualCurrent: return tr("Residual current device");
		case ProtectionType::Rcbo: return tr("Circuit breaker with residual current");
		case ProtectionType::Fuse: return tr("Fuse");
		case ProtectionType::Switch: return tr("Switch");
	}
	return QString();
}

QString ElectricalTexts::field(const std::string &field)
{
	if (field == "system") return tr("system");
	if (field == "voltage") return tr("voltage");
	if (field == "power") return tr("power");
	if (field == "powerFactor") return tr("power factor");
	if (field == "efficiency") return tr("efficiency");
	if (field == "designCurrent") return tr("design current");
	if (field == "length") return tr("length");
	if (field == "section") return tr("section");
	if (field == "material") return tr("material");
	if (field == "conductorTemperature") return tr("conductor temperature");
	if (field == "conductivity") return tr("conductivity");
	if (field == "reactance") return tr("reactance");
	if (field == "insulation") return tr("insulation");
	if (field == "installationMethod") return tr("installation method");
	if (field == "loadedConductors") return tr("loaded conductors");
	if (field == "ambientTemperature") return tr("ambient temperature");
	if (field == "temperatureFactor") return tr("temperature factor");
	if (field == "groupingFactor") return tr("grouping factor");
	if (field == "otherFactor") return tr("other factor");
	if (field == "referenceAmbient") return tr("reference temperature of the table");
	if (field == "ratedCurrent") return tr("rated current of the protection");
	if (field == "protectionStandard") return tr("standard of the protection");
	if (field == "conventionalCurrent") return tr("I2 of the protection");
	if (field == "admissibleCurrent") return tr("admissible current IZ");
	if (field == "protectionType") return tr("type of protection");
	if (field == "neutralSection") return tr("neutral section");
	if (field == "source") return tr("supply data of the origin board");
	if (field == "sourceMaxCurrent") return tr("maximum Icc at the origin");
	if (field == "sourceMinCurrent") return tr("minimum Icc at the origin");
	if (field == "breakingCapacity") return tr("breaking capacity of the protection");
	if (field == "curve") return tr("curve of the protection (B, C, D)");
	if (field == "tripCurrent") return tr("instantaneous trip current of the protection");
	if (field == "letThrough") return tr("let-through energy I²t of the manufacturer");
	if (field == "letThroughAt") return tr("prospective current of the I²t");
	if (field == "earthing") return tr("earthing system of the installation");
	if (field == "earthResistance") return tr("earth resistance RA");
	if (field == "limitVoltage") return tr("limit voltage U");
	if (field == "operatingCurrent") return tr("operating current Ia of the device");
	if (field == "residualCurrent") return tr("IΔn of the residual current device");
	if (field == "residualDevice") return tr("residual current device of the circuit");
	if (field == "protectiveSection") return tr("protective conductor section");
	if (field == "firstFaultCurrent") return tr("first fault current Id");
	if (field == "massesInterconnected")
		return tr("whether the masses are interconnected or earthed apart");
	if (field == "neutralDistributed") return tr("whether the neutral is distributed");
	if (field == "feeders") return tr("lines from the CGP");
	if (field == "board") return tr("board of the circuit");
	if (field == "points") return tr("points of use");
	if (field == "dropAtRated") return tr("voltage drop with IN");
	if (field == "conduit") return tr("outer diameter of the conduit");
	if (field == "area") return tr("useful area of the dwelling");
	if (field == "heating") return tr("whether electric heating is planned");
	if (field == "airConditioning") return tr("whether air conditioning is planned");
	if (field == "automation") return tr("whether automation is planned");
	if (field == "vehicleCharging") return tr("whether EV charging is planned");
	if (field == "singleFamily") return tr("whether it is a single family dwelling");
	if (field == "generalSwitch") return tr("rated current of the general switch (IGA)");
	if (field == "individualLine") return tr("admissible current of the DI");
	if (field == "rooms") return tr("the rooms of the dwelling");
	if (field == "dryer") return tr("whether an independent dryer is planned");
	if (field == "residualDevice")
		return tr("the residual current device of some dwelling circuits");
	if (field.compare(0, 5, "room:") == 0) {
		const std::size_t slash = field.rfind('/');
		if (slash != std::string::npos && slash > 5)
			return tr("%1 of the room %2")
				.arg(roomField(field.substr(slash + 1)),
					 QString::fromStdString(field.substr(5, slash - 5)));
	}
	if (field.compare(0, 15, "residualDevice:") == 0)
		return tr("residual current device %1")
			.arg(QString::fromStdString(field.substr(15)));
	return QString::fromStdString(field);
}

QString ElectricalTexts::diagnostic(const rebt::Diagnostic &diagnostic)
{
	const QString name = field(diagnostic.field);
	switch (diagnostic.issue) {
		case rebt::Issue::MissingSystem:
		case rebt::Issue::MissingVoltage:
		case rebt::Issue::MissingPower:
		case rebt::Issue::MissingPowerFactor:
		case rebt::Issue::MissingLength:
		case rebt::Issue::MissingSection:
		case rebt::Issue::MissingMaterial:
		case rebt::Issue::MissingTemperature:
		case rebt::Issue::MissingInsulation:
		case rebt::Issue::MissingMethod:
		case rebt::Issue::MissingLoadedConductors:
		case rebt::Issue::MissingAmbient:
		case rebt::Issue::MissingGroupingFactor:
			return tr("Missing: %1").arg(name);
		case rebt::Issue::MissingJustification:
			return tr("%1 needs its justification").arg(name);
		case rebt::Issue::NoTable:
			return tr("No ampacity table loaded for this material, insulation, "
					  "loaded conductors and method");
		case rebt::Issue::SectionNotInTable:
			return tr("The section is not a row of the ampacity table");
		case rebt::Issue::MissingRatedCurrent:
		case rebt::Issue::MissingProtectionStandard:
		case rebt::Issue::MissingAdmissibleCurrent:
			return tr("Missing: %1").arg(name);
		case rebt::Issue::MissingConventionalCurrent:
			return tr("Missing: I2 of the manufacturer (gG fuse under 16 A)");
		case rebt::Issue::NoOverloadProtection:
			return tr("The device does not protect the cable against overload "
					  "(residual current device alone, switch)");
		case rebt::Issue::FeederLoop:
			return tr("The boards feed each other in a loop, at %1").arg(name);
		case rebt::Issue::FeederUseUnknown:
			return tr("The use of the feeder %1 is not known: interior or "
					  "service connection?").arg(name);
		case rebt::Issue::FeederNotComputable:
			return tr("The voltage drop of the feeder %1 cannot be computed").arg(name);
		case rebt::Issue::NotPositive:
			return tr("%1 must be greater than zero").arg(name);
		case rebt::Issue::NotFinite:
			return tr("%1 is not a number").arg(name);
		case rebt::Issue::OutOfRange:
			if (diagnostic.field == "sourceMinCurrent")
				return tr("The minimum Icc at the origin is greater than the maximum");
			return tr("%1 must be greater than 0 and at most 1").arg(name);
		case rebt::Issue::Negative:
			return tr("%1 must not be negative").arg(name);
		case rebt::Issue::NoShortCircuitProtection:
			return tr("The device does not break short circuits "
					  "(residual current device alone, switch)");
		case rebt::Issue::MissingSource:
			return tr("Missing: %1").arg(name);
		case rebt::Issue::MissingSourceCurrent:
		case rebt::Issue::MissingBreakingCapacity:
		case rebt::Issue::MissingTripCurrent:
		case rebt::Issue::MissingNeutralSection:
			return tr("Missing: %1").arg(name);
		case rebt::Issue::MissingLetThrough:
			return tr("Missing: %1 (the fault is cleared in under 0.1 s or the "
					  "device has no standard trip time)").arg(name);
		case rebt::Issue::NoSourceImpedance:
			return tr("0.8 U / R needs the lines from the CGP: this circuit "
					  "leaves from the origin board itself");
		case rebt::Issue::NoKFactor:
			return tr("No k factor for this conductor and insulation");
		case rebt::Issue::MissingPoints:
		case rebt::Issue::MissingConduit:
		case rebt::Issue::MissingDwellingData:
		case rebt::Issue::MissingGeneralSwitch:
		case rebt::Issue::MissingResidualDevice:
		case rebt::Issue::MissingIndividualLine:
			return tr("Missing: %1").arg(name);
		case rebt::Issue::MissingRoomType:
		case rebt::Issue::MissingRoomSize:
		case rebt::Issue::MissingRoomCount:
		case rebt::Issue::MissingIndividualDrop:
			return tr("Missing: %1").arg(name);
		case rebt::Issue::MissingSensitivity:
			return tr("Missing: sensitivity of %1").arg(name);
		case rebt::Issue::MissingEarthing:
		case rebt::Issue::MissingEarthResistance:
		case rebt::Issue::MissingOperatingCurrent:
		case rebt::Issue::MissingProtectiveSection:
		case rebt::Issue::MissingFirstFault:
		case rebt::Issue::MissingMassesArrangement:
		case rebt::Issue::MissingNeutralArrangement:
			return tr("Missing: %1").arg(name);
		case rebt::Issue::ResidualInTNC:
			return tr("Residual current device in TN-C: not allowed (ITC-BT-24 4.1.1)");
		case rebt::Issue::LimitAboveNormal:
			return tr("%1 over 50 V, the value of normal conditions").arg(name);
		case rebt::Issue::LetThroughBelowFault:
			return tr("I²t given at a current below the maximum Icc: it is not "
					  "valid at that fault, %1 needed at the maximum Icc").arg(name);
	}
	return name;
}

QString ElectricalTexts::assumption(rebt::Assumption assumption)
{
	switch (assumption) {
		case rebt::Assumption::EfficiencyNotApplied:
			return tr("No efficiency given: the power is taken as absorbed");
		case rebt::Assumption::CurrentGiven:
			return tr("Design current as given, not computed");
		case rebt::Assumption::ConductivityGiven:
			return tr("Conductivity as given");
		case rebt::Assumption::ResistivityFromMaterial:
			return tr("Resistivity from the material at the conductor temperature");
		case rebt::Assumption::ReactanceNeglected:
			return tr("No reactance given: neglected");
		case rebt::Assumption::LoadAtEnd:
			return tr("Whole load at the far end of the line");
		case rebt::Assumption::BalancedLoad:
			return tr("Balanced three phase load");
		case rebt::Assumption::LoadedConductorsFromSystem:
			return tr("Loaded conductors from the system: 2 single phase, 3 three phase");
		case rebt::Assumption::TemperatureFactorComputed:
			return tr("Temperature factor computed from the ambient temperature");
		case rebt::Assumption::ConventionalCurrentFromStandard:
			return tr("I2 from the standard of the device");
		case rebt::Assumption::OriginBoard:
			return tr("The first board fed by no circuit is the origin of the "
					  "interior installation");
		case rebt::Assumption::GuideSourceFactor:
			return tr("Network not known: 0.8 U at the CGP (Guía-BT Anexo 3)");
		case rebt::Assumption::SourceCurrentGiven:
			return tr("Icc at the origin as given, with its source");
		case rebt::Assumption::ResistanceAt20:
			return tr("Maximum Icc with the conductors at 20 °C");
		case rebt::Assumption::ResistanceAtFinalTemperature:
			return tr("Minimum Icc with the conductors at their temperature at the "
					  "end of the fault (160 °C PVC, 250 °C XLPE/EPR)");
		case rebt::Assumption::NeutralAsPhase:
			return tr("Neutral section taken equal to the phase");
		case rebt::Assumption::TripCurrentFromCurve:
			return tr("Instantaneous trip at the upper limit of the curve "
					  "(B 5, C 10, D 20 IN)");
		case rebt::Assumption::InstantaneousTripTime:
			return tr("No I²t given: the device clears in under 0.1 s and "
					  "(k S / Icc max)² is at least 0.1 s");
		case rebt::Assumption::DropAtRatedCurrent:
			return tr("Voltage drop with the rated current of the breaker, as "
					  "ITC-BT-25 3 asks");
		case rebt::Assumption::PowerFactorOne:
			return tr("No power factor given: cos φ = 1, as table B of Guía-BT-25");
		case rebt::Assumption::RoundedUp:
			return tr("Rounded up: the text does not say how");
		case rebt::Assumption::NormalLimitVoltage:
			return tr("U = 50 V, normal conditions (ITC-BT-24 4.1)");
		case rebt::Assumption::ResidualOperatingCurrent:
			return tr("Ia: IΔn of the residual current device (ITC-BT-24 4.1.2)");
		case rebt::Assumption::EarthResistanceGiven:
			return tr("RA as given: earth electrode plus protective conductors, "
					  "not worked out");
		case rebt::Assumption::GuideLoop:
			return tr("Supply not known: Zs = R / 0.8 from the CGP (Guía-BT Anexo 3)");
		case rebt::Assumption::LoopFromItOrigin:
			return tr("IT: Zs of the lines from the board where IT is given, "
					  "feeders included, no source");
		case rebt::Assumption::InstantaneousWithinTable:
			return tr("Ia clears in under 0.1 s: within every time of the tables "
					  "of ITC-BT-24");
		case rebt::Assumption::CompensatedWithIndividual:
			return tr("Voltage drop compensated with the DI (ITC-BT-19 2.2.2)");
		case rebt::Assumption::OwnTransformerOrigin:
			return tr("Origin at the output of the own transformer "
					  "(ITC-BT-19 2.2.2)");
	}
	return QString();
}

QString ElectricalTexts::rule(const rebt::NormRef &rule)
{
	QString text = QString::fromStdString(rule.id) + QLatin1String(" - ")
				   + QString::fromStdString(rule.source);
	if (rule.section != "-")
		text += QLatin1Char(' ') + QString::fromStdString(rule.section);
	return text;
}

QString ElectricalTexts::outcome(const CircuitEvaluation &evaluation)
{
	if (!evaluation.limit)
		return tr("No limit", "voltage drop check: the circuit use is not known");
	switch (evaluation.check.outcome) {
		case rebt::CheckOutcome::Passed:
			return tr("Passed");
		case rebt::CheckOutcome::NotPassed:
			return tr("Not passed");
		case rebt::CheckOutcome::NotPerformed:
			return tr("Not performed");
	}
	return QString();
}

QString ElectricalTexts::details(const CircuitEvaluation &evaluation)
{
	QStringList lines;
	for (const rebt::Diagnostic &d : evaluation.drop.diagnostics)
		lines << diagnostic(d);

	if (evaluation.limit) {
		const rebt::VoltageDropLimit &limit = *evaluation.limit;
		lines << tr("Limit: voltage drop %1 %2 % (%3)")
				 .arg(limit.strict ? QStringLiteral("<") : QStringLiteral("<="))
				 .arg(formatNumber(limit.percent))
				 .arg(rule(limit.rule));
	} else {
		lines << tr("No limit: the use of the circuit is not known");
	}

	for (rebt::Assumption a : evaluation.drop.assumptions)
		lines << QStringLiteral("- ") + assumption(a);

	if (evaluation.chainEvaluated) {
		for (const rebt::Diagnostic &d : evaluation.chainDiagnostics)
			lines << diagnostic(d);
		if (evaluation.cumulativePercent && !evaluation.feeders.isEmpty())
			lines << tr("From the origin: %1 % (this circuit %2 % + %n feeder(s))",
						"", evaluation.feeders.size())
					 .arg(formatNumber(evaluation.cumulativePercent, 2),
						  formatNumber(evaluation.drop.percent, 2));
		else if (evaluation.limit && !evaluation.cumulativePercent)
			lines << tr("The drop from the origin cannot be worked out: the check "
						"is not complete");
		if (evaluation.compensatedPercent && evaluation.individualPercent)
			lines << tr("Compensated with the DI: %1 % + DI %2 % = %3 %")
					 .arg(formatNumber(evaluation.cumulativePercent, 2),
						  formatNumber(evaluation.individualPercent, 2),
						  formatNumber(evaluation.compensatedPercent, 2));
		else if (evaluation.compensated && evaluation.cumulativePercent)
			lines << tr("Compensated with the DI, whose drop is not known: "
						"not performed");
	} else if (evaluation.limit) {
		lines << tr("Partial check: only this circuit, not the drop "
					"accumulated from the origin of the installation");
	}
	QStringList rules;
	for (const rebt::NormRef &r : evaluation.drop.rules)
		rules << rule(r);
	if (!rules.isEmpty())
		lines << tr("Rules: %1").arg(rules.join(QStringLiteral("; ")));
	return lines.join(QLatin1Char('\n'));
}

QString ElectricalTexts::factor(rebt::FactorKind kind)
{
	switch (kind) {
		case rebt::FactorKind::Temperature: return tr("Temperature factor");
		case rebt::FactorKind::Grouping: return tr("Grouping factor");
		case rebt::FactorKind::Other: return tr("Other factor");
	}
	return QString();
}

QString ElectricalTexts::ampacityOutcome(const CircuitEvaluation &evaluation)
{
	switch (evaluation.ampacityCheck.outcome) {
		case rebt::CheckOutcome::Passed: return tr("Passed");
		case rebt::CheckOutcome::NotPassed: return tr("Not passed");
		case rebt::CheckOutcome::NotPerformed: return tr("Not performed");
	}
	return QString();
}

QString ElectricalTexts::ampacityDetails(const CircuitEvaluation &evaluation)
{
	const rebt::AmpacityResult &a = evaluation.ampacity;
	QStringList lines;
	for (const rebt::Diagnostic &d : a.diagnostics)
		lines << diagnostic(d);
	if (a.status == rebt::Status::Ok) {
		lines << tr("Table: %1 (%2): %3 A")
				 .arg(QString::fromStdString(a.tableId),
					  QString::fromStdString(a.tableSource),
					  formatNumber(a.tableCurrent, 2));
		for (const rebt::CorrectionFactor &f : a.factors) {
			QString line = QStringLiteral("× %1 %2").arg(factor(f.kind),
														formatNumber(f.value, 3));
			if (f.computed)
				line += QLatin1String(" (") + tr("computed") + QLatin1Char(')');
			else if (!f.justification.empty())
				line += QLatin1String(": ") + QString::fromStdString(f.justification);
			lines << line;
		}
		lines << tr("IZ = %1 A").arg(formatNumber(a.admissible, 2));
	}
	for (rebt::Assumption assumption_ : a.assumptions)
		lines << QStringLiteral("- ") + assumption(assumption_);
	QStringList rules;
	for (const rebt::NormRef &r : a.rules)
		rules << rule(r);
	lines << tr("Rules: %1").arg(rules.join(QStringLiteral("; ")));
	return lines.join(QLatin1Char('\n'));
}

QString ElectricalTexts::suggestion(const CircuitEvaluation &evaluation)
{
	const rebt::SectionSuggestion &s = evaluation.suggestion;
	if (s.status != rebt::Status::Ok)
		return tr("Suggested section: not computable");
	if (s.noneEnough)
		return tr("Suggested section: no section of the table is enough");
	QString text = tr("Suggested section: %1 mm² (IZ %2 A)")
				   .arg(formatNumber(s.section), formatNumber(s.admissible, 2));
	if (s.dropConsidered)
		text += QLatin1Char(' ') + tr("with the voltage drop within its limit");
	else
		text += QLatin1Char(' ') + tr("by admissible current only: the voltage "
									  "drop was not taken into account");
	return text;
}

QString ElectricalTexts::standard(rebt::ProtectionStandard standard)
{
	switch (standard) {
		case rebt::ProtectionStandard::EN60898:
			return tr("UNE-EN 60898 (circuit breaker, household)");
		case rebt::ProtectionStandard::EN61009:
			return tr("UNE-EN 61009 (RCBO, household)");
		case rebt::ProtectionStandard::EN60947_2:
			return tr("UNE-EN 60947-2 (circuit breaker, industrial)");
		case rebt::ProtectionStandard::FuseGG:
			return tr("UNE-EN 60269 gG fuse");
	}
	return QString();
}

QString ElectricalTexts::earthing(rebt::EarthingSystem earthing)
{
	switch (earthing) {
		case rebt::EarthingSystem::TT: return tr("TT");
		case rebt::EarthingSystem::TN: return tr("TN-S or TN-C-S");
		case rebt::EarthingSystem::TNC: return tr("TN-C");
		case rebt::EarthingSystem::IT: return tr("IT");
	}
	return QString();
}

QString ElectricalTexts::indirectOutcome(const CircuitEvaluation &evaluation)
{
	const rebt::IndirectContactResult &r = evaluation.indirect;
	switch (r.outcome) {
		case rebt::CheckOutcome::Passed: return tr("Passed");
		case rebt::CheckOutcome::NotPassed: return tr("Not passed");
		case rebt::CheckOutcome::NotPerformed: return tr("Not performed");
	}
	return QString();
}

QString ElectricalTexts::indirectDetails(const CircuitEvaluation &evaluation)
{
	const rebt::IndirectContactResult &r = evaluation.indirect;
	QStringList lines;
	for (const rebt::Diagnostic &d : r.diagnostics) {
		QString text = diagnostic(d);
		if (d.field.compare(0, 7, "feeder[") == 0)
			text.replace(QString::fromStdString(d.field),
						 faultField(d.field, evaluation.faultFeederNames));
		lines << text;
	}
	if (!evaluation.earthResistanceReason.isEmpty())
		lines << tr("RA as given: %1").arg(evaluation.earthResistanceReason);
	if (r.operatingCurrent) {
		const bool residual = std::find(r.assumptions.begin(), r.assumptions.end(),
										rebt::Assumption::ResidualOperatingCurrent)
							  != r.assumptions.end();
		lines << (residual
					  ? (evaluation.residualDeviceName.isEmpty()
							 ? tr("Ia = IΔn = %1 mA").arg(formatNumber(*r.operatingCurrent * 1000.0, 0))
							 : tr("Ia = IΔn of %1 = %2 mA")
								   .arg(evaluation.residualDeviceName,
										formatNumber(*r.operatingCurrent * 1000.0, 0)))
					  : tr("Ia = instantaneous trip = %1 A").arg(formatNumber(r.operatingCurrent, 0)));
	}
	if (r.limitVoltage)
		lines << (evaluation.limitVoltageReason.isEmpty()
					  ? tr("U = %1 V").arg(formatNumber(r.limitVoltage, 0))
					  : tr("U = %1 V (%2)").arg(formatNumber(r.limitVoltage, 0),
												evaluation.limitVoltageReason));
	auto word = [](rebt::CheckOutcome o) {
		switch (o) {
			case rebt::CheckOutcome::Passed: return tr("passed");
			case rebt::CheckOutcome::NotPassed: return tr("not passed");
			case rebt::CheckOutcome::NotPerformed: return tr("not performed");
		}
		return QString();
	};
	const bool it = std::any_of(r.rules.begin(), r.rules.end(), [](const rebt::NormRef &n) {
		return n.id == "REBT-CIN-003";
	});
	if (it) {
		if (!evaluation.firstFaultReason.isEmpty())
			lines << tr("Id as given: %1").arg(evaluation.firstFaultReason);
		if (r.firstFaultVoltage)
			lines << tr("First fault: RA × Id = %1 V").arg(formatNumber(r.firstFaultVoltage, 2));
		lines << tr("First fault, RA × Id ≤ U: %1").arg(word(r.firstFault));
	}
	if (r.maxResistance)
		lines << tr("Largest RA: U / Ia = %1 Ω").arg(formatNumber(r.maxResistance, 2));
	if (r.contactVoltage)
		lines << tr("RA × Ia = %1 V; RA × Ia ≤ U: %2")
					 .arg(formatNumber(r.contactVoltage, 2),
						  word(it ? r.secondFault : r.outcome));
	if (r.loopLimit)
		lines << tr("Limit of the loop: %1 V").arg(formatNumber(r.loopLimit, 0));
	if (r.loopImpedance)
		lines << tr("Zs = %1 Ω").arg(formatNumber(r.loopImpedance, 4));
	if (r.maxImpedance)
		lines << (it ? tr("Largest Zs: U / 2 Ia = %1 Ω").arg(formatNumber(r.maxImpedance, 4))
					 : tr("Largest Zs: U0 / Ia = %1 Ω").arg(formatNumber(r.maxImpedance, 4)));
	if (r.loopVoltage)
		lines << (it ? tr("2 Zs × Ia = %1 V; 2 Zs × Ia ≤ U: %2").arg(formatNumber(r.loopVoltage, 2),
													  word(r.secondFault))
					 : tr("Zs × Ia = %1 V; Zs × Ia ≤ U0: %2").arg(formatNumber(r.loopVoltage, 2),
														 word(r.outcome)));
	for (rebt::Assumption a : r.assumptions)
		lines << QStringLiteral("- ") + assumption(a);
	QStringList rules;
	for (const rebt::NormRef &n : r.rules)
		rules << rule(n);
	if (!rules.isEmpty())
		lines << tr("Rules: %1").arg(rules.join(QStringLiteral("; ")));
	return lines.join(QLatin1Char('\n'));
}

QString ElectricalTexts::coordinationOutcome(const CircuitEvaluation &evaluation)
{
	switch (evaluation.coordination.outcome) {
		case rebt::CheckOutcome::Passed: return tr("Passed");
		case rebt::CheckOutcome::NotPassed: return tr("Not passed");
		case rebt::CheckOutcome::NotPerformed: return tr("Not performed");
	}
	return QString();
}

QString ElectricalTexts::coordinationDetails(const CircuitEvaluation &evaluation)
{
	const rebt::CoordinationResult &c = evaluation.coordination;
	auto word = [](rebt::CheckOutcome o) {
		switch (o) {
			case rebt::CheckOutcome::Passed: return tr("passed");
			case rebt::CheckOutcome::NotPassed: return tr("not passed");
			case rebt::CheckOutcome::NotPerformed: return tr("not performed");
		}
		return QString();
	};
	QStringList lines;
	for (const rebt::Diagnostic &d : c.diagnostics)
		lines << diagnostic(d);
	lines << tr("IB ≤ IN: %1").arg(word(c.ibIn));
	lines << tr("IN ≤ IZ: %1").arg(word(c.inIz));
	lines << tr("I2 ≤ 1.45 IZ: %1").arg(word(c.i2Iz));
	if (c.conventionalCurrent)
		lines << tr("I2 = %1 A").arg(formatNumber(c.conventionalCurrent, 2));
	for (rebt::Assumption a : c.assumptions)
		lines << QStringLiteral("- ") + assumption(a);
	QStringList rules;
	for (const rebt::NormRef &r : c.rules)
		rules << rule(r);
	lines << tr("Rules: %1").arg(rules.join(QStringLiteral("; ")));
	return lines.join(QLatin1Char('\n'));
}

QString ElectricalTexts::source(rebt::SourceMethod method)
{
	switch (method) {
		case rebt::SourceMethod::GuideFactor:
			return tr("Network not known, this board is the CGP (0.8 U / R)");
		case rebt::SourceMethod::KnownCurrent:
			return tr("Icc at this board known");
	}
	return QString();
}

QString ElectricalTexts::shortCircuitOutcome(const CircuitEvaluation &evaluation)
{
	switch (evaluation.shortCircuit.outcome) {
		case rebt::CheckOutcome::Passed: return tr("Passed");
		case rebt::CheckOutcome::NotPassed: return tr("Not passed");
		case rebt::CheckOutcome::NotPerformed: return tr("Not performed");
	}
	return QString();
}

QString ElectricalTexts::faultField(const std::string &field, const QStringList &feeders)
{
	static const std::string prefix = "feeder[";
	if (field.compare(0, prefix.size(), prefix) == 0) {
		const std::size_t close = field.find("]/");
		if (close != std::string::npos) {
			bool ok = false;
			const int i = QString::fromStdString(
				field.substr(prefix.size(), close - prefix.size())).toInt(&ok);
			const QString name = ok && i >= 0 && i < feeders.size()
								 ? feeders.at(i) : QString::number(i + 1);
			return tr("%1 of the feeder %2")
				.arg(ElectricalTexts::field(field.substr(close + 2)), name);
		}
	}
	return ElectricalTexts::field(field);
}

QString ElectricalTexts::shortCircuitDetails(const CircuitEvaluation &evaluation)
{
	const rebt::ShortCircuitResult &s = evaluation.shortCircuit;
	auto word = [](rebt::CheckOutcome o) {
		switch (o) {
			case rebt::CheckOutcome::Passed: return tr("passed");
			case rebt::CheckOutcome::NotPassed: return tr("not passed");
			case rebt::CheckOutcome::NotPerformed: return tr("not performed");
		}
		return QString();
	};
	auto kilo = [](const std::optional<double> &a) {
		return a ? formatNumber(*a / 1000.0, 2) : QStringLiteral("—");
	};
	QStringList lines;
	for (const rebt::Diagnostic &d : evaluation.faultDiagnostics)
		lines << diagnostic(d);
	for (const rebt::Diagnostic &d : s.diagnostics) {
		QString text = diagnostic(d);
			// field() leaves "feeder[1]/length" as it is: put the name in
		if (d.field.compare(0, 7, "feeder[") == 0)
			text.replace(QString::fromStdString(d.field),
						 faultField(d.field, evaluation.faultFeederNames));
		lines << text;
	}
	if (!evaluation.sourceText.isEmpty())
		lines << tr("Icc at the origin, maximum / minimum: %1").arg(evaluation.sourceText);
	lines << tr("Icc max at the board: %1 kA").arg(kilo(s.maxCurrent));
	lines << tr("Icc min at the far end: %1 kA").arg(kilo(s.minCurrent));
	lines << tr("Breaking capacity ≥ Icc max: %1").arg(word(s.breaking));
	if (s.tripCurrent)
		lines << tr("Icc min ≥ instantaneous trip %1 A: %2")
					 .arg(formatNumber(s.tripCurrent, 0), word(s.trip));
	if (s.tripCurrent && !evaluation.tripCurrentReason.isEmpty()
		&& std::find(s.assumptions.begin(), s.assumptions.end(),
					 rebt::Assumption::TripCurrentFromCurve) == s.assumptions.end())
		lines << tr("Instantaneous trip as given: %1").arg(evaluation.tripCurrentReason);
	else
		lines << tr("Icc min ≥ instantaneous trip: %1").arg(word(s.trip));
	if (s.k)
		lines << tr("k = %1, k²S² = %2 A²s").arg(formatNumber(s.k, 0),
												  formatNumber(s.conductorEnergy, 0));
	if (s.allowedTime)
		lines << tr("(k S / Icc max)² = %1 s").arg(formatNumber(s.allowedTime, 3));
	if (s.letThrough)
		lines << tr("I²t of the manufacturer: %1 A²s (%2)")
					 .arg(formatNumber(s.letThrough, 0), evaluation.letThroughReason);
	lines << tr("I²t ≤ k²S²: %1").arg(word(s.thermal));
	for (rebt::Assumption a : s.assumptions)
		lines << QStringLiteral("- ") + assumption(a);
	QStringList rules;
	for (const rebt::NormRef &r : s.rules)
		rules << rule(r);
	lines << tr("Rules: %1").arg(rules.join(QStringLiteral("; ")));
	return lines.join(QLatin1Char('\n'));
}

QString ElectricalTexts::dwellingCode(rebt::DwellingCircuit circuit)
{
	switch (circuit) {
		case rebt::DwellingCircuit::C1: return QStringLiteral("C1");
		case rebt::DwellingCircuit::C2: return QStringLiteral("C2");
		case rebt::DwellingCircuit::C3: return QStringLiteral("C3");
		case rebt::DwellingCircuit::C4: return QStringLiteral("C4");
		case rebt::DwellingCircuit::C4Single: return QStringLiteral("C4.x");
		case rebt::DwellingCircuit::C5: return QStringLiteral("C5");
		case rebt::DwellingCircuit::C6: return QStringLiteral("C6");
		case rebt::DwellingCircuit::C7: return QStringLiteral("C7");
		case rebt::DwellingCircuit::C8: return QStringLiteral("C8");
		case rebt::DwellingCircuit::C9: return QStringLiteral("C9");
		case rebt::DwellingCircuit::C10: return QStringLiteral("C10");
		case rebt::DwellingCircuit::C11: return QStringLiteral("C11");
		case rebt::DwellingCircuit::C12C3: return QStringLiteral("C12 (C3)");
		case rebt::DwellingCircuit::C12C4: return QStringLiteral("C12 (C4)");
		case rebt::DwellingCircuit::C12C5: return QStringLiteral("C12 (C5)");
		case rebt::DwellingCircuit::C13: return QStringLiteral("C13");
	}
	return QString();
}

QString ElectricalTexts::dwellingCircuit(rebt::DwellingCircuit circuit)
{
	const QString code = dwellingCode(circuit) + QLatin1Char(' ');
	switch (circuit) {
		case rebt::DwellingCircuit::C1: return code + tr("Lighting");
		case rebt::DwellingCircuit::C2: return code + tr("General use sockets and fridge");
		case rebt::DwellingCircuit::C3: return code + tr("Cooker and oven");
		case rebt::DwellingCircuit::C4:
			return code + tr("Washing machine, dishwasher and water heater");
		case rebt::DwellingCircuit::C4Single:
			return code + tr("One appliance of C4, its own 16 A circuit (note 8)");
		case rebt::DwellingCircuit::C5: return code + tr("Bathroom and kitchen worktop");
		case rebt::DwellingCircuit::C6: return code + tr("Additional lighting, per 30 points");
		case rebt::DwellingCircuit::C7: return code + tr("Additional general use sockets");
		case rebt::DwellingCircuit::C8: return code + tr("Electric heating");
		case rebt::DwellingCircuit::C9: return code + tr("Air conditioning");
		case rebt::DwellingCircuit::C10: return code + tr("Dryer");
		case rebt::DwellingCircuit::C11: return code + tr("Home automation");
		case rebt::DwellingCircuit::C12C3:
		case rebt::DwellingCircuit::C12C4:
		case rebt::DwellingCircuit::C12C5: return code + tr("Additional circuit");
		case rebt::DwellingCircuit::C13: return code + tr("Electric vehicle charging");
	}
	return code;
}

QString ElectricalTexts::dwellingOutcome(const CircuitEvaluation &evaluation)
{
	if (!evaluation.dwelling.spec.breaker && !evaluation.dwelling.spec.section
		&& !evaluation.dwelling.spec.points)
		return QString();   // not a dwelling circuit
	switch (evaluation.dwelling.outcome) {
		case rebt::CheckOutcome::Passed: return tr("Passed");
		case rebt::CheckOutcome::NotPassed: return tr("Not passed");
		case rebt::CheckOutcome::NotPerformed: return tr("Not performed");
	}
	return QString();
}

QString ElectricalTexts::dwellingDetails(const CircuitEvaluation &evaluation)
{
	const rebt::DwellingResult &d = evaluation.dwelling;
	auto word = [](rebt::CheckOutcome o) {
		switch (o) {
			case rebt::CheckOutcome::Passed: return tr("passed");
			case rebt::CheckOutcome::NotPassed: return tr("not passed");
			case rebt::CheckOutcome::NotPerformed: return tr("not performed");
		}
		return QString();
	};
	QStringList lines;
	for (const rebt::Diagnostic &x : d.diagnostics)
		lines << diagnostic(x);
	if (d.spec.breaker)
		lines << tr("Breaker ≤ %1 A: %2").arg(formatNumber(d.spec.breaker), word(d.breaker));
	lines << tr("Copper conductors: %1").arg(word(d.copper));
	if (d.spec.section)
		lines << tr("Section ≥ %1 mm²: %2").arg(formatNumber(d.spec.section), word(d.section));
	if (d.spec.points)
		lines << tr("Points ≤ %1: %2").arg(d.spec.points.value()).arg(word(d.points));
	if (d.spec.power)
		lines << tr("Power ≤ %1 W: %2").arg(formatNumber(d.spec.power, 0), word(d.power));
	if (d.spec.conduit)
		lines << tr("Conduit ≥ %1 mm: %2").arg(formatNumber(d.spec.conduit), word(d.conduit));
	if (evaluation.cumulativeAtRated)
		lines << tr("ΔU with IN from the origin ≤ 3 %: %1 % (%2)")
					 .arg(formatNumber(evaluation.cumulativeAtRated, 2), word(d.drop));
	else
		lines << tr("ΔU with IN from the origin ≤ 3 %: %1").arg(word(d.drop));
	for (rebt::Assumption a : evaluation.dropAtRated.assumptions)
		lines << QStringLiteral("- ") + assumption(a);
	QStringList rules;
	for (const rebt::NormRef &r : d.rules)
		rules << rule(r);
	lines << tr("Rules: %1").arg(rules.join(QStringLiteral("; ")));
	return lines.join(QLatin1Char('\n'));
}

QString ElectricalTexts::minimumCircuitsDetails(const rebt::MinimumCircuitsResult &result)
{
	switch (result.outcome) {
		case rebt::CheckOutcome::Passed:
			return tr("Basic electrification: C1 to C5 present (%1)")
				.arg(rule(result.rule));
		case rebt::CheckOutcome::NotPassed: {
			QStringList missing;
			for (rebt::DwellingCircuit c : result.missing)
				missing << dwellingCode(c);
			return tr("Basic electrification: missing %1 (%2)")
				.arg(missing.join(QStringLiteral(", ")), rule(result.rule));
		}
		case rebt::CheckOutcome::NotPerformed:
			break;
	}
	return QString();
}

QString ElectricalTexts::elevatedReason(rebt::ElevatedReason reason)
{
	switch (reason) {
		case rebt::ElevatedReason::Area: return tr("useful area over 160 m²");
		case rebt::ElevatedReason::Heating: return tr("electric heating planned");
		case rebt::ElevatedReason::AirConditioning: return tr("air conditioning planned");
		case rebt::ElevatedReason::Automation: return tr("automation planned");
		case rebt::ElevatedReason::VehicleCharging:
			return tr("EV charging in a single family dwelling");
		case rebt::ElevatedReason::ExtraCircuit:
			return tr("circuits beyond the basic ones");
	}
	return QString();
}

QString ElectricalTexts::dwellingCheckOutcome(const rebt::DwellingCheck &check)
{
	QString grade = tr("Dwelling");
	if (check.grade)
		grade = *check.grade == rebt::Electrification::Basic ? tr("Basic") : tr("Elevated");
	switch (check.outcome) {
		case rebt::CheckOutcome::Passed: return tr("%1: passed").arg(grade);
		case rebt::CheckOutcome::NotPassed: return tr("%1: not passed").arg(grade);
		case rebt::CheckOutcome::NotPerformed: return tr("%1: not performed").arg(grade);
	}
	return QString();
}

QString ElectricalTexts::dwellingCheckDetails(const rebt::DwellingCheck &c)
{
	auto word = [](rebt::CheckOutcome o) {
		switch (o) {
			case rebt::CheckOutcome::Passed: return tr("passed");
			case rebt::CheckOutcome::NotPassed: return tr("not passed");
			case rebt::CheckOutcome::NotPerformed: return tr("not performed");
		}
		return QString();
	};
	QStringList lines;
	if (!c.grade) {
		lines << tr("Grade of electrification: not known");
	} else if (*c.grade == rebt::Electrification::Basic) {
		lines << tr("Grade of electrification: basic");
	} else {
		QStringList why;
		for (rebt::ElevatedReason r : c.reasons)
			why << elevatedReason(r);
		lines << tr("Grade of electrification: elevated (%1)").arg(why.join(QStringLiteral(", ")));
	}
	for (const rebt::Diagnostic &d : c.diagnostics)
		lines << diagnostic(d);
	QStringList missing;
	for (rebt::DwellingCircuit m : c.missing)
		missing << dwellingCode(m);
	if (c.rooms.empty())
		lines << tr("Points of use of the rooms: %1").arg(word(c.roomPoints));
	for (const rebt::RoomResult &room : c.rooms) {
		QStringList items;
		for (const rebt::RoomRequirement &q : room.items) {
			const QString present = q.present ? QString::number(*q.present)
											  : QStringLiteral("?");
			const QString required = q.required ? QString::number(*q.required)
												: QStringLiteral("?");
			items << tr("%1 %2/%3").arg(roomItem(q.item), present, required);
		}
		lines << tr("Room %1: %2 (%3)").arg(QString::fromStdString(room.name),
											word(room.outcome),
											items.join(QStringLiteral(", ")));
	}
	lines << tr("C1 to C5: %1").arg(word(c.minimumCircuits));
	lines << tr("Circuits for what is planned: %1").arg(word(c.plannedCircuits));
	if (!missing.isEmpty())
		lines << tr("Missing circuits: %1").arg(missing.join(QStringLiteral(", ")));
	lines << tr("General switch ≥ %1 A: %2").arg(formatNumber(c.minimumSwitch),
												  word(c.generalSwitch));
	lines << tr("IZ of the DI ≥ IGA: %1").arg(word(c.individualLine));
	lines << tr("Residual current devices ≥ %1 (one per five circuits): %2")
				 .arg(c.residualNeeded).arg(word(c.residualCount));
	lines << tr("Every circuit at 30 mA at most: %1").arg(word(c.residualSensitivity));
	lines << tr("Rated current of the devices ≥ IGA: %1").arg(word(c.residualRating));
	if (c.vehicleCircuit)
		lines << tr("C13 on a device of its own: %1").arg(word(c.vehicleExclusive));
	QStringList rules;
	for (const rebt::NormRef &r : c.rules)
		rules << rule(r);
	lines << tr("Rules: %1").arg(rules.join(QStringLiteral("; ")));
	return lines.join(QLatin1Char('\n'));
}

QString ElectricalTexts::roomType(rebt::RoomType type)
{
	switch (type) {
		case rebt::RoomType::Access: return tr("Access");
		case rebt::RoomType::Hall: return tr("Hall");
		case rebt::RoomType::LivingRoom: return tr("Living room");
		case rebt::RoomType::Bedroom: return tr("Bedroom");
		case rebt::RoomType::Bathroom: return tr("Bathroom");
		case rebt::RoomType::Corridor: return tr("Corridor or landing");
		case rebt::RoomType::Kitchen: return tr("Kitchen");
		case rebt::RoomType::TerraceOrDressing: return tr("Terrace or dressing room");
		case rebt::RoomType::GarageOrOther: return tr("Garage or other");
	}
	return QString();
}

QString ElectricalTexts::roomItem(rebt::RoomItem item)
{
	switch (item) {
		case rebt::RoomItem::Bell: return tr("bell push");
		case rebt::RoomItem::LightPoints: return tr("light points");
		case rebt::RoomItem::Switches: return tr("switches");
		case rebt::RoomItem::Sockets: return tr("C2 sockets");
		case rebt::RoomItem::Cooker: return tr("C3 25 A socket");
		case rebt::RoomItem::ApplianceSockets: return tr("C4 sockets");
		case rebt::RoomItem::WetSockets: return tr("C5 sockets");
		case rebt::RoomItem::Heating: return tr("heating outlets");
		case rebt::RoomItem::AirConditioning: return tr("air conditioning outlets");
		case rebt::RoomItem::Dryer: return tr("C10 dryer socket");
		case rebt::RoomItem::VehicleCharging: return tr("C13 charging socket");
	}
	return QString();
}

QString ElectricalTexts::roomField(const std::string &field)
{
	if (field == "type") return tr("type");
	if (field == "area") return tr("area");
	if (field == "length") return tr("length");
	if (field == "accesses") return tr("accesses");
	for (rebt::RoomItem item : {rebt::RoomItem::Bell, rebt::RoomItem::LightPoints,
								rebt::RoomItem::Switches, rebt::RoomItem::Sockets,
								rebt::RoomItem::Cooker, rebt::RoomItem::ApplianceSockets,
								rebt::RoomItem::WetSockets, rebt::RoomItem::Heating,
								rebt::RoomItem::AirConditioning, rebt::RoomItem::Dryer,
								rebt::RoomItem::VehicleCharging})
		if (field == rebt::roomItemField(item))
			return roomItem(item);
	return QString::fromStdString(field);
}
