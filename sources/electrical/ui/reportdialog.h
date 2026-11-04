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
#ifndef REPORTDIALOG_H
#define REPORTDIALOG_H

#include "electrical/electricalmodel.h"

#include <QDialog>
#include <QMap>

class QCheckBox;
class QLabel;
class QLineEdit;

/**
	@brief The title block of the calculation report, and whether the
	folios come first. What is typed is kept with the project.
*/
class ReportDialog : public QDialog
{
		Q_OBJECT

	public:
		ReportDialog(const electrical::ReportInfo &info, QWidget *parent = nullptr);
		electrical::ReportInfo info() const;
		bool withFolios() const;
			/// Take the logo from an image file. @return why not, empty when done
		QString setLogoFile(const QString &path);
		void clearLogo();
		QLabel *logoPreview() const { return m_logo_preview; }
		QLineEdit *field(const QString &name) const { return m_fields.value(name); }
		QCheckBox *foliosBox() const { return m_folios; }

			/**
				@brief The data to start from: those kept with the project,
				else the title block of its new folios, the date of today
			*/
		static electrical::ReportInfo proposal(const electrical::ReportInfo &kept,
											   const QString &author,
											   const QString &revision);

	private:
		QMap<QString, QLineEdit *> m_fields;
		QCheckBox *m_folios = nullptr;
		QByteArray m_logo;
		QLabel *m_logo_preview = nullptr;
		void showLogo();
};

#endif // REPORTDIALOG_H
