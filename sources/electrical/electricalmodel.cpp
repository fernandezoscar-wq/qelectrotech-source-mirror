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
#include "electricalmodel.h"

#include <QLocale>
#include <QTextStream>

#include <cmath>

namespace electrical {

namespace {

// The words written to the file. They never change once released: a new
// value gets a new word.
template <typename E>
struct Words { E value; const char *word; };

const Words<rebt::System> SYSTEMS[] = {
	{rebt::System::SinglePhase, "single-phase"},
	{rebt::System::ThreePhase, "three-phase"}};
const Words<rebt::Material> MATERIALS[] = {
	{rebt::Material::Copper, "Cu"},
	{rebt::Material::Aluminium, "Al"}};
const Words<CircuitUse> USES[] = {
	{CircuitUse::DwellingInterior, "dwelling-interior"},
	{CircuitUse::Lighting, "lighting"},
	{CircuitUse::OtherUses, "other-uses"},
	{CircuitUse::LgaTotallyCentralized, "lga-totally-centralized"},
	{CircuitUse::LgaPartlyCentralized, "lga-partly-centralized"},
	{CircuitUse::DiMetersInSeveralPlaces, "di-meters-in-several-places"},
	{CircuitUse::DiMetersTotallyGrouped, "di-meters-totally-grouped"},
	{CircuitUse::DiSingleUser, "di-single-user"},
	{CircuitUse::GeneratorLine, "generator-line"}};
const Words<rebt::ProtectionStandard> STANDARDS[] = {
	{rebt::ProtectionStandard::EN60898, "EN60898"},
	{rebt::ProtectionStandard::EN61009, "EN61009"},
	{rebt::ProtectionStandard::EN60947_2, "EN60947-2"},
	{rebt::ProtectionStandard::FuseGG, "gG"}};
const Words<Insulation> INSULATIONS[] = {
	{Insulation::PVC, "PVC"},
	{Insulation::XLPE, "XLPE"},
	{Insulation::EPR, "EPR"}};
const Words<rebt::DwellingCircuit> DWELLING_CIRCUITS[] = {
	{rebt::DwellingCircuit::C1, "C1"}, {rebt::DwellingCircuit::C2, "C2"},
	{rebt::DwellingCircuit::C3, "C3"}, {rebt::DwellingCircuit::C4, "C4"},
	{rebt::DwellingCircuit::C4Single, "C4-appliance"},
	{rebt::DwellingCircuit::C5, "C5"}, {rebt::DwellingCircuit::C6, "C6"},
	{rebt::DwellingCircuit::C7, "C7"}, {rebt::DwellingCircuit::C8, "C8"},
	{rebt::DwellingCircuit::C9, "C9"}, {rebt::DwellingCircuit::C10, "C10"},
	{rebt::DwellingCircuit::C11, "C11"}, {rebt::DwellingCircuit::C12C3, "C12-C3"},
	{rebt::DwellingCircuit::C12C4, "C12-C4"}, {rebt::DwellingCircuit::C12C5, "C12-C5"},
	{rebt::DwellingCircuit::C13, "C13"}};
const Words<bool> BOOLS[] = {{true, "true"}, {false, "false"}};
const Words<rebt::RoomType> ROOM_TYPES[] = {
	{rebt::RoomType::Access, "access"}, {rebt::RoomType::Hall, "hall"},
	{rebt::RoomType::LivingRoom, "living-room"}, {rebt::RoomType::Bedroom, "bedroom"},
	{rebt::RoomType::Bathroom, "bathroom"}, {rebt::RoomType::Corridor, "corridor"},
	{rebt::RoomType::Kitchen, "kitchen"},
	{rebt::RoomType::TerraceOrDressing, "terrace-or-dressing"},
	{rebt::RoomType::GarageOrOther, "garage-or-other"}};
const rebt::RoomItem ROOM_ITEMS[] = {
	rebt::RoomItem::Bell, rebt::RoomItem::LightPoints, rebt::RoomItem::Switches,
	rebt::RoomItem::Sockets, rebt::RoomItem::Cooker, rebt::RoomItem::ApplianceSockets,
	rebt::RoomItem::WetSockets, rebt::RoomItem::Heating,
	rebt::RoomItem::AirConditioning, rebt::RoomItem::Dryer,
	rebt::RoomItem::VehicleCharging};
const Words<rebt::SourceMethod> SOURCES[] = {
	{rebt::SourceMethod::GuideFactor, "guide-factor"},
	{rebt::SourceMethod::KnownCurrent, "known-current"}};
const Words<rebt::EarthingSystem> EARTHINGS[] = {
	{rebt::EarthingSystem::TT, "TT"},
	{rebt::EarthingSystem::TN, "TN"},
	{rebt::EarthingSystem::IT, "IT"},
	{rebt::EarthingSystem::TNC, "TN-C"}};
const Words<ProtectionType> PROTECTIONS[] = {
	{ProtectionType::CircuitBreaker, "circuit-breaker"},
	{ProtectionType::ResidualCurrent, "residual-current"},
	{ProtectionType::Rcbo, "rcbo"},
	{ProtectionType::Fuse, "fuse"},
	{ProtectionType::Switch, "switch"}};

template <typename E, size_t N>
QString wordOf(const Words<E> (&words)[N], E value)
{
	for (const auto &w : words)
		if (w.value == value)
			return QString::fromLatin1(w.word);
	return QString();
}

/**
	Writes only what is known
*/
class Writer
{
	public:
		explicit Writer(QDomElement &element) : m_element(element) {}

