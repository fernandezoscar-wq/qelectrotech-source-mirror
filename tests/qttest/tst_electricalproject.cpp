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
#include "qetapptest.h"
#include "dataBase/qetdbfile.h"
#include "diagram.h"
#include "electrical/electricalmodel.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "qetresult.h"

#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>

using namespace electrical;

/**
	The electrical model of a project, through the real project and every
	file format: it is saved, opened again unchanged, and never disturbs a
	project that has none.
*/
class tst_electricalproject : public QObject
{
		Q_OBJECT

	private slots:
		void initTestCase();
		void projectWithoutModelIsUnchanged();
		void settingMarksModifiedAndWritesOneBlock();
		void clearingRemovesTheBlock();
		void survivesQet();
		void survivesQetz();
		void survivesQetdb();
		void survivesEveryConversion();
		void otherUnknownBlocksAreKept();
		void newerVersionIsNeverOverwritten();
		void badValuesAreReported();

	private:
		QString fixtureCopy(const QString &name, const QString &extra = QString());
		ElectricalModel sampleModel(QETProject &project) const;
		static int blocksIn(const QDomDocument &document, const QString &tag);
		static QDomElement blockIn(const QDomDocument &document, const QString &tag);

		QTemporaryDir m_dir;
		QString m_fixture;
};

void tst_electricalproject::initTestCase()
{
	QVERIFY(m_dir.isValid());
	m_fixture = QFINDTESTDATA("fixtures/every_feature.qet");
	QVERIFY(!m_fixture.isEmpty());
}

/**
	A copy of the fixture, with @p extra written into <project> when given
*/
QString tst_electricalproject::fixtureCopy(const QString &name, const QString &extra)
{
	QFile source(m_fixture);
	if (!source.open(QIODevice::ReadOnly)) return QString();
	QString text = QString::fromUtf8(source.readAll());
	if (!extra.isEmpty()) {
		const int end = text.lastIndexOf(QLatin1String("</project>"));
		if (end < 0) return QString();
		text.insert(end, extra);
	}
	const QString path = m_dir.filePath(name);
	QFile copy(path);
	if (!copy.open(QIODevice::WriteOnly)) return QString();
	copy.write(text.toUtf8());
	return path;
}

/**
	A model whose board and circuit point at real symbols of @p project
	when its first folio has some
*/
ElectricalModel tst_electricalproject::sampleModel(QETProject &project) const
{
	QList<QUuid> symbols;
	for (Diagram *diagram : project.diagrams()) {
		for (Element *element : diagram->elements())
			symbols << element->uuid();
		if (symbols.size() >= 3) break;
	}

	ElectricalModel model;
	Board board;
	board.id = QUuid::createUuid();
	board.name = QStringLiteral("CGMP");
	board.element = symbols.value(0);
	model.setBoard(board);

	Circuit circuit;
	circuit.id = QUuid::createUuid();
	circuit.name = QStringLiteral("C1");
	circuit.board = board.id;
	circuit.origin = symbols.value(1);
	circuit.destination = symbols.value(2);
	circuit.system = rebt::System::SinglePhase;
	circuit.voltage = 230.0;
	circuit.use = CircuitUse::DwellingInterior;
	circuit.load.power = 3680.0;
	circuit.load.powerFactor = 1.0;
	circuit.conductor.material = rebt::Material::Copper;
	circuit.conductor.section = 2.5;
	circuit.conductor.length = 25.0;
	circuit.conductor.temperature = 70.0;
	circuit.protection.ratedCurrent = 16.0;
	circuit.protection.curve = QStringLiteral("C");
	circuit.notes = QStringLiteral("Longitud estimada");
	model.setCircuit(circuit);

	Circuit unknown;   // nothing known yet but its board
	unknown.id = QUuid::createUuid();
	unknown.board = board.id;
	model.setCircuit(unknown);
	return model;
}

int tst_electricalproject::blocksIn(const QDomDocument &document, const QString &tag)
{
	int count = 0;
	for (QDomElement e = document.documentElement().firstChildElement(tag);
		 !e.isNull(); e = e.nextSiblingElement(tag))
		++count;
	return count;
}

QDomElement tst_electricalproject::blockIn(const QDomDocument &document, const QString &tag)
{
	return document.documentElement().firstChildElement(tag);
}

