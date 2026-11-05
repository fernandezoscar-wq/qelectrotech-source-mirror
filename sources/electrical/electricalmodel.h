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
#ifndef ELECTRICALMODEL_H
#define ELECTRICALMODEL_H

#include "rebt/rebtampacity.h"
#include "rebt/rebtdwelling.h"
#include "rebt/rebtindirect.h"
#include "rebt/rebtprotection.h"
#include "rebt/rebtshortcircuit.h"
#include "rebt/rebttypes.h"

#include <QByteArray>
#include <QDomDocument>
#include <QList>
#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QUuid>

#include <optional>

/**
	@brief The electrical model of a project: boards and the circuits
	leaving them, each with its load, conductor and protection.

	It holds data only. The symbols drawn for it are referred to by
	uuid (Element::uuid(), Conductor::uuid()), so the drawing and the
	model never hold copies of each other.

	Every value may be unknown, and an unknown value is never replaced
	with a default: it stays empty in memory and is not written to the
	file.

	The model is written as <electrical version="1"> in the project.
	A file from a newer version is kept as it is and the model is then
	read only, so its data is never overwritten with less.
*/
namespace electrical {

enum class CircuitUse {
	DwellingInterior,   ///< any interior circuit of a dwelling
	Lighting,           ///< lighting, other installations
	OtherUses,          ///< other uses, other installations
		// Service connection: their drop is not part of the interior one
	LgaTotallyCentralized,  ///< LGA, meters totally centralized
	LgaPartlyCentralized,   ///< LGA, meters partly centralized
	DiMetersInSeveralPlaces,///< DI, meters grouped in more than one place
	DiMetersTotallyGrouped, ///< DI, meters totally grouped
	DiSingleUser,           ///< DI of a single user, no LGA
		/// Line of a generator (an inverter) to its point of connection:
		/// its own drop, at most 1.5 %, cables for 125 % (ITC-BT-40 5)
	GeneratorLine
};

/// Part of the interior installation: its drop adds up from the origin
bool isInterior(CircuitUse use);

/// The engine's: its highest temperature sets the temperature factor
using Insulation = rebt::Insulation;

enum class ProtectionType {
	CircuitBreaker,       ///< magnetotermico
	ResidualCurrent,      ///< interruptor diferencial
	Rcbo,                 ///< magnetotermico diferencial
	Fuse,
	Switch
};

struct Load
{
	std::optional<double> power;          ///< W
	std::optional<double> powerFactor;
	std::optional<double> efficiency;
	std::optional<double> designCurrent;  ///< A, when known instead of power

	bool operator==(const Load &other) const;
};

struct ConductorData
{
	std::optional<rebt::Material> material;
	std::optional<Insulation> insulation;
	QString installationMethod;           ///< reference method, e.g. "B1"
	std::optional<double> section;        ///< mm2, phase
	std::optional<double> neutralSection; ///< mm2
	std::optional<double> peSection;      ///< mm2
	std::optional<double> length;         ///< m
	std::optional<double> temperature;    ///< degC, conductor
	std::optional<double> conductivity;   ///< m/(ohm.mm2)
	std::optional<double> reactance;      ///< ohm/km
	std::optional<double> conduit;        ///< mm, outer diameter of its conduit
	QList<QUuid> wires;                   ///< conductors of the drawing
		// Admissible current (ITC-BT-19 2.2.3)
	std::optional<int> loadedConductors;  ///< 2 or 3, else from the system
	std::optional<double> ambientTemperature;  ///< degC
	std::optional<double> temperatureFactor;   ///< given instead of computed
	QString temperatureFactorReason;
	std::optional<double> groupingFactor;      ///< 1 for a lone circuit
	QString groupingFactorReason;
	std::optional<double> otherFactor;
	QString otherFactorReason;

	bool operator==(const ConductorData &other) const;
};

struct ProtectionData
{
	QUuid element;                        ///< symbol of the drawing
	QString manufacturer;                 ///< of the device, as the user writes it
	QString partNumber;                   ///< reference of the manufacturer
	std::optional<ProtectionType> type;
	std::optional<double> ratedCurrent;   ///< A
	QString curve;                        ///< "B", "C", "D", "gG"...
	std::optional<double> breakingCapacity; ///< kA
	std::optional<double> sensitivity;    ///< mA
	std::optional<int> poles;
	std::optional<rebt::ProtectionStandard> standard;   ///< sets I2
	std::optional<double> conventionalCurrent;   ///< I2 given, A
	QString conventionalCurrentReason;
		/// Name of the residual current device of the board that
		/// protects the circuit; an RCBO protects it by itself
	QString residualDevice;
		// Short circuit, when the standard does not give them
	std::optional<double> tripCurrent;    ///< A, instantaneous trip
	QString tripCurrentReason;
	std::optional<double> letThrough;     ///< A2s at the maximum Icc
	std::optional<double> letThroughAt;   ///< kA, the prospective current of that I2t
	QString letThroughReason;

