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
#ifndef DEVICECATALOGUE_H
#define DEVICECATALOGUE_H

#include "electricalmodel.h"

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

#include <optional>

/**
	@brief Protective devices from a file the user provides, as the parts
	management of EPLAN: choosing a part fills the data of the device. The
	program ships none: the data of a manufacturer are the user's to
	type from its catalogue, with the source of each one.

	Format (JSON, see docs/rebt/catalogo_aparatos.md):
	{ "format": "qet-devices", "version": 1,
	  "devices": [ { "manufacturer": "...", "partNumber": "...",
	                 "source": "catalogue, page...",
	                 "type": "circuit-breaker", "standard": "EN60898",
	                 "ratedCurrent": 16, "curve": "C", "breakingCapacity": 6,
	                 "poles": 2, "letThrough": {"energy": 20000, "atCurrent": 6} } ] }

	A device with a mistake is left out whole and the mistake reported.
*/
namespace electrical {

struct DeviceEntry
{
	QString manufacturer;
	QString partNumber;
	QString source;                        ///< where the values come from
	ProtectionType type = ProtectionType::CircuitBreaker;
	std::optional<rebt::ProtectionStandard> standard;
	double ratedCurrent = 0.0;             ///< A
	QString curve;
	std::optional<double> breakingCapacity;    ///< kA
	std::optional<double> sensitivity;     ///< mA
	std::optional<int> poles;
	std::optional<double> conventionalCurrent; ///< I2, A
	std::optional<double> tripCurrent;     ///< A, instantaneous trip
	std::optional<double> letThrough;      ///< A2s
	std::optional<double> letThroughAt;    ///< kA, the prospective current of it

		/// "Manufacturer REF", what a list shows
	QString label() const;
};

struct DeviceCatalogue
{
	QList<DeviceEntry> devices;
	QStringList errors;     ///< what was wrong, in the user's language
	QString path;

		/// The device of @p manufacturer and @p partNumber, none if absent
	std::optional<DeviceEntry> find(const QString &manufacturer,
									const QString &partNumber) const;
};

DeviceCatalogue readDeviceCatalogueJson(const QByteArray &json);
DeviceCatalogue loadDeviceCatalogue(const QString &path);

/**
	@brief The data of @p entry in @p protection: every datum of the
	device it was is replaced, so nothing of another device is left
	(an I2t of the old breaker on the new one). The symbol linked and
	the residual current device named stay.
*/
void applyDeviceEntry(ProtectionData &protection, const DeviceEntry &entry);

/**
	@brief The catalogue of the file named in the settings, read again
	when the file or the setting changes
*/
class DeviceCatalogues
{
	public:
		static QString settingKey();
		static QString filePath();
		static void setFilePath(const QString &path);
		static const DeviceCatalogue &current();

	private:
		static DeviceCatalogue s_file;
		static QDateTime s_modified;
		static qint64 s_size;
};

} // namespace electrical

#endif // DEVICECATALOGUE_H