void tst_electricalproject::projectWithoutModelIsUnchanged()
{
	QETProject project(fixtureCopy(QStringLiteral("plain.qet")));
	QCOMPARE(project.state(), QETProject::Ok);
	QVERIFY(project.electricalModel().isEmpty());
	QCOMPARE(project.electricalReadReport().version, 0);
	QCOMPARE(blocksIn(project.toXml(), ElectricalModel::tagName()), 0);
		// setting an empty model on an empty one changes nothing
	QSignalSpy modified(&project, &QETProject::projectModified);
	QVERIFY(project.setElectricalModel(ElectricalModel()));
	QCOMPARE(modified.count(), 0);
}

void tst_electricalproject::settingMarksModifiedAndWritesOneBlock()
{
	QETProject project(fixtureCopy(QStringLiteral("set.qet")));
	QCOMPARE(project.state(), QETProject::Ok);
	QSignalSpy modified(&project, &QETProject::projectModified);

	ElectricalModel model = sampleModel(project);
	QVERIFY(project.setElectricalModel(model));
	QVERIFY(modified.count() >= 1);
	QCOMPARE(modified.last().at(1).toBool(), true);
	QVERIFY(project.electricalModel() == model);

		// a second model replaces the first: still one block
	Circuit c = model.circuits().first();
	c.name = QStringLiteral("C1 renamed");
	model.setCircuit(c);
	QVERIFY(project.setElectricalModel(model));
	const QDomDocument document = project.toXml();
	QCOMPARE(blocksIn(document, ElectricalModel::tagName()), 1);
	QVERIFY(blockIn(document, ElectricalModel::tagName()).toElement()
			.firstChildElement(QStringLiteral("circuit"))
			.attribute(QStringLiteral("name")) == QStringLiteral("C1 renamed"));
}

void tst_electricalproject::clearingRemovesTheBlock()
{
	QETProject project(fixtureCopy(QStringLiteral("clear.qet")));
	QVERIFY(project.setElectricalModel(sampleModel(project)));
	QCOMPARE(blocksIn(project.toXml(), ElectricalModel::tagName()), 1);
	QVERIFY(project.setElectricalModel(ElectricalModel()));
	QCOMPARE(blocksIn(project.toXml(), ElectricalModel::tagName()), 0);
}

void tst_electricalproject::survivesQet()
{
	const QString path = m_dir.filePath(QStringLiteral("saved.qet"));
	ElectricalModel model;
	{
		QETProject project(fixtureCopy(QStringLiteral("source-qet.qet")));
		model = sampleModel(project);
		QVERIFY(project.setElectricalModel(model));
		project.setFilePath(path);
		QVERIFY(project.write().isOk());
	}
	QETProject reopened(path);
	QCOMPARE(reopened.state(), QETProject::Ok);
	QVERIFY(reopened.electricalModel() == model);
	QCOMPARE(reopened.electricalReadReport().version, 1);
	QVERIFY(reopened.electricalReadReport().badValues.isEmpty());
		// the symbols it points at are still those of the drawing
	QSet<QUuid> drawn;
	for (Diagram *diagram : reopened.diagrams())
		for (Element *element : diagram->elements())
			drawn << element->uuid();
	QCOMPARE(model.referencedItems().size(), 3);
	QVERIFY(model.referencedItems().subtract(drawn).isEmpty());
	QVERIFY(reopened.electricalModel().danglingItems(drawn).isEmpty());
}

void tst_electricalproject::survivesQetz()
{
	const QString path = m_dir.filePath(QStringLiteral("saved.qetz"));
	ElectricalModel model;
	{
		QETProject project(fixtureCopy(QStringLiteral("source-qetz.qet")));
		model = sampleModel(project);
		QVERIFY(project.setElectricalModel(model));
		QString error;
		QVERIFY2(project.writeContainerFile(path, &error), qPrintable(error));
	}
	QETProject reopened(path);
	QCOMPARE(reopened.state(), QETProject::Ok);
	QVERIFY(reopened.electricalModel() == model);
}

void tst_electricalproject::survivesQetdb()
{
	const QString path = m_dir.filePath(QStringLiteral("saved.qetdb"));
	ElectricalModel model;
	{
		QETProject project(fixtureCopy(QStringLiteral("source-qetdb.qet")));
		model = sampleModel(project);
		QVERIFY(project.setElectricalModel(model));
		QString error;
		QVERIFY2(QetDbFile::save(*project.dataBase(), path, &error), qPrintable(error));
	}
	QETProject reopened(path);
	QCOMPARE(reopened.state(), QETProject::Ok);
	QVERIFY(reopened.electricalModel() == model);
}