	bool operator==(const ProtectionData &other) const;
};

/// A residual current device of a board, that circuits refer to by name
struct ResidualDeviceData
{
	QString name;                          ///< "ID1"
	std::optional<double> sensitivity;     ///< mA
	std::optional<double> rated;           ///< A

	bool operator==(const ResidualDeviceData &other) const;
};

/// A room of a dwelling and what it has (table 2 of ITC-BT-25)
struct RoomData
{
	QString name;
	std::optional<rebt::RoomType> type;
	std::optional<double> area;        ///< m2
	std::optional<double> length;      ///< m, corridors
	std::optional<int> accesses;       ///< corridors
	QMap<rebt::RoomItem, int> counts;  ///< those given; missing: unknown

	bool operator==(const RoomData &other) const;
};

struct Board
{
	QUuid id;
	QString name;
	QUuid element;                        ///< symbol of the drawing
	QUuid feeder;                         ///< Circuit::id that supplies it, none: origin
		// Supply, for a board fed by nothing (short circuit)
	std::optional<rebt::SourceMethod> source;
	std::optional<double> sourceMaxCurrent;   ///< kA at the board
	std::optional<double> sourceMinCurrent;   ///< kA at the board
	QString sourceReason;                     ///< where the values come from
		/// Industrial installation fed in HV through its own transformer:
		/// the origin is its output (ITC-BT-19 2.2.2, 4.5 % and 6.5 %)
	bool ownTransformer = false;
		// Dwelling whose interior installation starts here (ITC-BT-25)
	std::optional<double> dwellingArea;       ///< m2, useful
	std::optional<bool> plannedHeating;       ///< electric heating
	std::optional<bool> plannedAirConditioning;
	std::optional<bool> plannedAutomation;
	std::optional<bool> plannedVehicleCharging;   ///< EV parking places
	std::optional<bool> singleFamily;         ///< vivienda unifamiliar
	std::optional<double> generalSwitch;      ///< A, IGA
	QList<ResidualDeviceData> residualDevices;    ///< of this board
	std::optional<bool> plannedDryer;         ///< independent dryer (C10)
	QList<RoomData> rooms;                    ///< of the dwelling
		/// Drop compensated with the DI that feeds this board (ITC-BT-19 2.2.2)
	bool compensateWithIndividualLine = false;
		// Earthing of the installation from this board on (ITC-BT-08, ITC-BT-24);
		// a board with none takes those of the board that feeds it
	std::optional<rebt::EarthingSystem> earthing;
	std::optional<double> earthResistance;    ///< RA, ohm: electrode and protective conductors
	QString earthResistanceReason;            ///< measured, estimated per ITC-BT-18...
	std::optional<double> limitVoltage;       ///< U, V; empty: 50 V
	QString limitVoltageReason;
		// IT (ITC-BT-24 4.1.3)
	std::optional<double> firstFaultCurrent;  ///< Id, A
	QString firstFaultReason;
	std::optional<bool> massesInterconnected; ///< masses joined by the PE, one earthing
	std::optional<bool> neutralDistributed;

	bool operator==(const Board &other) const;
};

/**
	@brief One group of equal strings in parallel on one input (MPPT) of
	the inverter, with its string cable
*/
struct PvStringGroup
{
	QString name;                          ///< "MPPT1"
	std::optional<int> modulesPerString;
	std::optional<int> parallelStrings;
	std::optional<double> stringFuse;      ///< A, In of the string protection
	std::optional<double> cableLength;     ///< m, one way, farthest module
	std::optional<double> cableSection;    ///< mm2
	std::optional<rebt::Material> cableMaterial;
	std::optional<double> conductorTemperature;   ///< degC
	std::optional<double> cableAdmissible; ///< Iz of the string cable, A
	QString cableAdmissibleReason;         ///< table and factors it comes from

