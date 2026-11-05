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
#include "pvdialog.h"

#include "electrical/electricalevaluation.h"
#include "electricaltexts.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>

using namespace electrical;

namespace {

enum Column { NameColumn, ModulesColumn, StringsColumn, FuseColumn, LengthColumn,
			  SectionColumn, MaterialColumn, TemperatureColumn, AdmissibleColumn,
			  AdmissibleReasonColumn, ColumnCount };

/// Text columns, the others are numbers
bool isText(int column)
{
	return column == NameColumn || column == MaterialColumn
		   || column == AdmissibleReasonColumn;
}

QString integerText(const std::optional<int> &value)
{
	return value ? QString::number(*value) : QString();
}

QString materialText(const std::optional<rebt::Material> &material)
{
	if (!material) return QString();
	return *material == rebt::Material::Copper ? QStringLiteral("Cu") : QStringLiteral("Al");
}

/// "Cu" or "Al", any case; empty not known; anything else not valid
std::optional<rebt::Material> parseMaterial(const QString &text, bool *ok)
{
	const QString t = text.trimmed().toLower();
	*ok = true;
	if (t.isEmpty()) return std::nullopt;
	if (t == QLatin1String("cu")) return rebt::Material::Copper;
	if (t == QLatin1String("al")) return rebt::Material::Aluminium;
	*ok = false;
	return std::nullopt;
}

/// A whole number of 1 or more; empty not known
std::optional<int> parseCount(const QString &text, bool *ok)
{
	const QString t = text.trimmed();
	*ok = true;
	if (t.isEmpty()) return std::nullopt;
	const int value = t.toInt(ok);
	if (!*ok || value < 1) {
		*ok = false;
		return std::nullopt;
	}
	return value;
}

} // namespace