void tst_electricalproject::survivesEveryConversion()
{
	// .qet -> .qetdb -> .qetz -> .qet, edited on the way
	const QString db = m_dir.filePath(QStringLiteral("chain.qetdb"));
	const QString zip = m_dir.filePath(QStringLiteral("chain.qetz"));
	const QString qet = m_dir.filePath(QStringLiteral("chain.qet"));
	QString error;
	ElectricalModel model;
	{
		QETProject project(fixtureCopy(QStringLiteral("chain-source.qet")));
		model = sampleModel(project);
		QVERIFY(project.setElectricalModel(model));
		QVERIFY2(QetDbFile::save(*project.dataBase(), db, &error), qPrintable(error));
	}
	{
		QETProject project(db);
		QVERIFY(project.electricalModel() == model);
		Circuit c = model.circuits().last();
		c.conductor.length = 12.5;   // edited in the .qetdb
		model.setCircuit(c);
		QVERIFY(project.setElectricalModel(model));
		QVERIFY2(project.writeContainerFile(zip, &error), qPrintable(error));
	}
	{
		QETProject project(zip);
		QVERIFY(project.electricalModel() == model);
		project.setFilePath(qet);
		QVERIFY(project.write().isOk());
	}
	QETProject last(qet);
	QVERIFY(last.electricalModel() == model);
	QCOMPARE(*last.electricalModel().circuits().last().conductor.length, 12.5);
}

void tst_electricalproject::otherUnknownBlocksAreKept()
{
	const QString foreign = QStringLiteral("<futurething a=\"1\"><b/></futurething>");
	const QString path = m_dir.filePath(QStringLiteral("foreign.qet"));
	{
		QETProject project(fixtureCopy(QStringLiteral("foreign-source.qet"), foreign));
		QVERIFY(project.setElectricalModel(sampleModel(project)));
		project.setFilePath(path);
		QVERIFY(project.write().isOk());
	}
	QETProject reopened(path);
	const QDomDocument document = reopened.toXml();
	QCOMPARE(blocksIn(document, QStringLiteral("futurething")), 1);
	QCOMPARE(blocksIn(document, ElectricalModel::tagName()), 1);
	QVERIFY(!reopened.electricalModel().isEmpty());
}

void tst_electricalproject::newerVersionIsNeverOverwritten()
{
	const QString newer = QStringLiteral(
		"<electrical version=\"99\"><circuit id=\"{11111111-1111-1111-1111-111111111111}\""
		" newthing=\"kept\"/></electrical>");
	const QString path = m_dir.filePath(QStringLiteral("newer.qet"));
	{
		QETProject project(fixtureCopy(QStringLiteral("newer-source.qet"), newer));
		QVERIFY(project.electricalReadReport().tooNew);
		QVERIFY(project.electricalModel().isReadOnly());
		QVERIFY(!project.setElectricalModel(sampleModel(project)));
		project.setFilePath(path);
		QVERIFY(project.write().isOk());
	}
	QETProject reopened(path);
	const QDomElement block = blockIn(reopened.toXml(), ElectricalModel::tagName());
	QCOMPARE(block.attribute(QStringLiteral("version")), QStringLiteral("99"));
	QCOMPARE(block.firstChildElement(QStringLiteral("circuit"))
			 .attribute(QStringLiteral("newthing")), QStringLiteral("kept"));
}

void tst_electricalproject::badValuesAreReported()
{
	const QString bad = QStringLiteral(
		"<electrical version=\"1\"><circuit id=\"{22222222-2222-2222-2222-222222222222}\""
		" voltage=\"doscientos\"><conductor section=\"2.5\"/></circuit></electrical>");
	QETProject project(fixtureCopy(QStringLiteral("bad.qet"), bad));
	QCOMPARE(project.state(), QETProject::Ok);
	const ReadReport report = project.electricalReadReport();
	QCOMPARE(report.badValues, QStringList{QStringLiteral("circuit[0]@voltage")});
	const Circuit c = project.electricalModel().circuits().first();
	QVERIFY(!c.voltage);
	QCOMPARE(*c.conductor.section, 2.5);
}

int main(int argc, char **argv)
{
	QStandardPaths::setTestModeEnabled(true);
	return QetAppTest::run<tst_electricalproject>(argc, argv);
}

#include "tst_electricalproject.moc"
