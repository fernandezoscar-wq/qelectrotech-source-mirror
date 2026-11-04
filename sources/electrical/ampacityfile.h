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
#ifndef AMPACITYFILE_H
#define AMPACITYFILE_H

#include "rebt/rebtampacity.h"

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QStringList>

/**
	@brief Ampacity tables from a file the user provides. The program
	ships none: the values of UNE-HD 60364-5-52 are copyrighted, and a
	table typed from a cable catalogue is the user's to choose.

	Format (JSON, see docs/rebt/tablas_h4.md):
	{ "format": "qet-ampacity", "version": 1,
	  "tables": [ { "id": "...", "source": "...", "material": "Cu",
	                "insulation": "PVC", "loadedConductors": 2,
	                "method": "B1", "referenceAmbient": 30,
	                "entries": [[1.5, 15.5], [2.5, 21]] } ] }

	A table with a mistake is left out whole and the mistake reported:
	half a table is never used.
*/
namespace electrical {

struct AmpacityFile
{
	rebt::AmpacityLibrary library;
	QStringList errors;     ///< what was wrong, in the user's language
	QString path;
};

AmpacityFile readAmpacityJson(const QByteArray &json);
AmpacityFile loadAmpacityFile(const QString &path);

/**
	@brief The tables of the file named in the settings, read again when
	the file or the setting changes
*/
class AmpacityTables
{
	public:
		static QString settingKey();
		static QString filePath();
		static void setFilePath(const QString &path);
			/// The tables, empty when no file is set or it cannot be read
		static const AmpacityFile &current();

	private:
		static AmpacityFile s_file;
		static QDateTime s_modified;
		static qint64 s_size;
};

} // namespace electrical

#endif // AMPACITYFILE_H