PvDialog::PvDialog(const PvData &pv, QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(tr("Photovoltaic generator"));
	auto *content = new QWidget;
	auto *columns = new QHBoxLayout(content);

	auto *module_box = new QGroupBox(tr("Module (datasheet, STC)"));
	auto *module = new QFormLayout(module_box);
	addField(module, QStringLiteral("moduleName"), tr("Manufacturer and model"), QString(),
			 pv.moduleName);
	addField(module, QStringLiteral("openCircuitVoltage"), tr("Voc"), tr("V"),
			 formatNumber(pv.openCircuitVoltage));
	addField(module, QStringLiteral("mppVoltage"), tr("Vmpp"), tr("V"),
			 formatNumber(pv.mppVoltage));
	addField(module, QStringLiteral("shortCircuitCurrent"), tr("Isc"), tr("A"),
			 formatNumber(pv.shortCircuitCurrent));
	addField(module, QStringLiteral("mppCurrent"), tr("Impp"), tr("A"),
			 formatNumber(pv.mppCurrent));
	addField(module, QStringLiteral("vocCoefficient"), tr("Voc coefficient"), tr("%/°C"),
			 formatNumber(pv.vocCoefficient))->setToolTip(tr("Negative, e.g. -0,29"));
	addField(module, QStringLiteral("vmppCoefficient"), tr("Vmpp coefficient"), tr("%/°C"),
			 formatNumber(pv.vmppCoefficient))->setToolTip(
		tr("When the datasheet does not give it, that of Pmax is on the safe side: "
		   "say so in the source"));
	addField(module, QStringLiteral("maxSystemVoltage"), tr("Maximum system voltage"), tr("V"),
			 formatNumber(pv.maxSystemVoltage));
	addField(module, QStringLiteral("maxSeriesFuse"), tr("Maximum series fuse"), tr("A"),
			 formatNumber(pv.maxSeriesFuse))->setToolTip(tr("IMOD_MAX_OCPR of the datasheet"));
	addField(module, QStringLiteral("moduleSource"), tr("Source"), QString(), pv.moduleSource)
		->setPlaceholderText(tr("datasheet, revision..."));
	columns->addWidget(module_box);

	auto *right = new QVBoxLayout;
	auto *inverter_box = new QGroupBox(tr("Inverter, DC input (datasheet)"));
	auto *inverter = new QFormLayout(inverter_box);
	addField(inverter, QStringLiteral("inverterName"), tr("Manufacturer and model"), QString(),
			 pv.inverterName);
	addField(inverter, QStringLiteral("maxDcVoltage"), tr("Maximum DC voltage"), tr("V"),
			 formatNumber(pv.maxDcVoltage));
	addField(inverter, QStringLiteral("mpptMinVoltage"), tr("MPPT from"), tr("V"),
			 formatNumber(pv.mpptMinVoltage));
	addField(inverter, QStringLiteral("mpptMaxVoltage"), tr("MPPT to"), tr("V"),
			 formatNumber(pv.mpptMaxVoltage));
	addField(inverter, QStringLiteral("maxInputCurrent"), tr("Maximum input current per MPPT"),
			 tr("A"), formatNumber(pv.maxInputCurrent));
	addField(inverter, QStringLiteral("maxShortCircuitCurrent"),
			 tr("Maximum short circuit current per MPPT"), tr("A"),
			 formatNumber(pv.maxShortCircuitCurrent));
	addField(inverter, QStringLiteral("inverterSource"), tr("Source"), QString(),
			 pv.inverterSource)->setPlaceholderText(tr("datasheet, revision..."));
	right->addWidget(inverter_box);

	auto *site_box = new QGroupBox(tr("Site: cell temperatures"));
	auto *site = new QFormLayout(site_box);
	addField(site, QStringLiteral("minCellTemperature"), tr("Lowest"), tr("°C"),
			 formatNumber(pv.minCellTemperature))->setToolTip(
		tr("For the highest open circuit voltage: the lowest temperature of the site"));
	addField(site, QStringLiteral("maxCellTemperature"), tr("Highest in operation"), tr("°C"),
			 formatNumber(pv.maxCellTemperature))->setToolTip(
		tr("For the lowest MPP voltage: the highest cell temperature in operation"));
	addField(site, QStringLiteral("temperatureReason"), tr("Source"), QString(),
			 pv.temperatureReason)->setPlaceholderText(tr("climate data, criterion..."));
	right->addWidget(site_box);
	right->addStretch();
	columns->addLayout(right);

	auto *strings_box = new QGroupBox(tr("Strings, one row per inverter input"));
	auto *strings_layout = new QVBoxLayout(strings_box);
	m_strings = new QTableWidget(0, ColumnCount);
	m_strings->setObjectName(QStringLiteral("strings"));
	m_strings->setHorizontalHeaderLabels(
		{tr("Input"), tr("Modules per string"), tr("Strings in parallel"),
		 tr("String fuse (A)"), tr("Cable length (m)"), tr("Section (mm²)"),
		 tr("Material (Cu/Al)"), tr("Conductor (°C)"), tr("Iz (A)"), tr("Source of Iz")});
	m_strings->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	m_strings->horizontalHeader()->setStretchLastSection(true);
	for (const PvStringGroup &g : pv.strings)
		addRow(g);
	connect(m_strings, &QTableWidget::itemChanged, this, &PvDialog::updateState);
	strings_layout->addWidget(m_strings);
	auto *row_buttons = new QHBoxLayout;
	auto *add = new QPushButton(tr("Add"));
	add->setObjectName(QStringLiteral("addGroup"));
	connect(add, &QPushButton::clicked, this, &PvDialog::addGroup);
	auto *remove = new QPushButton(tr("Remove"));
	connect(remove, &QPushButton::clicked, this, [this] {
		const int row = m_strings->currentRow();
		if (row >= 0) m_strings->removeRow(row);
		updateState();
	});
	row_buttons->addWidget(add);
	row_buttons->addWidget(remove);
	row_buttons->addStretch();
	strings_layout->addLayout(row_buttons);
	auto *hint = new QLabel(tr("Length: one way, from the farthest module. Iz of the string "
							   "cable with its source (table and factors). Empty: not known."));
	hint->setWordWrap(true);
	strings_layout->addWidget(hint);

	m_result = new QLabel;
	m_result->setObjectName(QStringLiteral("result"));
	m_result->setWordWrap(true);
	m_result->setTextInteractionFlags(Qt::TextSelectableByMouse);

	auto *inner = new QWidget;
	auto *inner_layout = new QVBoxLayout(inner);
	inner_layout->addWidget(content);
	inner_layout->addWidget(strings_box);
	inner_layout->addWidget(m_result);
	inner_layout->addStretch();
	auto *scroll = new QScrollArea;
	scroll->setWidget(inner);
	scroll->setWidgetResizable(true);

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	auto *layout = new QVBoxLayout(this);
	layout->addWidget(scroll);
	layout->addWidget(m_buttons);
	resize(960, 760);
	updateState();
}

