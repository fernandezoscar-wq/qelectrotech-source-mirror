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
#include "ampacityfile.h"

#include <QCoreApplication>
#include <algorithm>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

#include <cmath>

namespace electrical {

namespace {

QString tr(const char *text)
{
	return QCoreApplication::translate("AmpacityFile", text);
}

bool positiveNumber(const QJsonValue &value, double *out)
{
	if (!value.isDouble()) return false;
	const double v = value.toDouble();
	if (!std::isfinite(v) || v <= 0.0) return false;
	*out = v;
	return true;
}

/// One table, or the reason it is left out
std::optional<rebt::AmpacityTable> readTable(const QJsonObject &o, const QString &where,
											 QStringList &errors)
{
	rebt::AmpacityTable t;
	const int before = errors.size();
	auto error = [&](const QString &text) { errors << where + QLatin1String(": ") + text; };

	t.id = o.value(QLatin1String("id")).toString().trimmed().toStdString();
	if (t.id.empty()) error(tr("no \"id\""));
	t.source = o.value(QLatin1String("source")).toString().trimmed().toStdString();
	if (t.source.empty())
		error(tr("no \"source\": say where the values come from"));

	const QString material = o.value(QLatin1String("material")).toString();
	if (material == QLatin1String("Cu")) t.material = rebt::Material::Copper;
	else if (material == QLatin1String("Al")) t.material = rebt::Material::Aluminium;
	else error(tr("\"material\" must be \"Cu\" or \"Al\""));

	const QString insulation = o.value(QLatin1String("insulation")).toString();
	if (insulation == QLatin1String("PVC")) t.insulation = rebt::Insulation::PVC;
	else if (insulation == QLatin1String("XLPE")) t.insulation = rebt::Insulation::XLPE;
	else if (insulation == QLatin1String("EPR")) t.insulation = rebt::Insulation::EPR;
	else error(tr("\"insulation\" must be \"PVC\", \"XLPE\" or \"EPR\""));

	const QJsonValue loaded = o.value(QLatin1String("loadedConductors"));
	if (loaded.isDouble() && (loaded.toDouble() == 2.0 || loaded.toDouble() == 3.0))
		t.loadedConductors = loaded.toInt();
	else error(tr("\"loadedConductors\" must be 2 or 3"));

	t.method = o.value(QLatin1String("method")).toString().trimmed().toStdString();
	if (t.method.empty()) error(tr("no \"method\""));

	const QJsonValue ambient = o.value(QLatin1String("referenceAmbient"));
	if (ambient.isDouble() && std::isfinite(ambient.toDouble()))
		t.referenceAmbient = ambient.toDouble();
	else error(tr("\"referenceAmbient\" must be a number (degC)"));

	const QJsonArray entries = o.value(QLatin1String("entries")).toArray();
	if (entries.isEmpty()) error(tr("no \"entries\""));
	for (int i = 0; i < entries.size(); ++i) {
		const QJsonArray pair = entries.at(i).toArray();
		rebt::AmpacityEntry entry{0, 0};
		if (pair.size() != 2 || !positiveNumber(pair.at(0), &entry.section)
			|| !positiveNumber(pair.at(1), &entry.current)) {
			error(tr("entry %1 must be [section, current], both greater than zero")
				  .arg(i + 1));
			continue;
		}
		t.entries.push_back(entry);
	}
	std::sort(t.entries.begin(), t.entries.end(),
			  [](const rebt::AmpacityEntry &a, const rebt::AmpacityEntry &b) {
		return a.section < b.section; });
	for (size_t i = 1; i < t.entries.size(); ++i) {
		if (t.entries[i].section == t.entries[i - 1].section)
			error(tr("section %1 appears twice").arg(t.entries[i].section));
		else if (t.entries[i].current <= t.entries[i - 1].current)
			error(tr("the current of %1 mm² is not above that of %2 mm²: a typo?")
				  .arg(t.entries[i].section).arg(t.entries[i - 1].section));
	}
	if (errors.size() != before)
		return std::nullopt;
	return t;
}

} // namespace

AmpacityFile readAmpacityJson(const QByteArray &json)
{
	AmpacityFile file;
	QJsonParseError parse_error;
	const QJsonDocument document = QJsonDocument::fromJson(json, &parse_error);
	if (parse_error.error != QJsonParseError::NoError || !document.isObject()) {
		file.errors << tr("Not a JSON file: %1").arg(parse_error.errorString());
		return file;
	}
	const QJsonObject root = document.object();
	if (root.value(QLatin1String("format")).toString() != QLatin1String("qet-ampacity")) {
		file.errors << tr("Not an ampacity file: \"format\" must be \"qet-ampacity\"");
		return file;
	}
	if (root.value(QLatin1String("version")).toInt() != 1) {
		file.errors << tr("Version %1 of the file format is not known")
					   .arg(root.value(QLatin1String("version")).toVariant().toString());
		return file;
	}

	const QJsonArray tables = root.value(QLatin1String("tables")).toArray();
	if (tables.isEmpty())
		file.errors << tr("The file holds no table");
	for (int i = 0; i < tables.size(); ++i) {
		const QJsonObject o = tables.at(i).toObject();
		QString where = tr("Table %1").arg(i + 1);
		const QString id = o.value(QLatin1String("id")).toString();
		if (!id.isEmpty())
			where += QLatin1String(" (") + id + QLatin1Char(')');
		const auto table = readTable(o, where, file.errors);
		if (table && !file.library.add(*table))
			file.errors << where + QLatin1String(": ")
						   + tr("another table has the same material, insulation, "
								"loaded conductors and method");
	}
	return file;
}

AmpacityFile loadAmpacityFile(const QString &path)
{
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly)) {
		AmpacityFile file;
		file.path = path;
		file.errors << tr("Cannot read %1: %2").arg(path, f.errorString());
		return file;
	}
	AmpacityFile file = readAmpacityJson(f.readAll());
	file.path = path;
	return file;
}

AmpacityFile AmpacityTables::s_file;
QDateTime AmpacityTables::s_modified;
qint64 AmpacityTables::s_size = -1;

QString AmpacityTables::settingKey()
{
	return QStringLiteral("electrical/ampacity_tables_file");
}

QString AmpacityTables::filePath()
{
	return QSettings().value(settingKey()).toString();
}

void AmpacityTables::setFilePath(const QString &path)
{
	QSettings().setValue(settingKey(), path);
}

const AmpacityFile &AmpacityTables::current()
{
	const QString path = filePath();
	const QFileInfo info(path);
	const QDateTime modified = info.exists() ? info.lastModified() : QDateTime();
	const qint64 size = info.exists() ? info.size() : -1;
	if (path != s_file.path || modified != s_modified || size != s_size) {
		s_file = path.isEmpty() ? AmpacityFile() : loadAmpacityFile(path);
		s_file.path = path;
		s_modified = modified;
		s_size = size;
	}
	return s_file;
}

} // namespace electrical
