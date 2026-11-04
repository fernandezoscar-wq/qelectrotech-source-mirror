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
#ifndef REPORTLOGO_H
#define REPORTLOGO_H

#include <QByteArray>
#include <QCoreApplication>
#include <QImage>
#include <QString>

/**
	@brief The company logo of the calculation report: read from an image
	file (PNG, JPEG, SVG...), kept in the project as a PNG no larger than
	it needs to be for print.
*/
class ReportLogo
{
		Q_DECLARE_TR_FUNCTIONS(ReportLogo)

	public:
			/// Longest side, in pixels: 70 mm at about 430 dpi
		static const int maxSide = 1200;

			/// The PNG to keep, empty and why in @p error when it cannot be read
		static QByteArray fromFile(const QString &path, QString *error = nullptr);
			/// The image of a kept PNG, null if it is not one
		static QImage image(const QByteArray &png);
};

#endif // REPORTLOGO_H
