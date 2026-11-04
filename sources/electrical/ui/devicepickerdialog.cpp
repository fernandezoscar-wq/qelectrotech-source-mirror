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
#include "devicepickerdialog.h"

#include "electricaltexts.h"

#include <QDialogButtonBox>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

using namespace electrical;

namespace {

enum Column { ManufacturerColumn, PartColumn, TypeColumn, RatedColumn, CurveColumn,
			  BreakingColumn, SensitivityColumn, PolesColumn, SourceColumn };

QString number(const std::optional<double> &value)
{
	return value ? QLocale().toString(*value, 'g', 6) : QString();
}

} // namespace

DevicePickerDialog::DevicePickerDialog(const DeviceCatalogue &catalogue, QWidget *parent) :
	QDialog(parent),
	m_catalogue(catalogue)
{
	setWindowTitle(tr("Device catalogue"));
	auto *layout = new QVBoxLayout(this);
	auto *origin = new QLabel(
		m_catalogue.path.isEmpty()
			? tr("%n device(s).", "", int(m_catalogue.devices.size()))
			: tr("%n device(s) of %1.", "", int(m_catalogue.devices.size()))
				  .arg(QFileInfo(m_catalogue.path).fileName()));
	origin->setWordWrap(true);
	layout->addWidget(origin);
	auto *hint = new QLabel(tr("The values are those of the file, typed by the user "
							   "from the manufacturer's catalogue: check them "
							   "against it. Choosing a device replaces every datum "
							   "of the device of the circuit."));
	hint->setWordWrap(true);
	layout->addWidget(hint);

	m_search = new QLineEdit;
	m_search->setObjectName(QStringLiteral("search"));
	m_search->setPlaceholderText(tr("Search by manufacturer, part number or source"));
	m_search->setClearButtonEnabled(true);
	connect(m_search, &QLineEdit::textChanged, this, &DevicePickerDialog::filter);
	layout->addWidget(m_search);

	m_list = new QTreeWidget;
	m_list->setObjectName(QStringLiteral("devices"));
	m_list->setRootIsDecorated(false);
	m_list->setUniformRowHeights(true);
	m_list->setSortingEnabled(true);
	m_list->setHeaderLabels({tr("Manufacturer"), tr("Part number"), tr("Type"),
							 tr("In (A)"), tr("Curve"), tr("Icu/Icn (kA)"),
							 tr("IΔn (mA)"), tr("Poles"), tr("Source")});
	for (int i = 0; i < m_catalogue.devices.size(); ++i) {
		const DeviceEntry &d = m_catalogue.devices.at(i);
		auto *item = new QTreeWidgetItem(m_list);
		item->setData(0, Qt::UserRole, i);
		item->setText(ManufacturerColumn, d.manufacturer);
		item->setText(PartColumn, d.partNumber);
		item->setText(TypeColumn, ElectricalTexts::protection(d.type));
		item->setText(RatedColumn, number(d.ratedCurrent));
		item->setText(CurveColumn, d.curve);
		item->setText(BreakingColumn, number(d.breakingCapacity));
		item->setText(SensitivityColumn, number(d.sensitivity));
		item->setText(PolesColumn, d.poles ? QString::number(*d.poles) : QString());
		item->setText(SourceColumn, d.source);
	}
	m_list->sortByColumn(ManufacturerColumn, Qt::AscendingOrder);
	m_list->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	m_list->header()->setStretchLastSection(true);
	connect(m_list, &QTreeWidget::itemSelectionChanged, this, &DevicePickerDialog::updateState);
	connect(m_list, &QTreeWidget::itemDoubleClicked, this, [this] {
		if (entry()) accept();
	});
	layout->addWidget(m_list, 1);

	m_details = new QLabel;
	m_details->setObjectName(QStringLiteral("details"));
	m_details->setWordWrap(true);
	m_details->setTextInteractionFlags(Qt::TextSelectableByMouse);
	layout->addWidget(m_details);

	if (!m_catalogue.errors.isEmpty()) {
		auto *errors = new QLabel(tr("Left out of the file:") + QLatin1Char('\n')
								  + m_catalogue.errors.join(QLatin1Char('\n')));
		errors->setObjectName(QStringLiteral("errors"));
		errors->setWordWrap(true);
		layout->addWidget(errors);
	}

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(m_buttons);
	resize(820, 480);
	updateState();
}

std::optional<DeviceEntry> DevicePickerDialog::entry() const
{
	const QList<QTreeWidgetItem *> items = m_list->selectedItems();
	if (items.isEmpty() || items.first()->isHidden())
		return std::nullopt;
	const int index = items.first()->data(0, Qt::UserRole).toInt();
	if (index < 0 || index >= m_catalogue.devices.size())
		return std::nullopt;
	return m_catalogue.devices.at(index);
}

void DevicePickerDialog::select(const QString &manufacturer, const QString &partNumber)
{
	for (int i = 0; i < m_list->topLevelItemCount(); ++i) {
		QTreeWidgetItem *item = m_list->topLevelItem(i);
		const DeviceEntry &d = m_catalogue.devices.at(item->data(0, Qt::UserRole).toInt());
		if (d.manufacturer.compare(manufacturer.trimmed(), Qt::CaseInsensitive) == 0
			&& d.partNumber.compare(partNumber.trimmed(), Qt::CaseInsensitive) == 0) {
			m_list->setCurrentItem(item);
			m_list->scrollToItem(item);
			return;
		}
	}
}

void DevicePickerDialog::filter(const QString &text)
{
	const QStringList words = text.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
	for (int i = 0; i < m_list->topLevelItemCount(); ++i) {
		QTreeWidgetItem *item = m_list->topLevelItem(i);
		const DeviceEntry &d = m_catalogue.devices.at(item->data(0, Qt::UserRole).toInt());
		const QString haystack = d.manufacturer + QLatin1Char(' ') + d.partNumber
								 + QLatin1Char(' ') + d.source;
		bool shown = true;
		for (const QString &w : words)
			shown = shown && haystack.contains(w, Qt::CaseInsensitive);
		item->setHidden(!shown);
		if (!shown && item->isSelected())
			item->setSelected(false);
	}
	updateState();
}

void DevicePickerDialog::updateState()
{
	const auto d = entry();
	m_buttons->button(QDialogButtonBox::Ok)->setEnabled(d.has_value());
	if (!d) {
		m_details->setText(m_catalogue.devices.isEmpty()
							   ? tr("The catalogue has no device.")
							   : tr("Choose a device."));
		return;
	}
	QStringList lines;
	lines << d->label();
	if (d->standard)
		lines << tr("Standard: %1").arg(ElectricalTexts::standard(*d->standard));
	if (d->conventionalCurrent)
		lines << tr("I2: %1 A").arg(number(d->conventionalCurrent));
	if (d->tripCurrent)
		lines << tr("Instantaneous trip: %1 A").arg(number(d->tripCurrent));
	if (d->letThrough)
		lines << (d->letThroughAt
					  ? tr("I²t: %1 A²s at %2 kA").arg(number(d->letThrough),
													   number(d->letThroughAt))
					  : tr("I²t: %1 A²s").arg(number(d->letThrough)));
	lines << tr("Source: %1").arg(d->source);
	m_details->setText(lines.join(QLatin1Char('\n')));
}
