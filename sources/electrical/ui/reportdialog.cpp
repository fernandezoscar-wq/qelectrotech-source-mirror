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
#include "reportdialog.h"

#include "electrical/report/electricalreport.h"
#include "electrical/report/reportlogo.h"

#include <QCheckBox>
#include <QDate>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

using namespace electrical;

ReportDialog::ReportDialog(const ReportInfo &info, QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(tr("Calculation report"));
	setObjectName(QStringLiteral("report_dialog"));

	auto *form = new QFormLayout;
	const QList<QPair<QString, QPair<QString, QString>>> fields{
		{QStringLiteral("company"), {tr("Company"), info.company}},
		{QStringLiteral("documentTitle"), {tr("Document"), info.documentTitle}},
		{QStringLiteral("planNumber"), {tr("Plan number"), info.planNumber}},
		{QStringLiteral("revision"), {tr("Revision"), info.revision}},
		{QStringLiteral("date"), {tr("Date"), info.date}},
		{QStringLiteral("author"), {tr("Author"), info.author}}};
	for (const auto &f : fields) {
		auto *edit = new QLineEdit(f.second.second);
		edit->setObjectName(f.first);
		m_fields.insert(f.first, edit);
		form->addRow(f.second.first, edit);
	}
	m_fields.value(QStringLiteral("documentTitle"))->setPlaceholderText(
		ElectricalReportBuilder::defaultDocumentTitle());

		//The logo: kept in the project, shown on the cover and in the title block
	m_logo = info.logo;
	m_logo_preview = new QLabel;
	m_logo_preview->setObjectName(QStringLiteral("logo_preview"));
	m_logo_preview->setMinimumSize(160, 50);
	m_logo_preview->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	auto *choose = new QPushButton(tr("Choose..."));
	auto *remove = new QPushButton(tr("Remove"));
	connect(choose, &QPushButton::clicked, this, [this] {
		const QString path = QFileDialog::getOpenFileName(
			this, tr("Company logo"), QString(),
			tr("Images (*.png *.jpg *.jpeg *.svg *.bmp)"));
		if (path.isEmpty()) return;
		const QString error = setLogoFile(path);
		if (!error.isEmpty())
			QMessageBox::warning(this, tr("Company logo"), error);
	});
	connect(remove, &QPushButton::clicked, this, &ReportDialog::clearLogo);
	auto *logo_row = new QHBoxLayout;
	logo_row->addWidget(m_logo_preview, 1);
	logo_row->addWidget(choose);
	logo_row->addWidget(remove);
	form->addRow(tr("Logo"), logo_row);
	showLogo();

	m_folios = new QCheckBox(tr("The folios of the project first, then the annex"));
	m_folios->setObjectName(QStringLiteral("with_folios"));
	m_folios->setChecked(true);

	auto *hint = new QLabel(tr("These data are kept with the project and printed "
							   "in the title block of every page of the annex. "
							   "An empty field stays empty."));
	hint->setWordWrap(true);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto *layout = new QVBoxLayout(this);
	layout->addLayout(form);
	layout->addWidget(m_folios);
	layout->addWidget(hint);
	layout->addWidget(buttons);
}

ReportInfo ReportDialog::info() const
{
	ReportInfo info;
	info.company = m_fields.value(QStringLiteral("company"))->text().trimmed();
	info.documentTitle = m_fields.value(QStringLiteral("documentTitle"))->text().trimmed();
	info.planNumber = m_fields.value(QStringLiteral("planNumber"))->text().trimmed();
	info.revision = m_fields.value(QStringLiteral("revision"))->text().trimmed();
	info.date = m_fields.value(QStringLiteral("date"))->text().trimmed();
	info.author = m_fields.value(QStringLiteral("author"))->text().trimmed();
	info.logo = m_logo;
	return info;
}

bool ReportDialog::withFolios() const
{
	return m_folios->isChecked();
}

ReportInfo ReportDialog::proposal(const ReportInfo &kept, const QString &author,
								  const QString &revision)
{
	if (!kept.isEmpty())
		return kept;
	ReportInfo info;
	info.author = author;
	info.revision = revision;
	info.date = QLocale().toString(QDate::currentDate(), QLocale::ShortFormat);
	return info;
}

QString ReportDialog::setLogoFile(const QString &path)
{
	QString error;
	const QByteArray png = ReportLogo::fromFile(path, &error);
	if (png.isEmpty())
		return error;
	m_logo = png;
	showLogo();
	return QString();
}

void ReportDialog::clearLogo()
{
	m_logo.clear();
	showLogo();
}

void ReportDialog::showLogo()
{
	const QImage image = ReportLogo::image(m_logo);
	if (image.isNull()) {
		m_logo_preview->setPixmap(QPixmap());
		m_logo_preview->setText(tr("No logo"));
		return;
	}
	m_logo_preview->setText(QString());
	m_logo_preview->setPixmap(QPixmap::fromImage(
		image.scaled(160, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
}
