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
#include "devicecatalogue.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QSettings>

#include <cmath>

namespace electrical {

namespace {

QString tr(const char *text)
{
	return QCoreApplication::translate("DeviceCatalogue", text);
}

struct Type { const char *word; ProtectionType type; };
const Type TYPES[] = {{"circuit-breaker", ProtectionType::CircuitBreaker},
					  {"residual-current", ProtectionType::ResidualCurrent},
					  {"rcbo", ProtectionType::Rcbo},
					  {"fuse", ProtectionType::Fuse},
					  {"switch", ProtectionType::Switch}};
struct Standard { const char *word; rebt::ProtectionStandard standard; };
const Standard STANDARDS[] = {{"EN60898", rebt::ProtectionStandard::EN60898},
							  {"EN61009", rebt::ProtectionStandard::EN61009},
							  {"EN60947-2", rebt::ProtectionStandard::EN60947_2},
							  {"gG", rebt::ProtectionStandard::FuseGG}};

QString key(const QString &manufacturer, const QString &part)
{
	return manufacturer.trimmed().toCaseFolded() + QLatin1Char('\n')
		   + part.trimmed().toCaseFolded();
}

/// One device, or the reason it is left out
std::optional<DeviceEntry> readDevice(const QJsonObject &o, const QString &where,
									  QStringList &errors)
{
	DeviceEntry d;
	const int before = errors.size();
	auto error = [&](const QString &text) { errors << where + QLatin1String(": ") + text; };
		//A number given must be one, and greater than zero
	auto number = [&](const char *name, std::optional<double> &out) {
		const QJsonValue v = o.value(QLatin1String(name));
		if (v.isUndefined() || v.isNull())
			return;
		if (!v.isDouble() || !std::isfinite(v.toDouble()) || v.toDouble() <= 0.0)
			error(tr("\"%1\" must be a number greater than zero").arg(QLatin1String(name)));
		else
			out = v.toDouble();
	};

	d.manufacturer = o.value(QLatin1String("manufacturer")).toString().trimmed();
	if (d.manufacturer.isEmpty()) error(tr("no \"manufacturer\""));
	d.partNumber = o.value(QLatin1String("partNumber")).toString().trimmed();
	if (d.partNumber.isEmpty()) error(tr("no \"partNumber\""));
	d.source = o.value(QLatin1String("source")).toString().trimmed();
	if (d.source.isEmpty())
		error(tr("no \"source\": say where the values come from"));

	const QString type = o.value(QLatin1String("type")).toString();
	bool type_ok = false;
	for (const Type &t : TYPES)
		if (type == QLatin1String(t.word)) { d.type = t.type; type_ok = true; }
	if (!type_ok)
		error(tr("\"type\" must be \"circuit-breaker\", \"residual-current\", \"rcbo\", "
				 "\"fuse\" or \"switch\""));

	const QJsonValue standard = o.value(QLatin1String("standard"));
	if (!standard.isUndefined() && !standard.isNull()) {
		bool ok = false;
		for (const Standard &s : STANDARDS)
			if (standard.toString() == QLatin1String(s.word)) { d.standard = s.standard; ok = true; }
		if (!ok)
			error(tr("\"standard\" must be \"EN60898\", \"EN61009\", \"EN60947-2\" or \"gG\""));
	}

	std::optional<double> rated;
	number("ratedCurrent", rated);
	if (rated) d.ratedCurrent = *rated;
	else if (o.value(QLatin1String("ratedCurrent")).isUndefined())
		error(tr("no \"ratedCurrent\""));

	d.curve = o.value(QLatin1String("curve")).toString().trimmed();
	number("breakingCapacity", d.breakingCapacity);
	number("sensitivity", d.sensitivity);
	number("conventionalCurrent", d.conventionalCurrent);
	number("tripCurrent", d.tripCurrent);
	{
		std::optional<double> poles;
		number("poles", poles);
		if (poles) {
			if (std::floor(*poles) != *poles || *poles > 4)
				error(tr("\"poles\" must be 1, 2, 3 or 4"));
			else
				d.poles = static_cast<int>(*poles);
		}
	}
		//I2t only with the prospective current it was given for
	const QJsonValue lt = o.value(QLatin1String("letThrough"));
	if (!lt.isUndefined() && !lt.isNull()) {
		const QJsonObject l = lt.toObject();
		const QJsonValue energy = l.value(QLatin1String("energy"));
		const QJsonValue at = l.value(QLatin1String("atCurrent"));
		if (!lt.isObject() || !energy.isDouble() || !at.isDouble()
			|| !std::isfinite(energy.toDouble()) || energy.toDouble() <= 0.0
			|| !std::isfinite(at.toDouble()) || at.toDouble() <= 0.0)
			error(tr("\"letThrough\" must be {\"energy\": A²s, \"atCurrent\": kA}, both "
					 "greater than zero"));
		else {
			d.letThrough = energy.toDouble();
			d.letThroughAt = at.toDouble();
		}
	}
	if (errors.size() != before)
		return std::nullopt;
	return d;
}

} // namespace

QString DeviceEntry::label() const
{
	return manufacturer + QLatin1Char(' ') + partNumber;
}

std::optional<DeviceEntry> DeviceCatalogue::find(const QString &manufacturer,
												 const QString &partNumber) const
{
	const QString k = key(manufacturer, partNumber);
	for (const DeviceEntry &d : devices)
		if (key(d.manufacturer, d.partNumber) == k)
			return d;
	return std::nullopt;
}

DeviceCatalogue readDeviceCatalogueJson(const QByteArray &json)
{
	DeviceCatalogue file;
	QJsonParseError parse_error;
	const QJsonDocument document = QJsonDocument::fromJson(json, &parse_error);
	if (parse_error.error != QJsonParseError::NoError || !document.isObject()) {
		file.errors << tr("Not a JSON file: %1").arg(parse_error.errorString());
		return file;
	}
	const QJsonObject root = document.object();
	if (root.value(QLatin1String("format")).toString() != QLatin1String("qet-devices")) {
		file.errors << tr("Not a device catalogue: \"format\" must be \"qet-devices\"");
		return file;
	}
	if (root.value(QLatin1String("version")).toInt() != 1) {
		file.errors << tr("Version %1 of the file format is not known")
					   .arg(root.value(QLatin1String("version")).toVariant().toString());
		return file;
	}
	const QJsonArray devices = root.value(QLatin1String("devices")).toArray();
	if (devices.isEmpty())
		file.errors << tr("The file holds no device");
	QSet<QString> seen;
	for (int i = 0; i < devices.size(); ++i) {
		const QJsonObject o = devices.at(i).toObject();
		QString where = tr("Device %1").arg(i + 1);
		const QString part = o.value(QLatin1String("partNumber")).toString();
		if (!part.isEmpty())
			where += QLatin1String(" (") + part + QLatin1Char(')');
		const auto device = readDevice(o, where, file.errors);
		if (!device)
			continue;
		const QString k = key(device->manufacturer, device->partNumber);
		if (seen.contains(k)) {
			file.errors << where + QLatin1String(": ")
						   + tr("another device has the same manufacturer and part number");
			continue;
		}
		seen.insert(k);
		file.devices << *device;
	}
	return file;
}

DeviceCatalogue loadDeviceCatalogue(const QString &path)
{
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly)) {
		DeviceCatalogue file;
		file.path = path;
		file.errors << tr("Cannot read %1: %2").arg(path, f.errorString());
		return file;
	}
	DeviceCatalogue file = readDeviceCatalogueJson(f.readAll());
	file.path = path;
	return file;
}

