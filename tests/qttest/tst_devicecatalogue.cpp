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
#include "electrical/devicecatalogue.h"

#include <QtTest>
#include <QTemporaryDir>

using namespace electrical;

namespace {

/// Fictitious devices, for the tests only: no manufacturer's values
QByteArray validJson()
{
	return R"({ "format": "qet-devices", "version": 1, "devices": [
		{ "manufacturer": "TEST", "partNumber": "MCB-C16-2P", "source": "test data",
		  "type": "circuit-breaker", "standard": "EN60898", "ratedCurrent": 16,
		  "curve": "C", "breakingCapacity": 6, "poles": 2,
		  "letThrough": {"energy": 20000, "atCurrent": 6} },
		{ "manufacturer": "TEST", "partNumber": "RCD-40-30", "source": "test data p. 2",
		  "type": "residual-current", "ratedCurrent": 40, "sensitivity": 30, "poles": 2 },
		{ "manufacturer": "OTHER", "partNumber": "MCCB-100", "source": "test data p. 3",
		  "type": "circuit-breaker", "standard": "EN60947-2", "ratedCurrent": 100,
		  "breakingCapacity": 25, "poles": 4, "tripCurrent": 1000,
		  "conventionalCurrent": 130 } ] })";
}

QByteArray withDevice(const QByteArray &device)
{
	return "{\"format\":\"qet-devices\",\"version\":1,\"devices\":[" + device
		   + ",{\"manufacturer\":\"TEST\",\"partNumber\":\"GOOD\",\"source\":\"s\","
			 "\"type\":\"fuse\",\"standard\":\"gG\",\"ratedCurrent\":25}]}";
}

} // namespace

class tst_devicecatalogue : public QObject
{
		Q_OBJECT

	private slots:
		void readsAValidFile()
		{
			const DeviceCatalogue c = readDeviceCatalogueJson(validJson());
			QVERIFY2(c.errors.isEmpty(), qPrintable(c.errors.join('\n')));
			QCOMPARE(c.devices.size(), 3);
			const DeviceEntry &d = c.devices.first();
			QCOMPARE(d.label(), QStringLiteral("TEST MCB-C16-2P"));
			QCOMPARE(d.type, ProtectionType::CircuitBreaker);
			QCOMPARE(*d.standard, rebt::ProtectionStandard::EN60898);
			QCOMPARE(d.ratedCurrent, 16.0);
			QCOMPARE(d.curve, QStringLiteral("C"));
			QCOMPARE(*d.breakingCapacity, 6.0);
			QCOMPARE(*d.poles, 2);
			QCOMPARE(*d.letThrough, 20000.0);
			QCOMPARE(*d.letThroughAt, 6.0);
			QVERIFY(!d.tripCurrent);
			QVERIFY(!d.sensitivity);
			const DeviceEntry &r = c.devices.at(1);
			QCOMPARE(r.type, ProtectionType::ResidualCurrent);
			QCOMPARE(*r.sensitivity, 30.0);
			QVERIFY(!r.standard);
			const DeviceEntry &m = c.devices.at(2);
			QCOMPARE(*m.standard, rebt::ProtectionStandard::EN60947_2);
			QCOMPARE(*m.tripCurrent, 1000.0);
			QCOMPARE(*m.conventionalCurrent, 130.0);
		}

		void findIgnoresCaseAndSpaces()
		{
			const DeviceCatalogue c = readDeviceCatalogueJson(validJson());
			QVERIFY(c.find(QStringLiteral(" test "), QStringLiteral("mcb-c16-2p")));
			QCOMPARE(c.find(QStringLiteral("OTHER"), QStringLiteral("MCCB-100"))->ratedCurrent,
					 100.0);
			QVERIFY(!c.find(QStringLiteral("TEST"), QStringLiteral("MCCB-100")));
		}

		void refusesWhatIsWrong_data()
		{
			QTest::addColumn<QByteArray>("json");
			QTest::addColumn<QString>("error");
			QTest::addColumn<int>("devices");

			const QByteArray good = "{\"manufacturer\":\"M\",\"partNumber\":\"P\","
									"\"source\":\"s\",\"type\":\"circuit-breaker\","
									"\"ratedCurrent\":10}";
			auto bad = [&](const char *from, const char *to) {
				return withDevice(QByteArray(good).replace(from, to));
			};
			QTest::newRow("not json") << QByteArray("[oops") << "Not a JSON file" << 0;
			QTest::newRow("format") << QByteArray("{\"format\":\"qet-ampacity\",\"version\":1}")
									<< "qet-devices" << 0;
			QTest::newRow("version") << QByteArray("{\"format\":\"qet-devices\",\"version\":3}")
									 << "Version 3" << 0;
			QTest::newRow("empty") << QByteArray("{\"format\":\"qet-devices\",\"version\":1,\"devices\":[]}")
								   << "no device" << 0;
			QTest::newRow("manufacturer") << bad("\"M\"", "\" \"") << "manufacturer" << 1;
			QTest::newRow("part") << bad("\"partNumber\":\"P\",", "") << "partNumber" << 1;
			QTest::newRow("source") << bad("\"source\":\"s\",", "") << "source" << 1;
			QTest::newRow("type") << bad("circuit-breaker", "breaker") << "\"type\"" << 1;
			QTest::newRow("standard") << bad("\"ratedCurrent\"", "\"standard\":\"IEC\",\"ratedCurrent\"")
									  << "\"standard\"" << 1;
			QTest::newRow("no rated") << bad(",\"ratedCurrent\":10", "") << "ratedCurrent" << 1;
			QTest::newRow("rated zero") << bad(":10}", ":0}") << "greater than zero" << 1;
			QTest::newRow("rated text") << bad(":10}", ":\"10\"}") << "greater than zero" << 1;
			QTest::newRow("negative Icu") << bad(":10}", ":10,\"breakingCapacity\":-6}")
										  << "breakingCapacity" << 1;
			QTest::newRow("poles 5") << bad(":10}", ":10,\"poles\":5}") << "poles" << 1;
			QTest::newRow("poles 1.5") << bad(":10}", ":10,\"poles\":1.5}") << "poles" << 1;
			QTest::newRow("I2t number") << bad(":10}", ":10,\"letThrough\":20000}")
										<< "atCurrent" << 1;
			QTest::newRow("I2t no current") << bad(":10}", ":10,\"letThrough\":{\"energy\":20000}}")
											<< "atCurrent" << 1;
			QTest::newRow("I2t zero") << bad(":10}", ":10,\"letThrough\":{\"energy\":0,\"atCurrent\":6}}")
									  << "atCurrent" << 1;
			QTest::newRow("I2t at zero") << bad(":10}", ":10,\"letThrough\":{\"energy\":20000,\"atCurrent\":0}}")
										 << "atCurrent" << 1;
			QTest::newRow("I2t at text") << bad(":10}", ":10,\"letThrough\":{\"energy\":20000,\"atCurrent\":\"6\"}}")
										 << "atCurrent" << 1;
			QTest::newRow("twice") << withDevice(QByteArray(good).replace("\"M\"", "\"test\"")
												 .replace("\"P\"", "\" good \""))
								   << "same manufacturer and part number" << 1;
		}

