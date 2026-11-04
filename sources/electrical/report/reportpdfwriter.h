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
#ifndef REPORTPDFWRITER_H
#define REPORTPDFWRITER_H

#include "electricalreport.h"

#include <QCoreApplication>

class QPainter;
class QPdfWriter;

/**
	@brief Paints an ElectricalReport as vector PDF pages, A4 landscape:
	a cover, the characteristics table of each board, the results and
	warnings of each circuit and the rules applied, every page with its
	title block (company, project, document, plan number, revision,
	date, author, page n of m).
*/
class ReportPdfWriter
{
		Q_DECLARE_TR_FUNCTIONS(ReportPdfWriter)

	public:
			/// A PDF of the report alone. @return false, and why in @p error
		static bool write(const electrical::ElectricalReport &report,
						  const QString &path, QString *error = nullptr);
		/**
			@brief Paint the report after what @p painter has already
			painted on @p writer (the folios of the project): each page
			of the report starts a new page.
			@return the number of pages painted
		*/
		static int append(const electrical::ElectricalReport &report,
						  QPdfWriter &writer, QPainter &painter);
};

#endif // REPORTPDFWRITER_H