void applyDeviceEntry(ProtectionData &p, const DeviceEntry &d)
{
	const QString reason = QCoreApplication::translate("DeviceCatalogue", "Catalogue: %1")
							   .arg(d.source);
	p.manufacturer = d.manufacturer;
	p.partNumber = d.partNumber;
	p.type = d.type;
	p.standard = d.standard;
	p.ratedCurrent = d.ratedCurrent;
	p.curve = d.curve;
	p.breakingCapacity = d.breakingCapacity;
	p.sensitivity = d.sensitivity;
	p.poles = d.poles;
	p.conventionalCurrent = d.conventionalCurrent;
	p.conventionalCurrentReason = d.conventionalCurrent ? reason : QString();
	p.tripCurrent = d.tripCurrent;
	p.tripCurrentReason = d.tripCurrent ? reason : QString();
	p.letThrough = d.letThrough;
	p.letThroughAt = d.letThroughAt;
	p.letThroughReason = d.letThrough ? reason : QString();
}

DeviceCatalogue DeviceCatalogues::s_file;
QDateTime DeviceCatalogues::s_modified;
qint64 DeviceCatalogues::s_size = -1;

QString DeviceCatalogues::settingKey()
{
	return QStringLiteral("electrical/device_catalogue_file");
}

QString DeviceCatalogues::filePath()
{
	return QSettings().value(settingKey()).toString();
}

void DeviceCatalogues::setFilePath(const QString &path)
{
	QSettings().setValue(settingKey(), path);
}

const DeviceCatalogue &DeviceCatalogues::current()
{
	const QString path = filePath();
	const QFileInfo info(path);
	const QDateTime modified = info.exists() ? info.lastModified() : QDateTime();
	const qint64 size = info.exists() ? info.size() : -1;
	if (path != s_file.path || modified != s_modified || size != s_size) {
		s_file = path.isEmpty() ? DeviceCatalogue() : loadDeviceCatalogue(path);
		s_file.path = path;
		s_modified = modified;
		s_size = size;
	}
	return s_file;
}

} // namespace electrical
