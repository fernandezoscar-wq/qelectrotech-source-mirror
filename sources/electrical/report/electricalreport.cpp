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
#include "electricalreport.h"

#include "../ui/electricaltexts.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QMap>
#include <QSet>

namespace electrical {

namespace {

CellState stateOf(rebt::CheckOutcome outcome)
{
	switch (outcome) {
		case rebt::CheckOutcome::Passed: return CellState::Passed;
		case rebt::CheckOutcome::NotPassed: return CellState::NotPassed;
		case rebt::CheckOutcome::NotPerformed: return CellState::NotPerformed;
	}
	return CellState::Plain;
}

QString circuitName(const Circuit &c)
{
	return c.name.isEmpty() ? ElectricalReportBuilder::tr("(unnamed circuit)") : c.name;
}

QString boardName(const Board &b)
{
	return b.name.isEmpty() ? ElectricalReportBuilder::tr("(unnamed board)") : b.name;
}

QString protectionText(const ProtectionData &p)
{
	QStringList parts;
	if (p.type)
		parts << ElectricalTexts::protection(*p.type);
	QString rating = p.curve;
	if (p.ratedCurrent)
		rating = (rating + QLatin1Char(' ')
				  + ElectricalReportBuilder::tr("%1 A").arg(formatNumber(p.ratedCurrent)))
					 .trimmed();
	if (!rating.isEmpty())
		parts << rating;
	if (p.sensitivity)
		parts << ElectricalReportBuilder::tr("%1 mA").arg(formatNumber(p.sensitivity));
	if (p.breakingCapacity)
		parts << ElectricalReportBuilder::tr("%1 kA").arg(formatNumber(p.breakingCapacity));
	if (p.standard)
		parts << ElectricalTexts::standard(*p.standard).section(QLatin1Char(' '), 0, 1);
	return parts.join(QStringLiteral(", "));
}

QString cableText(const ConductorData &c)
{
	QStringList parts;
	if (c.material)
		parts << (*c.material == rebt::Material::Copper ? QStringLiteral("Cu")
														: QStringLiteral("Al"));
	if (c.insulation)
		parts << ElectricalTexts::insulation(*c.insulation);
	QStringList sections;
	if (c.section)
		sections << ElectricalReportBuilder::tr("%1 mm²").arg(formatNumber(c.section));
	if (c.neutralSection)
		sections << ElectricalReportBuilder::tr("N %1").arg(formatNumber(c.neutralSection));
	if (c.peSection)
		sections << ElectricalReportBuilder::tr("PE %1").arg(formatNumber(c.peSection));
	if (!sections.isEmpty())
		parts << sections.join(QStringLiteral(" + "));
	if (!c.installationMethod.isEmpty())
		parts << c.installationMethod;
	return parts.join(QLatin1Char(' '));
}

QString outcomeWord(rebt::CheckOutcome outcome)
{
	switch (outcome) {
		case rebt::CheckOutcome::Passed: return ElectricalReportBuilder::tr("Passed");
		case rebt::CheckOutcome::NotPassed: return ElectricalReportBuilder::tr("Not passed");
		case rebt::CheckOutcome::NotPerformed: return ElectricalReportBuilder::tr("Not performed");
	}
	return QString();
}

/// The lines of a details text, without its own list of rules: the
/// rules have a section of their own in the report
QStringList withoutRules(const QString &details)
{
	QStringList lines;
	for (const QString &line : details.split(QLatin1Char('\n'))) {
		if (line.isEmpty()) continue;
		if (line.startsWith(ElectricalTexts::tr("Rules: %1").section(QLatin1Char('%'), 0, 0)))
			continue;
		lines << line;
	}
	return lines;
}

} // namespace

void CheckCounts::add(rebt::CheckOutcome outcome)
{
	switch (outcome) {
		case rebt::CheckOutcome::Passed: ++passed; break;
		case rebt::CheckOutcome::NotPassed: ++notPassed; break;
		case rebt::CheckOutcome::NotPerformed: ++notPerformed; break;
	}
}

QString ElectricalReportBuilder::defaultDocumentTitle()
{
	return tr("Electrical calculation annex");
}

ElectricalReport ElectricalReportBuilder::build(const ElectricalModel &model,
												const rebt::AmpacityLibrary &tables,
												const QString &tablesFile,
												const QString &projectTitle)
{
	ElectricalReport report;
	report.info = model.reportInfo();
	if (report.info.documentTitle.isEmpty())
		report.info.documentTitle = defaultDocumentTitle();
	report.projectTitle = projectTitle;

		//Columns: their widths add up to 277 mm, an A4 landscape page
		//with 10 mm margins
	const QList<QPair<QString, double>> columns{
		{tr("Circuit"), 24}, {tr("Use"), 15}, {tr("Sys."), 8}, {tr("U (V)"), 9},
		{tr("P (W)"), 10}, {tr("Ib (A)"), 10}, {tr("Protection"), 24},
		{tr("Cable"), 21}, {tr("L (m)"), 9}, {tr("Iz (A)"), 10},
		{tr("ΔU (%)"), 10}, {tr("ΔU total (%)"), 10}, {tr("ΔU check"), 15},
		{tr("Ib ≤ Iz"), 14}, {tr("Protection ↔ cable"), 15},
		{tr("Icc max (kA)"), 11}, {tr("Icc min (kA)"), 11},
		{tr("Short circuit"), 19}, {tr("ITC-BT-25"), 18}, {tr("ITC-BT-24"), 14}};
	for (const Board &b : model.boards())
		report.earthingGiven = report.earthingGiven || b.earthing.has_value();
	for (const auto &c : columns) {
		report.columns << c.first;
		report.columnWidths << c.second;
	}

	QMap<QString, rebt::NormRef> rules;
	QSet<QString> tables_used;
	auto add_rules = [&rules](const std::vector<rebt::NormRef> &list) {
		for (const rebt::NormRef &r : list)
			rules.insert(QString::fromStdString(r.id), r);
	};

	auto section_for = [&](const QString &title, const QString &subtitle,
						   const QUuid &board_id, bool orphans) {
		BoardSection section;
		section.title = title;
		section.subtitle = subtitle;
		for (const Circuit &c : model.circuits()) {
			const bool here = orphans ? !model.board(c.board).has_value()
									  : c.board == board_id;
			if (!here) continue;
			const CircuitEvaluation e = evaluate(c, model, tables);
			++report.circuitCount;
			report.drop.add(e.check.outcome);
			report.ampacity.add(e.ampacityCheck.outcome);
			report.coordination.add(e.coordination.outcome);
			report.shortCircuit.add(e.shortCircuit.outcome);
			if (c.dwellingCircuit)
				report.dwelling.add(e.dwelling.outcome);
			if (report.earthingGiven)
				report.indirect.add(e.indirect.outcome);
			auto kilo = [](const std::optional<double> &a) {
				return a ? formatNumber(*a / 1000.0, 2) : QStringLiteral("—");
			};

			ReportRow row;
			row << ReportCell{circuitName(c)}
				<< ReportCell{c.use ? ElectricalTexts::use(*c.use) : QString()}
				<< ReportCell{c.system ? (*c.system == rebt::System::SinglePhase
										  ? tr("1~") : tr("3~")) : QString()}
				<< ReportCell{formatNumber(c.voltage)}
				<< ReportCell{formatNumber(c.load.power)}
				<< ReportCell{formatNumber(e.designCurrent(), 2)}
				<< ReportCell{protectionText(c.protection)}
				<< ReportCell{cableText(c.conductor)}
				<< ReportCell{formatNumber(c.conductor.length)}
				<< ReportCell{e.ampacity.status == rebt::Status::Ok
							  ? formatNumber(e.ampacity.admissible, 2) : QStringLiteral("—")}
				<< ReportCell{e.drop.status == rebt::Status::Ok
							  ? formatNumber(e.drop.percent, 2) : QStringLiteral("—")}
				<< ReportCell{e.cumulativePercent ? formatNumber(e.cumulativePercent, 2)
												  : QStringLiteral("—")}
				<< ReportCell{e.limit ? outcomeWord(e.check.outcome)
									  : ElectricalTexts::tr("No limit", "voltage drop check: the circuit use is not known"),
							  e.limit ? stateOf(e.check.outcome) : CellState::NotPerformed}
				<< ReportCell{outcomeWord(e.ampacityCheck.outcome),
							  stateOf(e.ampacityCheck.outcome)}
				<< ReportCell{outcomeWord(e.coordination.outcome),
							  stateOf(e.coordination.outcome)}
				<< ReportCell{kilo(e.shortCircuit.maxCurrent)}
				<< ReportCell{kilo(e.shortCircuit.minCurrent)}
				<< ReportCell{outcomeWord(e.shortCircuit.outcome),
							  stateOf(e.shortCircuit.outcome)}
				<< (c.dwellingCircuit
					? ReportCell{ElectricalTexts::dwellingCode(*c.dwellingCircuit)
								 + QStringLiteral(": ") + outcomeWord(e.dwelling.outcome),
								 stateOf(e.dwelling.outcome)}
					: ReportCell{})
				<< ReportCell{outcomeWord(e.indirect.outcome), stateOf(e.indirect.outcome)};
			section.rows << row;
			section.circuits << c.id;

				//Its results, in words
			CircuitNotes notes;
			notes.title = title + QStringLiteral(" › ") + circuitName(c);
			notes.hasProblem = e.check.outcome == rebt::CheckOutcome::NotPassed
							   || e.ampacityCheck.outcome == rebt::CheckOutcome::NotPassed
							   || e.coordination.outcome == rebt::CheckOutcome::NotPassed
							   || e.shortCircuit.outcome == rebt::CheckOutcome::NotPassed
							   || e.indirect.outcome == rebt::CheckOutcome::NotPassed
							   || (c.dwellingCircuit
								   && e.dwelling.outcome == rebt::CheckOutcome::NotPassed);
			if (!c.protection.manufacturer.isEmpty() || !c.protection.partNumber.isEmpty())
				notes.lines << tr("Protective device: %1")
					.arg(QStringList{c.protection.manufacturer, c.protection.partNumber}
							 .filter(QRegularExpression(QStringLiteral("\\S")))
							 .join(QStringLiteral(", ")));
			notes.lines << tr("Voltage drop: %1").arg(ElectricalTexts::outcome(e));
			for (const QString &line : withoutRules(ElectricalTexts::details(e)))
				notes.lines << QStringLiteral("    ") + line;
			notes.lines << tr("IB ≤ IZ: %1").arg(outcomeWord(e.ampacityCheck.outcome));
			for (const QString &line : withoutRules(ElectricalTexts::ampacityDetails(e)))
				notes.lines << QStringLiteral("    ") + line;
			if (e.ampacityCheck.outcome != rebt::CheckOutcome::Passed
				|| e.check.outcome == rebt::CheckOutcome::NotPassed)
				notes.lines << QStringLiteral("    ") + ElectricalTexts::suggestion(e);
			notes.lines << tr("Protection ↔ cable: %1").arg(outcomeWord(e.coordination.outcome));
			for (const QString &line : withoutRules(ElectricalTexts::coordinationDetails(e)))
				notes.lines << QStringLiteral("    ") + line;
			notes.lines << tr("Short circuit: %1").arg(outcomeWord(e.shortCircuit.outcome));
			for (const QString &line : withoutRules(ElectricalTexts::shortCircuitDetails(e)))
				notes.lines << QStringLiteral("    ") + line;
			if (report.earthingGiven) {
				notes.lines << tr("Indirect contacts: %1").arg(outcomeWord(e.indirect.outcome));
				for (const QString &line : withoutRules(ElectricalTexts::indirectDetails(e)))
					notes.lines << QStringLiteral("    ") + line;
				add_rules(e.indirect.rules);
			}
			if (c.dwellingCircuit) {
				notes.lines << tr("ITC-BT-25, %1: %2")
					.arg(ElectricalTexts::dwellingCircuit(*c.dwellingCircuit),
						 outcomeWord(e.dwelling.outcome));
				for (const QString &line : withoutRules(ElectricalTexts::dwellingDetails(e)))
					notes.lines << QStringLiteral("    ") + line;
				add_rules(e.dwelling.rules);
			}
			if (!c.notes.isEmpty())
				notes.lines << tr("Notes: %1").arg(c.notes);
			report.notes << notes;

			add_rules(e.drop.rules);
			if (e.limit) add_rules({e.limit->rule});
			add_rules(e.ampacity.rules);
			add_rules(e.coordination.rules);
			add_rules(e.shortCircuit.rules);
			if (!e.ampacity.tableId.empty())
				tables_used << tr("%1 (%2)").arg(QString::fromStdString(e.ampacity.tableId),
												 QString::fromStdString(e.ampacity.tableSource));
		}
		if (!section.rows.isEmpty() || !orphans)
			report.boards << section;
	};

	for (const Board &b : model.boards()) {
		QString subtitle;
		if (b.feeder.isNull()) {
			subtitle = tr("Origin of the interior installation: fed by no circuit of the project");
			if (!b.source)
				subtitle += QStringLiteral(". ") + tr("Supply not known: no short circuit");
			else if (*b.source == rebt::SourceMethod::GuideFactor)
				subtitle += QStringLiteral(". ") + ElectricalTexts::source(*b.source);
			else
				subtitle += QStringLiteral(". ")
							+ tr("Icc at this board: maximum %1 kA, minimum %2 kA (%3)")
							  .arg(formatNumber(b.sourceMaxCurrent),
								   formatNumber(b.sourceMinCurrent), b.sourceReason);
			if (b.ownTransformer)
				subtitle += QStringLiteral(". ")
							+ tr("Own transformer: voltage drop at most 4.5 % lighting, "
								 "6.5 % other uses (ITC-BT-19 2.2.2)");
		} else if (auto feeder = model.circuit(b.feeder)) {
			subtitle = tr("Fed by %1").arg(circuitName(*feeder));
		}
		if (isInteriorOrigin(b, model)) {
			const rebt::DwellingCheck dwelling = dwellingCheck(b.id, model, tables);
			if (!dwelling.rules.empty()) {
				report.dwellings.add(dwelling.outcome);
				DwellingNotes d;
				d.title = boardName(b);
				d.outcome = dwelling.outcome;
				d.summary = ElectricalTexts::dwellingCheckOutcome(dwelling);
				d.lines = withoutRules(ElectricalTexts::dwellingCheckDetails(dwelling));
				report.dwellingNotes << d;
				subtitle += (subtitle.isEmpty() ? QString() : QStringLiteral(". "))
							+ tr("ITC-BT-25: %1").arg(d.summary);
				for (const rebt::NormRef &r : dwelling.rules)
					rules.insert(QString::fromStdString(r.id), r);
			}
		}
		section_for(boardName(b), subtitle, b.id, false);
	}
	section_for(tr("(no board)"), QString(), QUuid(), true);

	report.rules = rules.values();
	report.tables = tables_used.values();
	report.tables.sort();
	if (report.tables.isEmpty())
		report.tables << (tablesFile.isEmpty()
						  ? tr("No ampacity table loaded: IZ not computed.")
						  : tr("No table of %1 applies to these circuits.")
							.arg(QFileInfo(tablesFile).fileName()));

	report.disclaimer = tr(
		"This annex shows, circuit by circuit, the checks listed, done with "
		"the data entered in the project. It does not state that the "
		"installation complies with the REBT as a whole: protection against "
		"electric shock beyond the conditions of ITC-BT-24 4.1 for indirect "
		"contacts (TT, TN and IT, with the RA and Id given), the earthing "
		"itself, selectivity, the protective conductor and "
		"the particular requirements of each ITC, beyond the checks of table 1 "
		"of ITC-BT-25 listed, are not checked here. The "
		"short circuit is checked only for the phase conductors, with the "
		"supply data of the origin board and the methods stated. A check "
		"marked \"not performed\" lacks data: it is neither passed nor failed.");
	return report;
}

} // namespace electrical