QLineEdit *PvDialog::addField(QFormLayout *form, const QString &name, const QString &label,
							  const QString &unit, const QString &text)
{
	auto *edit = new QLineEdit(text);
	edit->setObjectName(name);
	if (!unit.isEmpty())
		edit->setPlaceholderText(tr("unknown"));
	m_fields.insert(name, edit);
	connect(edit, &QLineEdit::textChanged, this, &PvDialog::updateState);
	if (unit.isEmpty()) {
		form->addRow(label, edit);
	} else {
		auto *row = new QHBoxLayout;
		row->addWidget(edit);
		row->addWidget(new QLabel(unit));
		form->addRow(label, row);
	}
	return edit;
}

void PvDialog::addGroup()
{
	PvStringGroup g;
	g.name = tr("MPPT%1").arg(m_strings->rowCount() + 1);
	addRow(g);
	updateState();
}

void PvDialog::addRow(const PvStringGroup &g)
{
	const QSignalBlocker blocker(m_strings);
	const int row = m_strings->rowCount();
	m_strings->insertRow(row);
	const QStringList texts{g.name, integerText(g.modulesPerString),
							integerText(g.parallelStrings), formatNumber(g.stringFuse),
							formatNumber(g.cableLength), formatNumber(g.cableSection),
							materialText(g.cableMaterial), formatNumber(g.conductorTemperature),
							formatNumber(g.cableAdmissible), g.cableAdmissibleReason};
	for (int c = 0; c < ColumnCount; ++c)
		m_strings->setItem(row, c, new QTableWidgetItem(texts.at(c)));
}

bool PvDialog::isValid() const
{
	for (auto it = m_fields.cbegin(); it != m_fields.cend(); ++it) {
		if (it.value()->placeholderText() != tr("unknown"))
			continue;   // a name or a source
		bool ok = false;
		parseNumber(it.value()->text(), &ok);
		if (!ok) return false;
	}
	for (int row = 0; row < m_strings->rowCount(); ++row) {
		for (int c = 0; c < ColumnCount; ++c) {
			const QString text = m_strings->item(row, c) ? m_strings->item(row, c)->text() : QString();
			bool ok = true;
			if (c == ModulesColumn || c == StringsColumn) parseCount(text, &ok);
			else if (c == MaterialColumn) parseMaterial(text, &ok);
			else if (!isText(c)) parseNumber(text, &ok);
			if (!ok) return false;
		}
	}
	return true;
}