		void refusesWhatIsWrong()
		{
			QFETCH(QByteArray, json);
			QFETCH(QString, error);
			QFETCH(int, devices);
			const DeviceCatalogue c = readDeviceCatalogueJson(json);
			QVERIFY2(c.errors.join('\n').contains(error), qPrintable(c.errors.join('\n')));
				// the wrong device is left out whole, the good one kept
			QCOMPARE(int(c.devices.size()), devices);
		}

		void applyReplacesTheWholeDevice()
		{
			const DeviceCatalogue c = readDeviceCatalogueJson(validJson());
			ProtectionData p;
			p.element = QUuid::createUuid();
			p.residualDevice = QStringLiteral("ID1");
				// what the previous device had
			p.manufacturer = QStringLiteral("OLD");
			p.tripCurrent = 500;
			p.tripCurrentReason = QStringLiteral("old catalogue");
			p.conventionalCurrent = 20;
			p.conventionalCurrentReason = QStringLiteral("old");
			p.sensitivity = 300;
			const QUuid element = p.element;
			applyDeviceEntry(p, c.devices.first());
			QCOMPARE(p.element, element);
			QCOMPARE(p.residualDevice, QStringLiteral("ID1"));
			QCOMPARE(p.manufacturer, QStringLiteral("TEST"));
			QCOMPARE(p.partNumber, QStringLiteral("MCB-C16-2P"));
			QCOMPARE(*p.ratedCurrent, 16.0);
			QCOMPARE(*p.type, ProtectionType::CircuitBreaker);
			QVERIFY(!p.tripCurrent);
			QVERIFY(p.tripCurrentReason.isEmpty());
			QVERIFY(!p.conventionalCurrent);
			QVERIFY(p.conventionalCurrentReason.isEmpty());
			QVERIFY(!p.sensitivity);
			QCOMPARE(*p.letThrough, 20000.0);
			QCOMPARE(*p.letThroughAt, 6.0);
			QCOMPARE(p.letThroughReason, QStringLiteral("Catalogue: test data"));

				// the next device has no I2t: none is left of this one
			applyDeviceEntry(p, c.devices.at(2));
			QVERIFY(!p.letThrough);
			QVERIFY(!p.letThroughAt);
			QVERIFY(p.letThroughReason.isEmpty());
			QCOMPARE(*p.tripCurrent, 1000.0);
			QCOMPARE(p.tripCurrentReason, QStringLiteral("Catalogue: test data p. 3"));
			QCOMPARE(*p.conventionalCurrent, 130.0);
			QCOMPARE(*p.poles, 4);
		}

		void catalogueFollowsTheSetting()
		{
			QCoreApplication::setOrganizationName(QStringLiteral("QElectroTechTest"));
			QCoreApplication::setApplicationName(QStringLiteral("tst_devicecatalogue"));
			QTemporaryDir dir;
			QSettings::setDefaultFormat(QSettings::IniFormat);
			QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.filePath("settings"));

			DeviceCatalogues::setFilePath(QString());
			QVERIFY(DeviceCatalogues::current().devices.isEmpty());
			QVERIFY(DeviceCatalogues::current().errors.isEmpty());

			const QString path = dir.filePath("devices.json");
			QFile file(path);
			QVERIFY(file.open(QIODevice::WriteOnly));
			file.write(validJson());
			file.close();
			DeviceCatalogues::setFilePath(path);
			QCOMPARE(DeviceCatalogues::current().devices.size(), 3);
			QCOMPARE(DeviceCatalogues::current().path, path);

				// changed on disk: read again
			QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
			file.write(QByteArray(validJson()).replace("\"source\": \"test data\",", ""));
			file.close();
			QCOMPARE(DeviceCatalogues::current().devices.size(), 2);
			QCOMPARE(DeviceCatalogues::current().errors.size(), 1);

			DeviceCatalogues::setFilePath(dir.filePath("missing.json"));
			QVERIFY(DeviceCatalogues::current().devices.isEmpty());
			QVERIFY(DeviceCatalogues::current().errors.first().contains("Cannot read"));
			DeviceCatalogues::setFilePath(QString());
		}
};

QTEST_GUILESS_MAIN(tst_devicecatalogue)
#include "tst_devicecatalogue.moc"
