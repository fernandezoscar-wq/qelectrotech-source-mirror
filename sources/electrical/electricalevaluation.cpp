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
#include "electricalevaluation.h"

#include <QLocale>
#include <QMap>
#include <QSet>

#include <algorithm>
#include <cmath>

namespace electrical {

std::optional<double> CircuitEvaluation::designCurrent() const
{
	if (drop.current)
		return drop.current;
	return current.current;
}

namespace {

rebt::DwellingInput dwellingInput(const Circuit &circuit)
{
	rebt::DwellingInput input;
	input.circuit = circuit.dwellingCircuit;
	input.ratedCurrent = circuit.protection.ratedCurrent;
	input.section = circuit.conductor.section;
	input.material = circuit.conductor.material;
	input.points = circuit.points;
	input.power = circuit.load.power;
	input.conduit = circuit.conductor.conduit;
	return input;
}

} // namespace

std::optional<rebt::VoltageDropLimit> limitFor(const Circuit &circuit)
{
	if (!circuit.use)
		return std::nullopt;
	switch (*circuit.use) {
		case CircuitUse::DwellingInterior:
			return rebt::rules::dwellingInterior();
		case CircuitUse::Lighting:
			return rebt::rules::otherInteriorLighting();
		case CircuitUse::OtherUses:
			return rebt::rules::otherInteriorOtherUses();
		case CircuitUse::LgaTotallyCentralized:
			return rebt::rules::lgaTotallyCentralized();
		case CircuitUse::LgaPartlyCentralized:
			return rebt::rules::lgaPartlyCentralized();
		case CircuitUse::DiMetersInSeveralPlaces:
			return rebt::rules::diMetersInSeveralPlaces();
		case CircuitUse::DiMetersTotallyGrouped:
			return rebt::rules::diMetersTotallyGrouped();
		case CircuitUse::DiSingleUser:
			return rebt::rules::diSingleUser();
		case CircuitUse::GeneratorLine:
			return rebt::VoltageDropLimit{GENERATOR_DROP_LIMIT, false, rebt::rules::pvAcLine()};
	}
	return std::nullopt;
}

CircuitEvaluation evaluate(const Circuit &circuit, const rebt::AmpacityLibrary &tables)
{
	CircuitEvaluation evaluation;
	const rebt::CircuitInput input = toCircuitInput(circuit);
	if (input.designCurrent) {
		evaluation.current.status = rebt::Status::Ok;
		evaluation.current.current = input.designCurrent;
		evaluation.current.assumptions.push_back(rebt::Assumption::CurrentGiven);
	} else {
		evaluation.current = rebt::designCurrent(input);
	}
	evaluation.drop = rebt::voltageDrop(input);
	evaluation.limit = limitFor(circuit);
	if (evaluation.limit)
		evaluation.check = rebt::checkVoltageDrop(evaluation.drop, *evaluation.limit);

	const rebt::AmpacityInput ampacity = toAmpacityInput(circuit);
	evaluation.ampacity = rebt::admissibleCurrent(ampacity, tables);
		//A generator: cables for 125 % of its maximum current (ITC-BT-40 5)
	const bool generator = circuit.use == CircuitUse::GeneratorLine;
	std::optional<double> sizing = evaluation.designCurrent();
	rebt::CircuitInput sizing_input = input;
	if (generator && sizing) {
		evaluation.sizingCurrent = GENERATOR_CABLE_FACTOR * *sizing;
		sizing = evaluation.sizingCurrent;
		if (sizing_input.designCurrent)
			sizing_input.designCurrent = sizing;
	}
	evaluation.ampacityCheck = rebt::checkAmpacity(sizing, evaluation.ampacity);
	if (generator)
		evaluation.ampacityCheck.rule = rebt::rules::pvAcLine();
	evaluation.suggestion = rebt::suggestSection(sizing_input, ampacity, tables,
												 evaluation.limit);

	rebt::CoordinationInput coordination;
	coordination.designCurrent = evaluation.designCurrent();
	coordination.ratedCurrent = circuit.protection.ratedCurrent;
	coordination.admissible = evaluation.ampacity.admissible;
	coordination.standard = circuit.protection.standard;
	coordination.conventionalCurrent = circuit.protection.conventionalCurrent;
	coordination.conventionalJustification =
		circuit.protection.conventionalCurrentReason.toStdString();
	coordination.protectsAgainstOverload =
		circuit.protection.type != ProtectionType::ResidualCurrent
		&& circuit.protection.type != ProtectionType::Switch;
	evaluation.coordination = rebt::checkCoordination(coordination);

	evaluation.dwelling = rebt::checkDwellingCircuit(dwellingInput(circuit));

		//The DC side of its PV generator: every group has to pass
	if (!circuit.pv.strings.isEmpty()) {
		bool all = true, failed = false;
		for (const PvStringGroup &g : circuit.pv.strings) {
			rebt::PvStringResult r = rebt::checkPvString(pvStringInput(circuit.pv, g));
			if (g.cableAdmissible && g.cableAdmissibleReason.trimmed().isEmpty())
				r.diagnostics.push_back({rebt::Issue::MissingJustification, "cableAdmissible"});
			evaluation.pvStrings << r;
			failed = failed || r.outcome == rebt::CheckOutcome::NotPassed;
			all = all && r.outcome == rebt::CheckOutcome::Passed;
		}
		evaluation.pvOutcome = failed ? rebt::CheckOutcome::NotPassed
							   : all ? rebt::CheckOutcome::Passed
									 : rebt::CheckOutcome::NotPerformed;
	}
	return evaluation;
}

rebt::PvStringInput pvStringInput(const PvData &pv, const PvStringGroup &g)
{
	rebt::PvStringInput in;
	in.module.openCircuitVoltage = pv.openCircuitVoltage;
	in.module.mppVoltage = pv.mppVoltage;
	in.module.shortCircuitCurrent = pv.shortCircuitCurrent;
	in.module.mppCurrent = pv.mppCurrent;
	in.module.vocCoefficient = pv.vocCoefficient;
	in.module.vmppCoefficient = pv.vmppCoefficient;
	in.module.maxSystemVoltage = pv.maxSystemVoltage;
	in.module.maxSeriesFuse = pv.maxSeriesFuse;
	in.module.source = pv.moduleSource.trimmed().toStdString();
	in.inverter.maxDcVoltage = pv.maxDcVoltage;
	in.inverter.mpptMinVoltage = pv.mpptMinVoltage;
	in.inverter.mpptMaxVoltage = pv.mpptMaxVoltage;
	in.inverter.maxInputCurrent = pv.maxInputCurrent;
	in.inverter.maxShortCircuitCurrent = pv.maxShortCircuitCurrent;
	in.inverter.source = pv.inverterSource.trimmed().toStdString();
	in.modulesPerString = g.modulesPerString;
	in.parallelStrings = g.parallelStrings;
	in.minCellTemperature = pv.minCellTemperature;
	in.maxCellTemperature = pv.maxCellTemperature;
	in.temperatureJustification = pv.temperatureReason.trimmed().toStdString();
	in.stringFuse = g.stringFuse;
	in.cableLength = g.cableLength;
	in.cableSection = g.cableSection;
	in.cableMaterial = g.cableMaterial;
	in.conductorTemperature = g.conductorTemperature;
		//An Iz given is used only with where it comes from
	if (!g.cableAdmissibleReason.trimmed().isEmpty())
		in.cableAdmissible = g.cableAdmissible;
	return in;
}

namespace {

std::string nameOf(const Circuit &circuit)
{
	return circuit.name.isEmpty() ? circuit.id.toString().toStdString()
								  : circuit.name.toStdString();
}

void addChainIssue(CircuitEvaluation &e, rebt::Issue issue, const Circuit &feeder)
{
	e.chainDiagnostics.push_back({issue, nameOf(feeder)});
}

bool hasText(const QString &text)
{
	return !text.trimmed().isEmpty();
}

std::optional<double> kiloToUnit(const std::optional<double> &value)
{
	if (!value)
		return std::nullopt;
	return *value * 1000.0;
}

/**
	The short circuit of @p circuit: the path goes up through every
	feeder to the board fed by nothing, whose supply data are used.
*/
rebt::ShortCircuitInput evaluateShortCircuit(CircuitEvaluation &e, const Circuit &circuit,
											 const ElectricalModel &model)
{
	rebt::ShortCircuitInput input;
	bool path_known = true;
	QSet<QUuid> visited{circuit.id};
	QList<Circuit> feeders;   // nearest first
	std::optional<Board> board = model.board(circuit.board);
	while (board && !board->feeder.isNull()) {
		const std::optional<Circuit> feeder = model.circuit(board->feeder);
		if (!feeder) {
			e.faultDiagnostics.push_back({rebt::Issue::FeederNotComputable,
										  board->feeder.toString().toStdString()});
			path_known = false;
			break;
		}
		if (visited.contains(feeder->id)) {
			e.faultDiagnostics.push_back({rebt::Issue::FeederLoop, nameOf(*feeder)});
			path_known = false;
			break;
		}
		visited.insert(feeder->id);
		feeders.prepend(*feeder);
		board = model.board(feeder->board);
	}
	if (!board) {
			//The circuit, or a feeder, leaves from no board of the model
		e.faultDiagnostics.push_back({rebt::Issue::MissingSource, "board"});
		path_known = false;
	}

	if (path_known) {
		e.faultOriginBoard = board->id;
		for (const Circuit &f : feeders) {
			e.faultFeeders << f.id;
			e.faultFeederNames << QString::fromStdString(nameOf(f));
			input.feeders.push_back(toLineStretch(f));
		}
		input.source.method = board->source;
		if (board->source == rebt::SourceMethod::KnownCurrent) {
				//A value given by the user is used only with its source
			if (hasText(board->sourceReason)) {
				input.source.maxCurrent = kiloToUnit(board->sourceMaxCurrent);
				input.source.minCurrent = kiloToUnit(board->sourceMinCurrent);
				e.sourceText = QStringLiteral("%1 / %2 kA, %3 (%4)")
					.arg(formatNumber(board->sourceMaxCurrent),
						 formatNumber(board->sourceMinCurrent),
						 board->name, board->sourceReason.trimmed());
			} else {
				e.faultDiagnostics.push_back({rebt::Issue::MissingJustification,
											  "source"});
			}
		}
	}

	if (circuit.system && circuit.voltage)
		input.phaseVoltage = *circuit.system == rebt::System::SinglePhase
							 ? *circuit.voltage : *circuit.voltage / std::sqrt(3.0);
	else if (!circuit.system)
		e.faultDiagnostics.push_back({rebt::Issue::MissingSystem, "system"});
	input.circuit = toLineStretch(circuit);

	const ProtectionData &pd = circuit.protection;
	input.protectsAgainstShortCircuit = pd.type != ProtectionType::ResidualCurrent
										&& pd.type != ProtectionType::Switch;
	input.breakingCapacity = kiloToUnit(pd.breakingCapacity);
	input.ratedCurrent = pd.ratedCurrent;
	input.standard = pd.standard;
	input.curve = tripCurve(pd);
	input.tripCurrent = pd.tripCurrent;
	input.tripCurrentJustification = pd.tripCurrentReason.toStdString();
	input.letThrough = pd.letThrough;
	input.letThroughAt = kiloToUnit(pd.letThroughAt);
	input.letThroughJustification = pd.letThroughReason.toStdString();
	e.tripCurrentReason = pd.tripCurrentReason.trimmed();
	e.letThroughReason = pd.letThroughReason.trimmed();

		//Path unknown: no source was set, nothing upstream is computed
	e.shortCircuit = rebt::checkShortCircuit(input);
	return input;
}

} // namespace

/**
	Indirect contacts of @p circuit: the earthing of its board or of the
	nearest one up the chain that gives it; Ia, the IDn of its residual
	current device or, with none, the instantaneous trip of its breaker.
*/
void evaluateIndirectContact(CircuitEvaluation &e, const Circuit &circuit,
							 const ElectricalModel &model,
							 const rebt::ShortCircuitInput &fault)
{
	rebt::IndirectContactInput input;
		//The fault loop, for TN and IT: that of the short circuit
	input.phaseVoltage = fault.phaseVoltage;
	input.source = fault.source;
	input.feeders = fault.feeders;
	input.circuit = fault.circuit;
	input.pathKnown = !e.faultOriginBoard.isNull();
	QSet<QUuid> seen;
	std::optional<Board> board = model.board(circuit.board);
	while (board && !seen.contains(board->id)) {
		seen.insert(board->id);
		if (board->earthing) {
			e.earthingBoard = board->id;
			input.earthing = board->earthing;
			input.earthResistance = board->earthResistance;
			input.earthResistanceJustification = board->earthResistanceReason.toStdString();
			input.limitVoltage = board->limitVoltage;
			input.limitVoltageJustification = board->limitVoltageReason.toStdString();
			input.firstFaultCurrent = board->firstFaultCurrent;
			input.firstFaultJustification = board->firstFaultReason.toStdString();
			input.massesInterconnected = board->massesInterconnected;
			input.neutralDistributed = board->neutralDistributed;
			e.firstFaultReason = board->firstFaultReason;
			e.earthResistanceReason = board->earthResistanceReason;
			e.limitVoltageReason = board->limitVoltageReason;
			break;
		}
		if (board->feeder.isNull())
			break;
		const std::optional<Circuit> feeder = model.circuit(board->feeder);
		board = feeder ? model.board(feeder->board) : std::nullopt;
	}

	const ProtectionData &pd = circuit.protection;
	const bool own_residual = pd.type == ProtectionType::ResidualCurrent
							  || pd.type == ProtectionType::Rcbo;
	bool missing_device = false;
	std::optional<double> sensitivity;   // mA
	if (own_residual) {
		sensitivity = pd.sensitivity;
		missing_device = !sensitivity;
	} else if (!pd.residualDevice.trimmed().isEmpty()) {
		const auto device = residualDeviceOf(circuit, model);
		e.residualDeviceName = pd.residualDevice.trimmed();
		if (device)
			sensitivity = device->sensitivity;
		missing_device = !sensitivity;
	}
	if (sensitivity) {
		input.residualCurrent = *sensitivity / 1000.0;
	} else if (!missing_device) {
			//No residual current device: the overcurrent device
		input.ratedCurrent = pd.ratedCurrent;
		input.standard = pd.standard;
		input.curve = tripCurve(pd);
		input.tripCurrent = pd.tripCurrent;
		input.tripCurrentJustification = pd.tripCurrentReason.toStdString();
	}
	if (input.earthing == rebt::EarthingSystem::IT && input.pathKnown) {
			//IT: the lines from the board where it is given, not above
		std::vector<rebt::LineStretch> below;
		bool reached = circuit.board == e.earthingBoard;
		for (int i = 0; i < e.faultFeeders.size(); ++i) {
			const std::optional<Circuit> f = model.circuit(e.faultFeeders.at(i));
			if (f && f->board == e.earthingBoard) reached = true;
			if (reached && !(circuit.board == e.earthingBoard))
				below.push_back(input.feeders.at(static_cast<std::size_t>(i)));
		}
		input.feeders = below;
	}
	e.indirect = rebt::checkIndirectContact(input);
	if (missing_device) {
			//The device named, or its IDn, is not known: not the breaker instead
		const rebt::Diagnostic d{own_residual || residualDeviceOf(circuit, model)
									 ? rebt::Issue::MissingSensitivity
									 : rebt::Issue::MissingResidualDevice,
								 "residualDevice"};
		e.indirect.diagnostics.push_back(d);
	}
}

CircuitEvaluation evaluate(const Circuit &circuit, const ElectricalModel &model,
						   const rebt::AmpacityLibrary &tables)
{
	CircuitEvaluation e = evaluate(circuit, tables);
	e.chainEvaluated = true;
	const rebt::ShortCircuitInput fault = evaluateShortCircuit(e, circuit, model);
	evaluateIndirectContact(e, circuit, model, fault);

		//The service connection is checked with its own drop, unless the
		//board it feeds compensates with it: then in the circuits there
	if (circuit.use && !isInterior(*circuit.use)) {
		e.cumulativePercent = e.drop.percent;
		for (const Board &b : model.boards()) {
			if (b.feeder == circuit.id && b.compensateWithIndividualLine) {
				e.compensated = true;
				e.check.outcome = rebt::CheckOutcome::NotPerformed;
				e.check.value.reset();
				e.drop.assumptions.push_back(rebt::Assumption::CompensatedWithIndividual);
			}
		}
		return e;
	}

	double sum = 0.0;
	bool known = true;
	QSet<QUuid> visited{circuit.id};
	std::optional<Board> board = model.board(circuit.board);
	while (board && !board->feeder.isNull()) {
		const std::optional<Circuit> feeder = model.circuit(board->feeder);
		if (!feeder) {
			e.chainDiagnostics.push_back({rebt::Issue::FeederNotComputable,
										  board->feeder.toString().toStdString()});
			known = false;
			break;
		}
		if (visited.contains(feeder->id)) {
			addChainIssue(e, rebt::Issue::FeederLoop, *feeder);
			known = false;
			break;
		}
		visited.insert(feeder->id);
		if (!feeder->use) {
			addChainIssue(e, rebt::Issue::FeederUseUnknown, *feeder);
			known = false;
			break;
		}
			//An LGA or a DI ends the interior installation
		if (!isInterior(*feeder->use))
			break;
		const rebt::VoltageDropResult drop = rebt::voltageDrop(toCircuitInput(*feeder));
		if (drop.status != rebt::Status::Ok) {
			addChainIssue(e, rebt::Issue::FeederNotComputable, *feeder);
			known = false;
			break;
		}
		sum += *drop.percent;
		e.feeders << feeder->id;
		board = model.board(feeder->board);
	}
		//Where the interior installation starts: from the boards and their
		//feeders alone, whatever data of the feeders are missing
	const std::optional<Board> origin = interiorOrigin(circuit, model);
	if (origin)
		e.originBoard = origin->id;

		//Own transformer at the origin: its limits for lighting and other
		//uses (ITC-BT-19 2.2.2); a dwelling keeps its 3 %
	if (origin && origin->feeder.isNull() && origin->ownTransformer && circuit.use
		&& (*circuit.use == CircuitUse::Lighting || *circuit.use == CircuitUse::OtherUses)) {
		e.limit = *circuit.use == CircuitUse::Lighting
				  ? rebt::rules::ownTransformerLighting()
				  : rebt::rules::ownTransformerOtherUses();
		e.check = rebt::checkVoltageDrop(e.drop, *e.limit);
		e.drop.assumptions.push_back(rebt::Assumption::OwnTransformerOrigin);
	}

	if (known && e.drop.percent)
		e.cumulativePercent = *e.drop.percent + sum;

		//Compensation with the DI that feeds the origin, when stated
	std::optional<rebt::VoltageDropLimit> di_limit;
	if (origin && origin->compensateWithIndividualLine && !origin->feeder.isNull()) {
		const std::optional<Circuit> di = model.circuit(origin->feeder);
		if (di && di->use && !isInterior(*di->use)) {
			e.compensated = true;
			const rebt::VoltageDropResult di_drop = rebt::voltageDrop(toCircuitInput(*di));
			if (di_drop.status == rebt::Status::Ok)
				e.individualPercent = di_drop.percent;
			else
				addChainIssue(e, rebt::Issue::MissingIndividualDrop, *di);
			di_limit = limitFor(*di);
		}
	}
	const bool compensating = e.compensated && e.individualPercent && di_limit;

		//Dwelling circuit: the drop with IN of its breaker (ITC-BT-25 3)
	if (circuit.dwellingCircuit && circuit.protection.ratedCurrent) {
		rebt::CircuitInput at_rated = toCircuitInput(circuit);
		at_rated.designCurrent = circuit.protection.ratedCurrent;
		if (!at_rated.powerFactor)
			at_rated.powerFactor = 1.0;
		e.dropAtRated = rebt::voltageDrop(at_rated);
			//The current is IN, put in here: not one the user gave
		auto &a = e.dropAtRated.assumptions;
		a.erase(std::remove(a.begin(), a.end(), rebt::Assumption::CurrentGiven), a.end());
		a.insert(a.begin(), rebt::Assumption::DropAtRatedCurrent);
		if (!circuit.load.powerFactor)
			e.dropAtRated.assumptions.push_back(rebt::Assumption::PowerFactorOne);
		if (known && e.dropAtRated.status == rebt::Status::Ok) {
			e.cumulativeAtRated = *e.dropAtRated.percent + sum;
			rebt::DwellingInput input = dwellingInput(circuit);
			if (!e.compensated) {
				input.dropAtRated = e.cumulativeAtRated;
			} else if (compensating) {
				input.dropAtRated = *e.cumulativeAtRated + *e.individualPercent;
				input.compensatedLimit = 3.0 + di_limit->percent;
			}
			e.dwelling = rebt::checkDwellingCircuit(input);
		}
	}

	if (!e.limit)
		return e;

		//The suggested section too: the feeders upstream already take part
		//of the limit, what is left is the circuit's. Without the drop of
		//the feeders, by admissible current only, and it says so.
	if (compensating) {
		rebt::VoltageDropLimit limit;
		limit.percent = e.limit->percent + di_limit->percent;
		limit.strict = true;   // "inferior a la suma de los valores limites"
		limit.rule = rebt::rules::compensatedDrop();
		e.limit = limit;
	}
	{
		std::optional<rebt::VoltageDropLimit> left;
		if (known && (!e.compensated || compensating)) {
			left = *e.limit;
			left->percent -= sum + (compensating ? *e.individualPercent : 0.0);
		}
		e.suggestion = rebt::suggestSection(toCircuitInput(circuit),
											toAmpacityInput(circuit), tables, left);
	}

		//The check of the interior drop, now against the drop from the origin
	if (e.compensated && !compensating) {
			//Its own limit no longer applies, the compensated one is unknown
		e.check.outcome = rebt::CheckOutcome::NotPerformed;
		e.check.value.reset();
	} else if (e.cumulativePercent) {
		rebt::VoltageDropResult total = e.drop;
		total.percent = e.cumulativePercent;
		if (compensating) {
			e.compensatedPercent = *e.cumulativePercent + *e.individualPercent;
			total.percent = e.compensatedPercent;
			e.drop.assumptions.push_back(rebt::Assumption::CompensatedWithIndividual);
		}
		e.check = rebt::checkVoltageDrop(total, *e.limit);
		e.drop.assumptions.push_back(rebt::Assumption::OriginBoard);
	} else if (e.check.outcome != rebt::CheckOutcome::NotPassed) {
			//Its own drop alone may already be too much; otherwise unknown
		e.check.outcome = rebt::CheckOutcome::NotPerformed;
		e.check.value.reset();
	}
	return e;
}

rebt::MinimumCircuitsResult minimumCircuits(const QUuid &origin,
											const ElectricalModel &model)
{
	std::vector<rebt::DwellingCircuit> circuits;
	for (const Circuit &c : model.circuits()) {
		if (!c.dwellingCircuit)
			continue;
		if (evaluate(c, model).originBoard == origin)
			circuits.push_back(*c.dwellingCircuit);
	}
	return rebt::checkMinimumCircuits(circuits);
}

rebt::DwellingCheck dwellingCheck(const QUuid &origin, const ElectricalModel &model,
								  const rebt::AmpacityLibrary &tables)
{
	rebt::DwellingData data;
	const std::optional<Board> start = model.board(origin);
	if (!start)
		return rebt::checkDwelling(data);
	data.area = start->dwellingArea;
	data.heating = start->plannedHeating;
	data.airConditioning = start->plannedAirConditioning;
	data.automation = start->plannedAutomation;
	data.vehicleCharging = start->plannedVehicleCharging;
	data.singleFamily = start->singleFamily;
	data.generalSwitch = start->generalSwitch;
	data.dryer = start->plannedDryer;
	for (const RoomData &room : start->rooms) {
		rebt::Room r;
		r.name = (room.name.isEmpty() ? QStringLiteral("?") : room.name).toStdString();
		r.type = room.type;
		r.area = room.area;
		r.length = room.length;
		r.accesses = room.accesses;
		for (auto it = room.counts.cbegin(); it != room.counts.cend(); ++it)
			r.counts.emplace_back(it.key(), it.value());
		data.rooms.push_back(r);
	}
		//The DI that feeds the dwelling, when it is in the model. A board
		//fed by an interior circuit starts no dwelling: none of the
		//circuits below has it as origin, so it is never checked.
	if (!start->feeder.isNull()) {
		const std::optional<Circuit> feeder = model.circuit(start->feeder);
		if (feeder && feeder->use && !isInterior(*feeder->use)) {
			const CircuitEvaluation di = evaluate(*feeder, tables);
			data.individualLine = di.ampacity.admissible;
		}
	}

		//Residual devices: by board and name; an RCBO is its own
	QMap<QPair<QUuid, QString>, int> devices;
	for (const Circuit &c : model.circuits()) {
		if (!c.dwellingCircuit || evaluate(c, model).originBoard != origin)
			continue;
		data.circuits.push_back(*c.dwellingCircuit);
		const ProtectionData &p = c.protection;
		if (p.type == ProtectionType::Rcbo) {
			rebt::ResidualDevice own;
			own.name = (c.name.isEmpty() ? c.id.toString() : c.name).toStdString();
			own.sensitivity = p.sensitivity;
			own.rated = p.ratedCurrent;
			own.combined = true;
			own.circuits = {*c.dwellingCircuit};
			data.residualDevices.push_back(own);
			continue;
		}
			//On its board or, protecting the feeder, on one upstream
		const QString name = p.residualDevice.trimmed();
		QUuid holder;
		const std::optional<ResidualDeviceData> found = residualDeviceOf(c, model, &holder);
		if (!found) {
			++data.unprotectedCircuits;
			continue;
		}
		const auto key = qMakePair(holder, name);
		if (!devices.contains(key)) {
			rebt::ResidualDevice device;
			device.name = name.toStdString();
			device.sensitivity = found->sensitivity;
			device.rated = found->rated;
			devices.insert(key, static_cast<int>(data.residualDevices.size()));
			data.residualDevices.push_back(device);
		}
		data.residualDevices[devices.value(key)].circuits.push_back(*c.dwellingCircuit);
	}
	return rebt::checkDwelling(data);
}

std::optional<ResidualDeviceData> residualDeviceOf(const Circuit &circuit,
												   const ElectricalModel &model,
												   QUuid *holder)
{
	const QString name = circuit.protection.residualDevice.trimmed();
	QSet<QUuid> seen;
	std::optional<Board> board = model.board(circuit.board);
	while (board && !name.isEmpty() && !seen.contains(board->id)) {
		seen.insert(board->id);
		for (const ResidualDeviceData &d : board->residualDevices) {
			if (d.name.trimmed() == name) {
				if (holder) *holder = board->id;
				return d;
			}
		}
		if (board->feeder.isNull())
			break;
		const std::optional<Circuit> feeder = model.circuit(board->feeder);
		board = feeder ? model.board(feeder->board) : std::nullopt;
	}
	return std::nullopt;
}

std::optional<Board> interiorOrigin(const Circuit &circuit, const ElectricalModel &model)
{
	QSet<QUuid> visited{circuit.id};
	std::optional<Board> board = model.board(circuit.board);
	while (board && !board->feeder.isNull()) {
		const std::optional<Circuit> feeder = model.circuit(board->feeder);
		if (!feeder || visited.contains(feeder->id) || !feeder->use)
			return std::nullopt;   // not known where it starts
		if (!isInterior(*feeder->use))
			return board;          // fed by an LGA or a DI
		visited.insert(feeder->id);
		board = model.board(feeder->board);
	}
	return board;
}

bool isInteriorOrigin(const Board &board, const ElectricalModel &model)
{
	if (board.feeder.isNull())
		return true;
	const std::optional<Circuit> feeder = model.circuit(board.feeder);
	return feeder && feeder->use && !isInterior(*feeder->use);
}

std::optional<double> parseNumber(const QString &text, bool *ok)
{
	QString t = text.trimmed();
	if (ok) *ok = true;
	if (t.isEmpty())
		return std::nullopt;
	if (t.contains(QLatin1Char(',')) && t.contains(QLatin1Char('.'))) {
		if (ok) *ok = false;
		return std::nullopt;
	}
	t.replace(QLatin1Char(','), QLatin1Char('.'));
	bool good = false;
	const double value = QLocale::c().toDouble(t, &good);
	if (!good || !std::isfinite(value)) {
		if (ok) *ok = false;
		return std::nullopt;
	}
	return value;
}

QString formatNumber(const std::optional<double> &value, int decimals)
{
	if (!value)
		return QString();
		//In the user's language, and never with a group separator: read
		//back, "3.680" would be 3.68
	QLocale locale;
	locale.setNumberOptions(QLocale::OmitGroupSeparator);
	if (decimals < 0)
		return locale.toString(*value, 'g', QLocale::FloatingPointShortest);
	return locale.toString(*value, 'f', decimals);
}

} // namespace electrical