		void number(const char *name, const std::optional<double> &value)
		{
			if (value)
				m_element.setAttribute(QLatin1String(name),
									   QString::number(*value, 'g',
													   QLocale::FloatingPointShortest));
		}

		void integer(const char *name, const std::optional<int> &value)
		{
			if (value)
				m_element.setAttribute(QLatin1String(name), *value);
		}

		void text(const char *name, const QString &value)
		{
			if (!value.isEmpty())
				m_element.setAttribute(QLatin1String(name), value);
		}

		void uuid(const char *name, const QUuid &value)
		{
			if (!value.isNull())
				m_element.setAttribute(QLatin1String(name), value.toString());
		}

		template <typename E, size_t N>
		void word(const char *name, const Words<E> (&words)[N],
				  const std::optional<E> &value)
		{
			if (value)
				m_element.setAttribute(QLatin1String(name), wordOf(words, *value));
		}

	private:
		QDomElement &m_element;
};

/**
	Reads what is there; a value it cannot make sense of is left empty
	and listed in the report, never guessed.
*/
class Reader
{
	public:
		Reader(const QDomElement &element, const QString &path,
			   ReadReport &report) :
			m_element(element), m_path(path), m_report(report)
		{}

		std::optional<double> number(const char *name)
		{
			if (!has(name))
				return std::nullopt;
			bool ok = false;
			const double value = attribute(name).toDouble(&ok);
			if (!ok || !std::isfinite(value)) {
				bad(name);
				return std::nullopt;
			}
			return value;
		}

		std::optional<int> integer(const char *name)
		{
			if (!has(name))
				return std::nullopt;
			bool ok = false;
			const int value = attribute(name).toInt(&ok);
			if (!ok) {
				bad(name);
				return std::nullopt;
			}
			return value;
		}

		QString text(const char *name) { return attribute(name); }

		QUuid uuid(const char *name)
		{
			if (!has(name))
				return QUuid();
			const QUuid value(attribute(name));
			if (value.isNull())
				bad(name);
			return value;
		}

		template <typename E, size_t N>
		std::optional<E> word(const char *name, const Words<E> (&words)[N])
		{
			if (!has(name))
				return std::nullopt;
			const QString value = attribute(name);
			for (const auto &w : words)
				if (value == QLatin1String(w.word))
					return w.value;
			bad(name);
			return std::nullopt;
		}

		void bad(const char *name)
		{
			m_report.badValues << m_path + QLatin1Char('@') + QLatin1String(name);
		}

	private:
		bool has(const char *name) const
		{ return m_element.hasAttribute(QLatin1String(name)); }
		QString attribute(const char *name) const
		{ return m_element.attribute(QLatin1String(name)); }