PvData PvDialog::pv() const
{
	PvData p;
	auto text = [this](const char *name) {
		return m_fields.value(QLatin1String(name))->text().trimmed();
	};
	auto number = [this](const char *name) {
		bool ok = false;
		const auto v = parseNumber(m_fields.value(QLatin1String(name))->text(), &ok);
		return ok ? v : std::nullopt;
	};
	p.moduleName = text("moduleName");
	p.openCircuitVoltage = number("openCircuitVoltage");
	p.mppVoltage = number("mppVoltage");
	p.shortCircuitCurrent = number("shortCircuitCurrent");
	p.mppCurrent = number("mppCurrent");
	p.vocCoefficient = number("vocCoefficient");
	p.vmppCoefficient = number("vmppCoefficient");
	p.maxSystemVoltage = number("maxSystemVoltage");
	p.maxSeriesFuse = number("maxSeriesFuse");
	p.moduleSource = text("moduleSource");
	p.inverterName = text("inverterName");
	p.maxDcVoltage = number("maxDcVoltage");
	p.mpptMinVoltage = number("mpptMinVoltage");
	p.mpptMaxVoltage = number("mpptMaxVoltage");
	p.maxInputCurrent = number("maxInputCurrent");
	p.maxShortCircuitCurrent = number("maxShortCircuitCurrent");
	p.inverterSource = text("inverterSource");
	p.minCellTemperature = number("minCellTemperature");
	p.maxCellTemperature = number("maxCellTemperature");
	p.temperatureReason = text("temperatureReason");
	for (int row = 0; row < m_strings->rowCount(); ++row) {
		auto cell = [&](int c) {
			return m_strings->item(row, c) ? m_strings->item(row, c)->text() : QString();
		};
		bool ok = false;
		PvStringGroup g;
		g.name = cell(NameColumn).trimmed();
		g.modulesPerString = parseCount(cell(ModulesColumn), &ok);
		g.parallelStrings = parseCount(cell(StringsColumn), &ok);
		g.stringFuse = parseNumber(cell(FuseColumn), &ok);
		g.cableLength = parseNumber(cell(LengthColumn), &ok);
		g.cableSection = parseNumber(cell(SectionColumn), &ok);
		g.cableMaterial = parseMaterial(cell(MaterialColumn), &ok);
		g.conductorTemperature = parseNumber(cell(TemperatureColumn), &ok);
		g.cableAdmissible = parseNumber(cell(AdmissibleColumn), &ok);
		g.cableAdmissibleReason = cell(AdmissibleReasonColumn).trimmed();
		if (!(g == PvStringGroup()))
			p.strings << g;
	}
	return p;
}

void PvDialog::updateState()
{
		//Not a number: in red, and the dialog cannot be accepted
	for (auto it = m_fields.cbegin(); it != m_fields.cend(); ++it) {
		QLineEdit *edit = it.value();
		if (edit->placeholderText() != tr("unknown"))
			continue;
		bool ok = false;
		parseNumber(edit->text(), &ok);
		edit->setStyleSheet(ok ? QString() : QStringLiteral("QLineEdit { background: #ffd6d6; }"));
	}
	{
		const QSignalBlocker blocker(m_strings);
		for (int row = 0; row < m_strings->rowCount(); ++row) {
			for (int c = 0; c < ColumnCount; ++c) {
				QTableWidgetItem *item = m_strings->item(row, c);
				if (!item) continue;
				bool ok = true;
				if (c == ModulesColumn || c == StringsColumn) parseCount(item->text(), &ok);
				else if (c == MaterialColumn) parseMaterial(item->text(), &ok);
				else if (!isText(c)) parseNumber(item->text(), &ok);
				item->setBackground(ok ? QBrush() : QBrush(QColor(0xff, 0xd6, 0xd6)));
			}
		}
	}
	const bool valid = isValid();
	m_buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
	if (!valid) {
		m_result->setText(tr("A field holds text that is not a number."));
		return;
	}
	Circuit c;
	c.pv = pv();
	CircuitEvaluation e;
	if (!c.pv.strings.isEmpty())
		e = evaluate(c, rebt::AmpacityLibrary());
	m_result->setText(c.pv.strings.isEmpty() ? tr("Add the strings of each input.")
											 : ElectricalTexts::pvDetails(c, e));
}
