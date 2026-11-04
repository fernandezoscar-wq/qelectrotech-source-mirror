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
#ifndef REBTAMPACITY_H
#define REBTAMPACITY_H

#include "rebttypes.h"

/**
	@brief Admissible current of a conductor (ITC-BT-19 2.2.3) and the
	smallest section that carries a circuit.

	The program holds no ampacity table: the tables of UNE-HD 60364-5-52
	are copyrighted. They are loaded from a file the user provides
	(AmpacityLibrary), and every result says which table it read.
*/
namespace rebt {

struct AmpacityEntry
{
	double section;   ///< mm2
	double current;   ///< A, at the reference conditions of the table
};

/**
	@brief One column of an ampacity table: one material, insulation,
	number of loaded conductors and installation method
*/
struct AmpacityTable
{
	std::string id;             ///< as the user named it
	std::string source;         ///< where the values come from
	Material material = Material::Copper;
	Insulation insulation = Insulation::PVC;
	int loadedConductors = 2;
	std::string method;         ///< reference method, e.g. "B1"
	double referenceAmbient = 30.0;   ///< degC of the values
	std::vector<AmpacityEntry> entries;   ///< ascending sections

		/// The current of @p section, when it is a row of the table
	std::optional<double> currentAt(double section) const;
};

class AmpacityLibrary
{
	public:
		/// False, and nothing added, when a table for the same conditions is there
		bool add(const AmpacityTable &table);
		const std::vector<AmpacityTable> &tables() const { return m_tables; }
		bool empty() const { return m_tables.empty(); }
		const AmpacityTable *find(Material material, Insulation insulation,
								  int loaded_conductors,
								  const std::string &method) const;

	private:
		std::vector<AmpacityTable> m_tables;
};

enum class FactorKind {
	Temperature,
	Grouping,
	Other
};

struct CorrectionFactor
{
	FactorKind kind;
	double value;
	std::string justification;
	bool computed = false;   ///< worked out here, not given
};

/**
	@brief What the admissible current of a conductor depends on. Every
	field may be unknown. A factor given by the user needs its reason.
*/
struct AmpacityInput
{
	std::optional<System> system;          ///< for the loaded conductors
	std::optional<Material> material;
	std::optional<Insulation> insulation;
	std::optional<int> loadedConductors;   ///< 2 or 3
	std::string method;
	std::optional<double> section;         ///< mm2
	std::optional<double> ambient;         ///< degC, for the temperature factor
	std::optional<double> temperatureFactor;   ///< given instead of computed
	std::string temperatureJustification;
	std::optional<double> groupingFactor;  ///< always needed, 1 for a lone circuit
	std::string groupingJustification;
	std::optional<double> otherFactor;     ///< any other, when it applies
	std::string otherJustification;
};

struct AmpacityResult
{
	Status status = Status::NotComputable;
	std::optional<double> tableCurrent;    ///< A, from the table
	std::optional<double> admissible;      ///< A, Iz with every factor
	std::string tableId;
	std::string tableSource;
	std::vector<CorrectionFactor> factors;
	std::vector<Diagnostic> diagnostics;
	std::vector<Assumption> assumptions;
	std::vector<NormRef> rules;
};

/// Highest conductor temperature in normal service, degC
double maxConductorTemperature(Insulation insulation);

/**
	@brief k = sqrt((theta max - theta a) / (theta max - theta ref))
	@return nothing when the ambient is at or above the highest
	conductor temperature
*/
std::optional<double> temperatureFactor(Insulation insulation,
										double ambient, double reference);

/**
	@brief Iz = I table x every correction factor
	Needs material, insulation, method, section, loaded conductors (or a
	system), a grouping factor and either a temperature factor or an
	ambient temperature.
*/
AmpacityResult admissibleCurrent(const AmpacityInput &input,
								 const AmpacityLibrary &library);

/**
	@brief IB <= IZ
	NotPerformed when either is unknown.
*/
CheckResult checkAmpacity(const std::optional<double> &design_current,
						  const AmpacityResult &ampacity);

struct SectionSuggestion
{
	Status status = Status::NotComputable;
	std::optional<double> section;     ///< mm2, smallest that will do
	std::optional<double> admissible;  ///< A, its Iz
	std::optional<double> dropPercent; ///< its voltage drop, when computed
	bool dropConsidered = false;       ///< the drop limit took part
	bool noneEnough = false;           ///< no row of the table will do
	std::vector<Diagnostic> diagnostics;
	std::vector<NormRef> rules;
};

/**
	@brief The smallest section of the table with IB <= IZ and, when a
	limit is given and the drop can be computed, a drop within it.
	The section of the inputs is ignored: every row is tried.
*/
SectionSuggestion suggestSection(const CircuitInput &circuit,
								 const AmpacityInput &ampacity,
								 const AmpacityLibrary &library,
								 const std::optional<VoltageDropLimit> &limit);

namespace rules {
/// ITC-BT-19 2.2.3: IB <= IZ, from the table loaded
NormRef admissibleCurrent();
/// UNE-HD 60364-5-52 annex B: the temperature factor formula
NormRef temperatureFactor();
}

} // namespace rebt

#endif // REBTAMPACITY_H