		QDomElement m_element;
		QString m_path;
		ReadReport &m_report;
};

template <typename T>
int indexOf(const QList<T> &list, const QUuid &id)
{
	for (int i = 0; i < list.size(); ++i)
		if (list.at(i).id == id)
			return i;
	return -1;
}

} // namespace

bool isInterior(CircuitUse use)
{
	return use == CircuitUse::DwellingInterior || use == CircuitUse::Lighting
		   || use == CircuitUse::OtherUses;
}

bool Load::operator==(const Load &o) const
{
	return power == o.power && powerFactor == o.powerFactor
		   && efficiency == o.efficiency && designCurrent == o.designCurrent;
}

bool ConductorData::operator==(const ConductorData &o) const
{
	return material == o.material && insulation == o.insulation
		   && installationMethod == o.installationMethod
		   && section == o.section && neutralSection == o.neutralSection
		   && peSection == o.peSection && length == o.length
		   && temperature == o.temperature && conductivity == o.conductivity
		   && reactance == o.reactance && conduit == o.conduit && wires == o.wires
		   && loadedConductors == o.loadedConductors
		   && ambientTemperature == o.ambientTemperature
		   && temperatureFactor == o.temperatureFactor
		   && temperatureFactorReason == o.temperatureFactorReason
		   && groupingFactor == o.groupingFactor
		   && groupingFactorReason == o.groupingFactorReason
		   && otherFactor == o.otherFactor
		   && otherFactorReason == o.otherFactorReason;
}

bool ProtectionData::operator==(const ProtectionData &o) const
{
	return element == o.element && manufacturer == o.manufacturer
		   && partNumber == o.partNumber && type == o.type
		   && ratedCurrent == o.ratedCurrent && curve == o.curve
		   && breakingCapacity == o.breakingCapacity
		   && sensitivity == o.sensitivity && poles == o.poles
		   && standard == o.standard
		   && conventionalCurrent == o.conventionalCurrent
		   && conventionalCurrentReason == o.conventionalCurrentReason
		   && tripCurrent == o.tripCurrent
		   && tripCurrentReason == o.tripCurrentReason
		   && letThrough == o.letThrough && letThroughAt == o.letThroughAt
		   && letThroughReason == o.letThroughReason
		   && residualDevice == o.residualDevice;
}

bool ResidualDeviceData::operator==(const ResidualDeviceData &o) const
{
	return name == o.name && sensitivity == o.sensitivity && rated == o.rated;
}

bool RoomData::operator==(const RoomData &o) const
{
	return name == o.name && type == o.type && area == o.area && length == o.length
		   && accesses == o.accesses && counts == o.counts;
}

bool Board::operator==(const Board &o) const
{
	return id == o.id && name == o.name && element == o.element
		   && feeder == o.feeder && source == o.source
		   && sourceMaxCurrent == o.sourceMaxCurrent
		   && sourceMinCurrent == o.sourceMinCurrent
		   && sourceReason == o.sourceReason
		   && ownTransformer == o.ownTransformer
		   && dwellingArea == o.dwellingArea && plannedHeating == o.plannedHeating
		   && plannedAirConditioning == o.plannedAirConditioning
		   && plannedAutomation == o.plannedAutomation
		   && plannedVehicleCharging == o.plannedVehicleCharging
		   && singleFamily == o.singleFamily && generalSwitch == o.generalSwitch
		   && residualDevices == o.residualDevices
		   && plannedDryer == o.plannedDryer && rooms == o.rooms
		   && compensateWithIndividualLine == o.compensateWithIndividualLine
		   && earthing == o.earthing && earthResistance == o.earthResistance
		   && earthResistanceReason == o.earthResistanceReason
		   && limitVoltage == o.limitVoltage && limitVoltageReason == o.limitVoltageReason
		   && firstFaultCurrent == o.firstFaultCurrent && firstFaultReason == o.firstFaultReason
		   && massesInterconnected == o.massesInterconnected
		   && neutralDistributed == o.neutralDistributed;
}

bool PvStringGroup::operator==(const PvStringGroup &o) const
{
	return name == o.name && modulesPerString == o.modulesPerString
		   && parallelStrings == o.parallelStrings && stringFuse == o.stringFuse
		   && cableLength == o.cableLength && cableSection == o.cableSection
		   && cableMaterial == o.cableMaterial && conductorTemperature == o.conductorTemperature
		   && cableAdmissible == o.cableAdmissible
		   && cableAdmissibleReason == o.cableAdmissibleReason;
}

bool PvData::isEmpty() const
{
	return *this == PvData();
}

bool PvData::operator==(const PvData &o) const
{
	return moduleName == o.moduleName && openCircuitVoltage == o.openCircuitVoltage
		   && mppVoltage == o.mppVoltage && shortCircuitCurrent == o.shortCircuitCurrent
		   && mppCurrent == o.mppCurrent && vocCoefficient == o.vocCoefficient
		   && vmppCoefficient == o.vmppCoefficient && maxSystemVoltage == o.maxSystemVoltage
		   && maxSeriesFuse == o.maxSeriesFuse && moduleSource == o.moduleSource
		   && inverterName == o.inverterName && maxDcVoltage == o.maxDcVoltage
		   && mpptMinVoltage == o.mpptMinVoltage && mpptMaxVoltage == o.mpptMaxVoltage
		   && maxInputCurrent == o.maxInputCurrent
		   && maxShortCircuitCurrent == o.maxShortCircuitCurrent
		   && inverterSource == o.inverterSource && minCellTemperature == o.minCellTemperature
		   && maxCellTemperature == o.maxCellTemperature
		   && temperatureReason == o.temperatureReason && strings == o.strings;
}

bool Circuit::operator==(const Circuit &o) const
{
	return id == o.id && name == o.name && board == o.board
		   && origin == o.origin && destination == o.destination
		   && system == o.system && voltage == o.voltage && use == o.use
		   && dwellingCircuit == o.dwellingCircuit && points == o.points
		   && load == o.load && conductor == o.conductor
		   && protection == o.protection && notes == o.notes && pv == o.pv;
}

bool ReportInfo::isEmpty() const
{
	return company.isEmpty() && documentTitle.isEmpty() && planNumber.isEmpty()
		   && revision.isEmpty() && date.isEmpty() && author.isEmpty()
		   && logo.isEmpty();
}

bool ReportInfo::operator==(const ReportInfo &o) const
{
	return company == o.company && documentTitle == o.documentTitle
		   && planNumber == o.planNumber && revision == o.revision
		   && date == o.date && author == o.author && logo == o.logo;
}

bool ElectricalModel::isEmpty() const
{
	return m_boards.isEmpty() && m_circuits.isEmpty() && m_report.isEmpty()
		   && m_newer.isEmpty();
}

bool ElectricalModel::setReportInfo(const ReportInfo &info)
{
	if (isReadOnly())
		return false;
	m_report = info;
	return true;
}

bool ElectricalModel::setBoard(const Board &board)
{
	if (isReadOnly() || board.id.isNull())
		return false;
	const int i = indexOf(m_boards, board.id);
	if (i < 0)
		m_boards.append(board);
	else
		m_boards[i] = board;
	return true;
}

bool ElectricalModel::setCircuit(const Circuit &circuit)
{
	if (isReadOnly() || circuit.id.isNull())
		return false;
	const int i = indexOf(m_circuits, circuit.id);
	if (i < 0)
		m_circuits.append(circuit);
	else
		m_circuits[i] = circuit;
	return true;
}

bool ElectricalModel::removeBoard(const QUuid &id)
{
	const int i = indexOf(m_boards, id);
	if (isReadOnly() || i < 0)
		return false;
	m_boards.removeAt(i);
	return true;
}

bool ElectricalModel::removeCircuit(const QUuid &id)
{
	const int i = indexOf(m_circuits, id);
	if (isReadOnly() || i < 0)
		return false;
	m_circuits.removeAt(i);
	return true;
}

std::optional<Board> ElectricalModel::board(const QUuid &id) const
{
	const int i = indexOf(m_boards, id);
	if (i < 0)
		return std::nullopt;
	return m_boards.at(i);
}

std::optional<Circuit> ElectricalModel::circuit(const QUuid &id) const
{
	const int i = indexOf(m_circuits, id);
	if (i < 0)
		return std::nullopt;
	return m_circuits.at(i);
}

QSet<QUuid> ElectricalModel::referencedItems() const
{
	QSet<QUuid> items;
	for (const Board &b : m_boards)
		items << b.element;
	for (const Circuit &c : m_circuits) {
		items << c.origin << c.destination << c.protection.element;
		for (const QUuid &wire : c.conductor.wires)
			items << wire;
	}
	items.remove(QUuid());
	return items;
}

QSet<QUuid> ElectricalModel::danglingItems(const QSet<QUuid> &existing) const
{
	return referencedItems().subtract(existing);
}

QList<QUuid> ElectricalModel::orphanCircuits() const
{
	QList<QUuid> orphans;
	for (const Circuit &c : m_circuits)
		if (indexOf(m_boards, c.board) < 0)
			orphans << c.id;
	return orphans;
}

QDomElement ElectricalModel::toXml(QDomDocument &document) const
{
	if (isReadOnly()) {
		QDomDocument newer;
		newer.setContent(m_newer);
		return document.importNode(newer.documentElement(), true).toElement();
	}

	QDomElement root = document.createElement(tagName());
	root.setAttribute(QStringLiteral("version"), version);

	if (!m_report.isEmpty()) {
		QDomElement e = document.createElement(QStringLiteral("report"));
		Writer w(e);
		w.text("company", m_report.company);
		w.text("documentTitle", m_report.documentTitle);
		w.text("planNumber", m_report.planNumber);
		w.text("revision", m_report.revision);
		w.text("date", m_report.date);
		w.text("author", m_report.author);
		if (!m_report.logo.isEmpty()) {
				//In the project, not a path: the file goes anywhere with it
			QDomElement logo = document.createElement(QStringLiteral("logo"));
			logo.setAttribute(QStringLiteral("format"), QStringLiteral("png"));
			logo.appendChild(document.createTextNode(
				QString::fromLatin1(m_report.logo.toBase64())));
			e.appendChild(logo);
		}
		root.appendChild(e);
	}

	for (const Board &b : m_boards) {
		QDomElement e = document.createElement(QStringLiteral("board"));
		Writer w(e);
		w.uuid("id", b.id);
		w.text("name", b.name);
		w.uuid("element", b.element);
		w.uuid("feeder", b.feeder);
		w.word("source", SOURCES, b.source);
		w.number("sourceMaxCurrent", b.sourceMaxCurrent);
		w.number("sourceMinCurrent", b.sourceMinCurrent);
		w.text("sourceReason", b.sourceReason);
		w.text("ownTransformer", b.ownTransformer ? QStringLiteral("true") : QString());
		w.word("earthing", EARTHINGS, b.earthing);
		w.number("earthResistance", b.earthResistance);
		w.text("earthResistanceReason", b.earthResistanceReason);
		w.number("limitVoltage", b.limitVoltage);
		w.text("limitVoltageReason", b.limitVoltageReason);
		w.number("firstFaultCurrent", b.firstFaultCurrent);
		w.text("firstFaultReason", b.firstFaultReason);
		w.word("massesInterconnected", BOOLS, b.massesInterconnected);
		w.word("neutralDistributed", BOOLS, b.neutralDistributed);
		w.number("dwellingArea", b.dwellingArea);
		w.word("plannedHeating", BOOLS, b.plannedHeating);
		w.word("plannedAirConditioning", BOOLS, b.plannedAirConditioning);
		w.word("plannedAutomation", BOOLS, b.plannedAutomation);
		w.word("plannedVehicleCharging", BOOLS, b.plannedVehicleCharging);
		w.word("singleFamily", BOOLS, b.singleFamily);
		w.number("generalSwitch", b.generalSwitch);
		w.word("plannedDryer", BOOLS, b.plannedDryer);
		w.text("compensateWithIndividualLine",
			   b.compensateWithIndividualLine ? QStringLiteral("true") : QString());
		for (const RoomData &room : b.rooms) {
			QDomElement re = document.createElement(QStringLiteral("room"));
			Writer rw(re);
			rw.text("name", room.name);
			rw.word("type", ROOM_TYPES, room.type);
			rw.number("area", room.area);
			rw.number("length", room.length);
			rw.integer("accesses", room.accesses);
			for (rebt::RoomItem item : ROOM_ITEMS)
				if (room.counts.contains(item))
					rw.integer(rebt::roomItemField(item), room.counts.value(item));
			e.appendChild(re);
		}
		for (const ResidualDeviceData &d : b.residualDevices) {
			QDomElement rd = document.createElement(QStringLiteral("residualDevice"));
			Writer rw(rd);
			rw.text("name", d.name);
			rw.number("sensitivity", d.sensitivity);
			rw.number("rated", d.rated);
			e.appendChild(rd);
		}
		root.appendChild(e);
	}

	for (const Circuit &c : m_circuits) {
		QDomElement e = document.createElement(QStringLiteral("circuit"));
		Writer w(e);
		w.uuid("id", c.id);
		w.text("name", c.name);
		w.uuid("board", c.board);
		w.uuid("origin", c.origin);
		w.uuid("destination", c.destination);
		w.word("system", SYSTEMS, c.system);
		w.number("voltage", c.voltage);
		w.word("use", USES, c.use);
		w.word("dwellingCircuit", DWELLING_CIRCUITS, c.dwellingCircuit);
		w.integer("points", c.points);

		QDomElement load = document.createElement(QStringLiteral("load"));
		Writer lw(load);
		lw.number("power", c.load.power);
		lw.number("powerFactor", c.load.powerFactor);
		lw.number("efficiency", c.load.efficiency);
		lw.number("designCurrent", c.load.designCurrent);
		if (load.attributes().count())
			e.appendChild(load);

		const ConductorData &cd = c.conductor;
		QDomElement cond = document.createElement(QStringLiteral("conductor"));
		Writer cw(cond);
		cw.word("material", MATERIALS, cd.material);
		cw.word("insulation", INSULATIONS, cd.insulation);
		cw.text("installationMethod", cd.installationMethod);
		cw.number("section", cd.section);
		cw.number("neutralSection", cd.neutralSection);
		cw.number("peSection", cd.peSection);
		cw.number("length", cd.length);
		cw.number("temperature", cd.temperature);
		cw.number("conductivity", cd.conductivity);
		cw.number("reactance", cd.reactance);
		cw.number("conduit", cd.conduit);
		cw.integer("loadedConductors", cd.loadedConductors);
		cw.number("ambientTemperature", cd.ambientTemperature);
		cw.number("temperatureFactor", cd.temperatureFactor);
		cw.text("temperatureFactorReason", cd.temperatureFactorReason);
		cw.number("groupingFactor", cd.groupingFactor);
		cw.text("groupingFactorReason", cd.groupingFactorReason);
		cw.number("otherFactor", cd.otherFactor);
		cw.text("otherFactorReason", cd.otherFactorReason);
		for (const QUuid &wire : cd.wires) {
			QDomElement we = document.createElement(QStringLiteral("wire"));
			we.setAttribute(QStringLiteral("uuid"), wire.toString());
			cond.appendChild(we);
		}
		if (cond.attributes().count() || cond.hasChildNodes())
			e.appendChild(cond);

		const ProtectionData &pd = c.protection;
		QDomElement prot = document.createElement(QStringLiteral("protection"));
		Writer pw(prot);
		pw.uuid("element", pd.element);
		pw.text("manufacturer", pd.manufacturer);
		pw.text("partNumber", pd.partNumber);
		pw.word("type", PROTECTIONS, pd.type);
		pw.number("ratedCurrent", pd.ratedCurrent);
		pw.text("curve", pd.curve);
		pw.number("breakingCapacity", pd.breakingCapacity);
		pw.number("sensitivity", pd.sensitivity);
		pw.integer("poles", pd.poles);
		pw.word("standard", STANDARDS, pd.standard);
		pw.number("conventionalCurrent", pd.conventionalCurrent);
		pw.text("conventionalCurrentReason", pd.conventionalCurrentReason);
		pw.text("residualDevice", pd.residualDevice);
		pw.number("tripCurrent", pd.tripCurrent);
		pw.text("tripCurrentReason", pd.tripCurrentReason);
		pw.number("letThrough", pd.letThrough);
		pw.number("letThroughAt", pd.letThroughAt);
		pw.text("letThroughReason", pd.letThroughReason);
		if (prot.attributes().count())
			e.appendChild(prot);

		if (!c.pv.isEmpty()) {
			const PvData &p = c.pv;
			QDomElement pv = document.createElement(QStringLiteral("pv"));
			Writer vw(pv);
			vw.text("moduleName", p.moduleName);
			vw.number("openCircuitVoltage", p.openCircuitVoltage);
			vw.number("mppVoltage", p.mppVoltage);
			vw.number("shortCircuitCurrent", p.shortCircuitCurrent);
			vw.number("mppCurrent", p.mppCurrent);
			vw.number("vocCoefficient", p.vocCoefficient);
			vw.number("vmppCoefficient", p.vmppCoefficient);
			vw.number("maxSystemVoltage", p.maxSystemVoltage);
			vw.number("maxSeriesFuse", p.maxSeriesFuse);
			vw.text("moduleSource", p.moduleSource);
			vw.text("inverterName", p.inverterName);
			vw.number("maxDcVoltage", p.maxDcVoltage);
			vw.number("mpptMinVoltage", p.mpptMinVoltage);
			vw.number("mpptMaxVoltage", p.mpptMaxVoltage);
			vw.number("maxInputCurrent", p.maxInputCurrent);
			vw.number("maxShortCircuitCurrent", p.maxShortCircuitCurrent);
			vw.text("inverterSource", p.inverterSource);
			vw.number("minCellTemperature", p.minCellTemperature);
			vw.number("maxCellTemperature", p.maxCellTemperature);
			vw.text("temperatureReason", p.temperatureReason);
			for (const PvStringGroup &g : p.strings) {
				QDomElement ge = document.createElement(QStringLiteral("pvStrings"));
				Writer gw(ge);
				gw.text("name", g.name);
				gw.integer("modulesPerString", g.modulesPerString);
				gw.integer("parallelStrings", g.parallelStrings);
				gw.number("stringFuse", g.stringFuse);
				gw.number("cableLength", g.cableLength);
				gw.number("cableSection", g.cableSection);
				gw.word("cableMaterial", MATERIALS, g.cableMaterial);
				gw.number("conductorTemperature", g.conductorTemperature);
				gw.number("cableAdmissible", g.cableAdmissible);
				gw.text("cableAdmissibleReason", g.cableAdmissibleReason);
				pv.appendChild(ge);
			}
			e.appendChild(pv);
		}

		if (!c.notes.isEmpty()) {
			QDomElement notes = document.createElement(QStringLiteral("notes"));
			notes.appendChild(document.createTextNode(c.notes));
			e.appendChild(notes);
		}
		root.appendChild(e);
	}
	return root;
}

ReadReport ElectricalModel::fromXml(const QDomElement &element)
{
	m_boards.clear();
	m_circuits.clear();
	m_report = ReportInfo();
	m_newer.clear();

	ReadReport report;
	bool ok = false;
	report.version = element.attribute(QStringLiteral("version")).toInt(&ok);
	if (!ok || report.version > version || report.version < 1) {
			//Not a version this program knows how to read: keep it all
		report.tooNew = ok && report.version > version;
		if (!ok || report.version < 1)
			report.badValues << QStringLiteral("electrical@version");
		QTextStream stream(&m_newer);
		element.save(stream, -1);
		return report;
	}

	{
		Reader r(element.firstChildElement(QStringLiteral("report")),
				 QStringLiteral("report"), report);
		m_report.company = r.text("company");
		m_report.documentTitle = r.text("documentTitle");
		m_report.planNumber = r.text("planNumber");
		m_report.revision = r.text("revision");
		m_report.date = r.text("date");
		m_report.author = r.text("author");
		const QDomElement logo = element.firstChildElement(QStringLiteral("report"))
									 .firstChildElement(QStringLiteral("logo"));
		if (!logo.isNull()) {
			const auto decoded = QByteArray::fromBase64Encoding(
				logo.text().trimmed().toLatin1(), QByteArray::AbortOnBase64DecodingErrors);
			if (decoded && !decoded.decoded.isEmpty())
				m_report.logo = decoded.decoded;
			else
				report.badValues << QStringLiteral("report/logo");
		}
	}

	int index = 0;
	for (QDomElement e = element.firstChildElement(QStringLiteral("board"));
		 !e.isNull();
		 e = e.nextSiblingElement(QStringLiteral("board")), ++index)
	{
		Reader r(e, QStringLiteral("board[%1]").arg(index), report);
		Board b;
		b.id = r.uuid("id");
		b.name = r.text("name");
		b.element = r.uuid("element");
		b.feeder = r.uuid("feeder");
		b.source = r.word("source", SOURCES);
		b.sourceMaxCurrent = r.number("sourceMaxCurrent");
		b.sourceMinCurrent = r.number("sourceMinCurrent");
		b.sourceReason = r.text("sourceReason");
		{
			const QString own = r.text("ownTransformer");
			b.ownTransformer = own == QLatin1String("true");
			if (!own.isEmpty() && !b.ownTransformer)
				r.bad("ownTransformer");
		}
		b.earthing = r.word("earthing", EARTHINGS);
		b.earthResistance = r.number("earthResistance");
		b.earthResistanceReason = r.text("earthResistanceReason");
		b.limitVoltage = r.number("limitVoltage");
		b.limitVoltageReason = r.text("limitVoltageReason");
		b.firstFaultCurrent = r.number("firstFaultCurrent");
		b.firstFaultReason = r.text("firstFaultReason");
		b.massesInterconnected = r.word("massesInterconnected", BOOLS);
		b.neutralDistributed = r.word("neutralDistributed", BOOLS);
		b.dwellingArea = r.number("dwellingArea");
		b.plannedHeating = r.word("plannedHeating", BOOLS);
		b.plannedAirConditioning = r.word("plannedAirConditioning", BOOLS);
		b.plannedAutomation = r.word("plannedAutomation", BOOLS);
		b.plannedVehicleCharging = r.word("plannedVehicleCharging", BOOLS);
		b.singleFamily = r.word("singleFamily", BOOLS);
		b.generalSwitch = r.number("generalSwitch");
		b.plannedDryer = r.word("plannedDryer", BOOLS);
		{
			const QString comp = r.text("compensateWithIndividualLine");
			b.compensateWithIndividualLine = comp == QLatin1String("true");
			if (!comp.isEmpty() && !b.compensateWithIndividualLine)
				r.bad("compensateWithIndividualLine");
		}
		{
			int room_index = 0;
			for (QDomElement re = e.firstChildElement(QStringLiteral("room"));
				 !re.isNull();
				 re = re.nextSiblingElement(QStringLiteral("room")), ++room_index) {
				Reader rr(re, QStringLiteral("board[%1]/room[%2]")
								  .arg(index).arg(room_index), report);
				RoomData room;
				room.name = rr.text("name");
				room.type = rr.word("type", ROOM_TYPES);
				room.area = rr.number("area");
				room.length = rr.number("length");
				room.accesses = rr.integer("accesses");
				for (rebt::RoomItem item : ROOM_ITEMS)
					if (const auto n = rr.integer(rebt::roomItemField(item)))
						room.counts.insert(item, *n);
				b.rooms << room;
			}
		}
		{
			int rd_index = 0;
			for (QDomElement rd = e.firstChildElement(QStringLiteral("residualDevice"));
				 !rd.isNull();
				 rd = rd.nextSiblingElement(QStringLiteral("residualDevice")), ++rd_index) {
				Reader rr(rd, QStringLiteral("board[%1]/residualDevice[%2]")
								  .arg(index).arg(rd_index), report);
				ResidualDeviceData d;
				d.name = rr.text("name");
				d.sensitivity = rr.number("sensitivity");
				d.rated = rr.number("rated");
				b.residualDevices << d;
			}
		}
		if (b.id.isNull() || indexOf(m_boards, b.id) >= 0) {
			r.bad("id");
			continue;
		}
		m_boards.append(b);
	}

	index = 0;
	for (QDomElement e = element.firstChildElement(QStringLiteral("circuit"));
		 !e.isNull();
		 e = e.nextSiblingElement(QStringLiteral("circuit")), ++index)
	{
		const QString path = QStringLiteral("circuit[%1]").arg(index);
		Reader r(e, path, report);
		Circuit c;
		c.id = r.uuid("id");
		if (c.id.isNull() || indexOf(m_circuits, c.id) >= 0) {
			r.bad("id");
			continue;
		}
		c.name = r.text("name");
		c.board = r.uuid("board");
		c.origin = r.uuid("origin");
		c.destination = r.uuid("destination");
		c.system = r.word("system", SYSTEMS);
		c.voltage = r.number("voltage");
		c.use = r.word("use", USES);
		c.dwellingCircuit = r.word("dwellingCircuit", DWELLING_CIRCUITS);
		c.points = r.integer("points");

		Reader lr(e.firstChildElement(QStringLiteral("load")),
				  path + QStringLiteral("/load"), report);
		c.load.power = lr.number("power");
		c.load.powerFactor = lr.number("powerFactor");
		c.load.efficiency = lr.number("efficiency");
		c.load.designCurrent = lr.number("designCurrent");

		const QDomElement cond = e.firstChildElement(QStringLiteral("conductor"));
		Reader cr(cond, path + QStringLiteral("/conductor"), report);
		ConductorData &cd = c.conductor;
		cd.material = cr.word("material", MATERIALS);
		cd.insulation = cr.word("insulation", INSULATIONS);
		cd.installationMethod = cr.text("installationMethod");
		cd.section = cr.number("section");
		cd.neutralSection = cr.number("neutralSection");
		cd.peSection = cr.number("peSection");
		cd.length = cr.number("length");
		cd.temperature = cr.number("temperature");
		cd.conductivity = cr.number("conductivity");
		cd.reactance = cr.number("reactance");
		cd.conduit = cr.number("conduit");
		cd.loadedConductors = cr.integer("loadedConductors");
		cd.ambientTemperature = cr.number("ambientTemperature");
		cd.temperatureFactor = cr.number("temperatureFactor");
		cd.temperatureFactorReason = cr.text("temperatureFactorReason");
		cd.groupingFactor = cr.number("groupingFactor");
		cd.groupingFactorReason = cr.text("groupingFactorReason");
		cd.otherFactor = cr.number("otherFactor");
		cd.otherFactorReason = cr.text("otherFactorReason");
		int wire_index = 0;
		for (QDomElement we = cond.firstChildElement(QStringLiteral("wire"));
			 !we.isNull();
			 we = we.nextSiblingElement(QStringLiteral("wire")), ++wire_index)
		{
			Reader wr(we, path + QStringLiteral("/conductor/wire[%1]")
							  .arg(wire_index), report);
			const QUuid wire = wr.uuid("uuid");
			if (!wire.isNull())
				cd.wires << wire;
		}

		Reader pr(e.firstChildElement(QStringLiteral("protection")),
				  path + QStringLiteral("/protection"), report);
		ProtectionData &pd = c.protection;
		pd.element = pr.uuid("element");
		pd.manufacturer = pr.text("manufacturer");
		pd.partNumber = pr.text("partNumber");
		pd.type = pr.word("type", PROTECTIONS);
		pd.ratedCurrent = pr.number("ratedCurrent");
		pd.curve = pr.text("curve");
		pd.breakingCapacity = pr.number("breakingCapacity");
		pd.sensitivity = pr.number("sensitivity");
		pd.poles = pr.integer("poles");
		pd.standard = pr.word("standard", STANDARDS);
		pd.conventionalCurrent = pr.number("conventionalCurrent");
		pd.conventionalCurrentReason = pr.text("conventionalCurrentReason");
		pd.residualDevice = pr.text("residualDevice");
		pd.tripCurrent = pr.number("tripCurrent");
		pd.tripCurrentReason = pr.text("tripCurrentReason");
		pd.letThrough = pr.number("letThrough");
		pd.letThroughAt = pr.number("letThroughAt");
		pd.letThroughReason = pr.text("letThroughReason");

		const QDomElement pv = e.firstChildElement(QStringLiteral("pv"));
		if (!pv.isNull()) {
			Reader vr(pv, path + QStringLiteral("/pv"), report);
			PvData &p = c.pv;
			p.moduleName = vr.text("moduleName");
			p.openCircuitVoltage = vr.number("openCircuitVoltage");
			p.mppVoltage = vr.number("mppVoltage");
			p.shortCircuitCurrent = vr.number("shortCircuitCurrent");
			p.mppCurrent = vr.number("mppCurrent");
			p.vocCoefficient = vr.number("vocCoefficient");
			p.vmppCoefficient = vr.number("vmppCoefficient");
			p.maxSystemVoltage = vr.number("maxSystemVoltage");
			p.maxSeriesFuse = vr.number("maxSeriesFuse");
			p.moduleSource = vr.text("moduleSource");
			p.inverterName = vr.text("inverterName");
			p.maxDcVoltage = vr.number("maxDcVoltage");
			p.mpptMinVoltage = vr.number("mpptMinVoltage");
			p.mpptMaxVoltage = vr.number("mpptMaxVoltage");
			p.maxInputCurrent = vr.number("maxInputCurrent");
			p.maxShortCircuitCurrent = vr.number("maxShortCircuitCurrent");
			p.inverterSource = vr.text("inverterSource");
			p.minCellTemperature = vr.number("minCellTemperature");
			p.maxCellTemperature = vr.number("maxCellTemperature");
			p.temperatureReason = vr.text("temperatureReason");
			int g_index = 0;
			for (QDomElement ge = pv.firstChildElement(QStringLiteral("pvStrings"));
				 !ge.isNull();
				 ge = ge.nextSiblingElement(QStringLiteral("pvStrings")), ++g_index) {
				Reader gr(ge, path + QStringLiteral("/pv/pvStrings[%1]").arg(g_index), report);
				PvStringGroup g;
				g.name = gr.text("name");
				g.modulesPerString = gr.integer("modulesPerString");
				g.parallelStrings = gr.integer("parallelStrings");
				g.stringFuse = gr.number("stringFuse");
				g.cableLength = gr.number("cableLength");
				g.cableSection = gr.number("cableSection");
				g.cableMaterial = gr.word("cableMaterial", MATERIALS);
				g.conductorTemperature = gr.number("conductorTemperature");
				g.cableAdmissible = gr.number("cableAdmissible");
				g.cableAdmissibleReason = gr.text("cableAdmissibleReason");
				p.strings << g;
			}
		}

		c.notes = e.firstChildElement(QStringLiteral("notes")).text();
		m_circuits.append(c);
	}
	return report;
}

QString ElectricalModel::toXmlText() const
{
	if (isReadOnly())
		return m_newer;
	if (isEmpty())
		return QString();
	QDomDocument document;
	const QDomElement root = toXml(document);
	QString text;
	QTextStream stream(&text);
	root.save(stream, -1);
	return text;
}

bool ElectricalModel::operator==(const ElectricalModel &o) const
{
	return m_boards == o.m_boards && m_circuits == o.m_circuits
		   && m_report == o.m_report && m_newer == o.m_newer;
}

rebt::CircuitInput toCircuitInput(const Circuit &circuit)
{
	rebt::CircuitInput input;
	input.system = circuit.system;
	input.voltage = circuit.voltage;
	input.power = circuit.load.power;
	input.powerFactor = circuit.load.powerFactor;
	input.efficiency = circuit.load.efficiency;
	input.designCurrent = circuit.load.designCurrent;
	input.length = circuit.conductor.length;
	input.section = circuit.conductor.section;
	input.material = circuit.conductor.material;
	input.conductorTemperature = circuit.conductor.temperature;
	input.conductivity = circuit.conductor.conductivity;
	input.reactance = circuit.conductor.reactance;
	return input;
}

std::optional<rebt::TripCurve> tripCurve(const ProtectionData &protection)
{
	const QString curve = protection.curve.trimmed().toUpper();
	if (curve == QLatin1String("B"))
		return rebt::TripCurve::B;
	if (curve == QLatin1String("C"))
		return rebt::TripCurve::C;
	if (curve == QLatin1String("D"))
		return rebt::TripCurve::D;
	return std::nullopt;
}

rebt::LineStretch toLineStretch(const Circuit &circuit)
{
	const ConductorData &cd = circuit.conductor;
	rebt::LineStretch line;
	line.length = cd.length;
	line.section = cd.section;
	line.neutralSection = cd.neutralSection;
	line.protectiveSection = cd.peSection;
	line.material = cd.material;
	line.insulation = cd.insulation;
	line.reactance = cd.reactance;
		//Only a use known to be interior brings in ITC-BT-19 2.2.2
	line.interior = circuit.use && isInterior(*circuit.use);
	return line;
}

rebt::AmpacityInput toAmpacityInput(const Circuit &circuit)
{
	const ConductorData &cd = circuit.conductor;
	rebt::AmpacityInput input;
	input.system = circuit.system;
	input.material = cd.material;
	input.insulation = cd.insulation;
	input.loadedConductors = cd.loadedConductors;
	input.method = cd.installationMethod.toStdString();
	input.section = cd.section;
	input.ambient = cd.ambientTemperature;
	input.temperatureFactor = cd.temperatureFactor;
	input.temperatureJustification = cd.temperatureFactorReason.toStdString();
	input.groupingFactor = cd.groupingFactor;
	input.groupingJustification = cd.groupingFactorReason.toStdString();
	input.otherFactor = cd.otherFactor;
	input.otherJustification = cd.otherFactorReason.toStdString();
	return input;
}

} // namespace electrical
