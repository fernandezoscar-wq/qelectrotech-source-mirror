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
#include "reportlogo.h"

#include <QBuffer>
#include <QImageReader>

QByteArray ReportLogo::fromFile(const QString &path, QString *error)
{
	QImageReader reader(path);
	reader.setAutoTransform(true);
		//A vector logo (SVG) is drawn straight at the size kept
	const QSize size = reader.size();
	if (size.isValid() && qMax(size.width(), size.height()) > maxSide)
		reader.setScaledSize(size.scaled(maxSide, maxSide, Qt::KeepAspectRatio));
	else if (size.isValid() && reader.format() == "svg")
		reader.setScaledSize(size.scaled(maxSide, maxSide, Qt::KeepAspectRatio));
	QImage image = reader.read();
	if (image.isNull()) {
		if (error)
			*error = tr("Cannot read the image %1: %2").arg(path, reader.errorString());
		return QByteArray();
	}
	if (qMax(image.width(), image.height()) > maxSide)
		image = image.scaled(maxSide, maxSide, Qt::KeepAspectRatio, Qt::SmoothTransformation);

	QByteArray png;
	QBuffer buffer(&png);
	buffer.open(QIODevice::WriteOnly);
	image.save(&buffer, "PNG");
	return png;
}

QImage ReportLogo::image(const QByteArray &png)
{
	QImage image;
	image.loadFromData(png);
	return image;
}