	bool operator==(const PvStringGroup &other) const;
};

/**
	@brief The photovoltaic generator behind an inverter: its module, the
	DC input of the inverter, the site and the strings. Held by the
	circuit of the inverter's AC line.
*/
struct PvData
{
	QString moduleName;                    ///< manufacturer and model
	std::optional<double> openCircuitVoltage;    ///< Voc, V, STC
	std::optional<double> mppVoltage;            ///< Vmpp, V
	std::optional<double> shortCircuitCurrent;   ///< Isc, A
	std::optional<double> mppCurrent;            ///< Impp, A
	std::optional<double> vocCoefficient;        ///< %/degC
	std::optional<double> vmppCoefficient;       ///< %/degC
	std::optional<double> maxSystemVoltage;      ///< V
	std::optional<double> maxSeriesFuse;         ///< A
	QString moduleSource;                  ///< datasheet
	QString inverterName;
	std::optional<double> maxDcVoltage;          ///< V
	std::optional<double> mpptMinVoltage;        ///< V
	std::optional<double> mpptMaxVoltage;        ///< V
	std::optional<double> maxInputCurrent;       ///< A, per MPPT
	std::optional<double> maxShortCircuitCurrent;///< A, per MPPT
	QString inverterSource;
	std::optional<double> minCellTemperature;    ///< degC
	std::optional<double> maxCellTemperature;    ///< degC
	QString temperatureReason;
	QList<PvStringGroup> strings;

	bool isEmpty() const;
	bool operator==(const PvData &other) const;
};

struct Circuit
{
	QUuid id;
	QString name;
	QUuid board;                          ///< Board::id it leaves from
	QUuid origin;                         ///< element it starts at
	QUuid destination;                    ///< element it ends at
	std::optional<rebt::System> system;
	std::optional<double> voltage;        ///< V
	std::optional<CircuitUse> use;
		// Dwelling (ITC-BT-25): which circuit of table 1, how many points
	std::optional<rebt::DwellingCircuit> dwellingCircuit;
	std::optional<int> points;
	Load load;
	ConductorData conductor;
	ProtectionData protection;
	QString notes;                        ///< hypotheses, pending data
	PvData pv;                            ///< the PV generator of a generator line

	bool operator==(const Circuit &other) const;
};

/**
	@brief What the title block of the calculation report shows.
	Empty fields are left blank in the report: none is made up.
*/
struct ReportInfo
{
	QString company;         ///< who signs the document
	QString documentTitle;   ///< e.g. "Anexo de calculo electrico"
	QString planNumber;
	QString revision;
	QString date;            ///< as the user writes it
	QString author;
	QByteArray logo;         ///< PNG of the company logo, empty: none

	bool isEmpty() const;
	bool operator==(const ReportInfo &other) const;
};

/**
	@brief What reading a model found
*/
struct ReadReport
{
	int version = 0;
	bool tooNew = false;          ///< version newer than this program
	QStringList badValues;        ///< "circuit[2]/load@power": not read
};

class ElectricalModel
{
	public:
		static const int version = 1;
		static QString tagName() { return QStringLiteral("electrical"); }

		bool isEmpty() const;
		bool isReadOnly() const { return !m_newer.isEmpty(); }

		const QList<Board> &boards() const { return m_boards; }
		const QList<Circuit> &circuits() const { return m_circuits; }

		/// Add or replace (same id). False when read only or id is null.
		bool setBoard(const Board &board);
		bool setCircuit(const Circuit &circuit);
		bool removeBoard(const QUuid &id);
		bool removeCircuit(const QUuid &id);

		const ReportInfo &reportInfo() const { return m_report; }
			/// False when read only
		bool setReportInfo(const ReportInfo &info);

		std::optional<Board> board(const QUuid &id) const;
		std::optional<Circuit> circuit(const QUuid &id) const;

		/// Every element and conductor uuid the model refers to
		QSet<QUuid> referencedItems() const;
		/// Those of @p existing the model refers to but are not drawn
		QSet<QUuid> danglingItems(const QSet<QUuid> &existing) const;
		/// Circuits whose board id is not a board of the model
		QList<QUuid> orphanCircuits() const;

		QDomElement toXml(QDomDocument &document) const;
		ReadReport fromXml(const QDomElement &element);
		/// The model as the text of its XML, empty when the model is
		QString toXmlText() const;

		bool operator==(const ElectricalModel &other) const;
		bool operator!=(const ElectricalModel &other) const
		{ return !(*this == other); }

	private:
		QList<Board> m_boards;
		QList<Circuit> m_circuits;
		ReportInfo m_report;
		/// The XML of a newer version, kept as it is
		QString m_newer;
};

/// The data of @p circuit as input for the calculations
rebt::CircuitInput toCircuitInput(const Circuit &circuit);
/// The curve of @p protection as B, C or D, nothing for any other text
std::optional<rebt::TripCurve> tripCurve(const ProtectionData &protection);
/// The data of @p circuit as one line of a fault path
rebt::LineStretch toLineStretch(const Circuit &circuit);
/// The data of @p circuit as input for its admissible current
rebt::AmpacityInput toAmpacityInput(const Circuit &circuit);

} // namespace electrical

#endif // ELECTRICALMODEL_H
