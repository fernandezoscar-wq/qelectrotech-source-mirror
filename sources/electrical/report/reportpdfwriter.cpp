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
#include "reportpdfwriter.h"
#include "reportlogo.h"

#include <QBuffer>
#include <QFontMetricsF>
#include <QPageLayout>
#include <QPainter>
#include <QPdfWriter>

#include <functional>
#include <vector>

using namespace electrical;

namespace {

	//A4 landscape, in mm
const double PAGE_W = 297.0;
const double PAGE_H = 210.0;
const double MARGIN = 10.0;
const double BLOCK_H = 16.0;   ///< title block
const double CONTENT_W = PAGE_W - 2 * MARGIN;
const double CONTENT_BOTTOM = PAGE_H - MARGIN - BLOCK_H - 4.0;
const double CELL_PAD = 0.8;

const QColor PASSED(0x1b, 0x7f, 0x3b);
const QColor FAILED(0xc6, 0x28, 0x28);
const QColor GREY(0x75, 0x75, 0x75);
const QColor HEADER_BG(0xe8, 0xe8, 0xe8);
const QColor LINE(0x90, 0x90, 0x90);

QPageLayout a4Landscape()
{
	return QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Landscape,
					   QMarginsF(0, 0, 0, 0));
}

QFont font(double points, bool bold = false)
{
	QFont f(QStringLiteral("Liberation Sans"));
	f.setPointSizeF(points);
	f.setBold(bold);
	return f;
}

/**
	Lays the report out in pages, then paints them: the number of pages
	is known before the first title block is painted.
*/
class Layout
{
	public:
		using Op = std::function<void(QPainter &)>;

		Layout(const ElectricalReport &report, QPdfWriter &writer) :
			m_report(report), m_writer(writer),
			m_dpi(writer.resolution()),
			m_logo(ReportLogo::image(report.info.logo)),
			m_measure(&m_measure_buffer)
		{
				//Every text is measured on the same device, whatever the
				//resolution of the writer: the annex alone and after the
				//folios then breaks its pages at the same places.
			m_measure.setResolution(MEASURE_DPI);
		}

			/// @p image in @p box (mm), whole and undistorted, to the left
			/// or the right of the box, centred vertically
		void drawFitted(QPainter &p, const QImage &image, const QRectF &box,
						Qt::Alignment align) const
		{
			if (image.isNull() || box.isEmpty()) return;
			QSizeF size = QSizeF(image.size()).scaled(box.size(), Qt::KeepAspectRatio);
			double x = align & Qt::AlignRight ? box.right() - size.width() : box.left();
			const double y = box.top() + (box.height() - size.height()) / 2;
			p.drawImage(rect(x, y, size.width(), size.height()), image);
		}

		double px(double mm) const { return mm * m_dpi / 25.4; }
		QRectF rect(double x, double y, double w, double h) const
		{ return QRectF(px(x), px(y), px(w), px(h)); }

		/// Height in mm of @p text wrapped in @p width mm
		double height(const QString &text, const QFont &f, double width) const
		{
			const QFontMetricsF metrics(f, &m_measure);
			const QRectF r = metrics.boundingRect(
				QRectF(0, 0, width * MEASURE_DPI / 25.4, 1e9), Qt::TextWordWrap, text);
			return r.height() * 25.4 / MEASURE_DPI;
		}

		void build()
		{
			cover();
			boards();
			notes();
			rules();
		}

		int paint(QPainter &painter, bool new_page_first)
		{
			const int count = int(m_pages.size());
			for (int i = 0; i < count; ++i) {
				if (i > 0 || new_page_first) {
					m_writer.setPageLayout(a4Landscape());
					m_writer.newPage();
				}
				painter.save();
				for (const Op &op : m_pages[size_t(i)])
					op(painter);
				titleBlock(painter, i + 1, count);
				painter.restore();
			}
			return count;
		}

	private:
		void newPage()
		{
			m_pages.emplace_back();
			m_y = MARGIN;
		}

		Op textOp(const QString &text, const QFont &f, const QColor &color,
				  double x, double y, double w, double h,
				  int flags = Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop)
		{
			const QRectF r = rect(x, y, w, h);
			return [=](QPainter &p) {
				p.setFont(f);
				p.setPen(color);
				p.drawText(r, flags, text);
			};
		}

			/// A paragraph at the cursor, on a new page if it does not fit
		void paragraph(const QString &text, const QFont &f,
					   const QColor &color = Qt::black, double indent = 0,
					   double after = 1.0)
		{
			const double w = CONTENT_W - indent;
			const double h = height(text, f, w);
			if (m_y + h > CONTENT_BOTTOM)
				newPage();
			m_pages.back().push_back(textOp(text, f, color, MARGIN + indent, m_y, w, h + 1));
			m_y += h + after;
		}

		void cover()
		{
			newPage();
			const ReportInfo &info = m_report.info;
			if (!m_logo.isNull()) {
				const QImage logo = m_logo;
				m_pages.back().push_back([this, logo](QPainter &p) {
					drawFitted(p, logo, QRectF(PAGE_W - MARGIN - 80, MARGIN, 80, 32),
							   Qt::AlignRight);
				});
			}
			m_y = m_logo.isNull() ? 30 : 48;
			paragraph(info.documentTitle, font(22, true), Qt::black, 0, 4);
			if (!m_report.projectTitle.isEmpty())
				paragraph(m_report.projectTitle, font(15), Qt::black, 0, 2);
			if (!info.company.isEmpty())
				paragraph(info.company, font(12), GREY, 0, 8);
			else
				m_y += 6;

			const QList<QPair<QString, QString>> fields{
				{ReportPdfWriter::tr("Plan number"), info.planNumber},
				{ReportPdfWriter::tr("Revision"), info.revision},
				{ReportPdfWriter::tr("Date"), info.date},
				{ReportPdfWriter::tr("Author"), info.author}};
			for (const auto &field : fields) {
				m_pages.back().push_back(textOp(field.first, font(10), GREY,
												MARGIN, m_y, 45, 6));
				m_pages.back().push_back(textOp(field.second, font(10, true), Qt::black,
												MARGIN + 45, m_y, 150, 6));
				m_y += 6;
			}
			m_y += 6;

				//Summary of the checks
			paragraph(ReportPdfWriter::tr("Summary: %n circuit(s)", "",
										  m_report.circuitCount), font(12, true));
			const double col[] = {110, 30, 30, 30};
			const QStringList head{QString(), ReportPdfWriter::tr("Passed"),
								   ReportPdfWriter::tr("Not passed"),
								   ReportPdfWriter::tr("Not performed")};
			auto row = [&](const QStringList &cells, bool bold) {
				double x = MARGIN;
				for (int i = 0; i < cells.size(); ++i) {
					m_pages.back().push_back(textOp(cells.at(i), font(10, bold),
						i == 2 && !bold && cells.at(i) != QLatin1String("0") ? FAILED : Qt::black,
						x, m_y, col[i], 6,
						(i == 0 ? Qt::AlignLeft : Qt::AlignRight) | Qt::AlignVCenter));
					x += col[i];
				}
				const double y_line = m_y + 6;
				m_pages.back().push_back([=](QPainter &p) {
					p.setPen(QPen(LINE, px(0.15)));
					p.drawLine(QPointF(px(MARGIN), px(y_line)),
							   QPointF(px(MARGIN + 200), px(y_line)));
				});
				m_y += 6.5;
			};
			auto counts = [](const CheckCounts &c) {
				return QStringList{QString::number(c.passed), QString::number(c.notPassed),
								   QString::number(c.notPerformed)};
			};
			row(head, true);
			row(QStringList{ReportPdfWriter::tr("Voltage drop from the origin (ITC-BT-19, 14, 15)")}
				+ counts(m_report.drop), false);
			row(QStringList{ReportPdfWriter::tr("IB ≤ IZ (ITC-BT-19 2.2.3)")}
				+ counts(m_report.ampacity), false);
			row(QStringList{ReportPdfWriter::tr("Protection ↔ cable (ITC-BT-22)")}
				+ counts(m_report.coordination), false);
			row(QStringList{ReportPdfWriter::tr("Short circuit (ITC-BT-22 1.1 b, UNE-HD 60364-4-43)")}
				+ counts(m_report.shortCircuit), false);
			if (m_report.pv.passed + m_report.pv.notPassed + m_report.pv.notPerformed > 0)
				row(QStringList{ReportPdfWriter::tr("PV generators, DC side "
													"(UNE-HD 60364-7-712, IDAE)")}
					+ counts(m_report.pv), false);
			if (m_report.earthingGiven)
				row(QStringList{ReportPdfWriter::tr("Indirect contacts (ITC-BT-24 4.1)")}
					+ counts(m_report.indirect), false);
			if (m_report.dwelling.passed + m_report.dwelling.notPassed
				+ m_report.dwelling.notPerformed > 0)
				row(QStringList{ReportPdfWriter::tr("Dwelling circuits (ITC-BT-25)")}
					+ counts(m_report.dwelling), false);
			if (m_report.dwellings.passed + m_report.dwellings.notPassed
				+ m_report.dwellings.notPerformed > 0)
				row(QStringList{ReportPdfWriter::tr("Dwellings as a whole (ITC-BT-25)")}
					+ counts(m_report.dwellings), false);
			m_y += 5;

			paragraph(ReportPdfWriter::tr("Ampacity tables read"), font(10, true));
			for (const QString &t : m_report.tables)
				paragraph(QStringLiteral("• ") + t, font(9), Qt::black, 3, 0.5);
			m_y += 4;

				//The disclaimer, framed
			const double h = height(m_report.disclaimer, font(9), CONTENT_W - 6);
			if (m_y + h + 6 > CONTENT_BOTTOM) newPage();
			const QRectF frame = rect(MARGIN, m_y, CONTENT_W, h + 6);
			m_pages.back().push_back([=](QPainter &p) {
				p.setPen(QPen(FAILED, px(0.3)));
				p.setBrush(Qt::NoBrush);
				p.drawRect(frame);
			});
			m_pages.back().push_back(textOp(m_report.disclaimer, font(9), Qt::black,
											MARGIN + 3, m_y + 3, CONTENT_W - 6, h + 1));
			m_y += h + 8;
		}

			/// Numbers right aligned: U, P, Ib, L, Iz, the drops, the Icc
		static bool isNumber(int column)
		{
			return (column >= 3 && column != 6 && column != 7 && column < 12)
				   || column == 15 || column == 16;
		}

		void tableHeader()
		{
			const QFont f = font(7, true);
			double h = 0;
			for (int i = 0; i < m_report.columns.size(); ++i)
				h = qMax(h, height(m_report.columns.at(i), f,
								   m_report.columnWidths.at(i) - 2 * CELL_PAD));
			h += 2 * CELL_PAD;
			double x = MARGIN;
			const double y = m_y;
			m_pages.back().push_back([=](QPainter &p) {
				p.setPen(Qt::NoPen);
				p.setBrush(HEADER_BG);
				p.drawRect(rect(MARGIN, y, CONTENT_W, h));
				p.setBrush(Qt::NoBrush);
			});
			for (int i = 0; i < m_report.columns.size(); ++i) {
				const double w = m_report.columnWidths.at(i);
				m_pages.back().push_back(textOp(m_report.columns.at(i), f, Qt::black,
												x + CELL_PAD, y + CELL_PAD,
												w - 2 * CELL_PAD, h,
												Qt::TextWordWrap | Qt::AlignTop
													| (isNumber(i) ? Qt::AlignRight
																   : Qt::AlignLeft)));
				x += w;
			}
			m_y += h;
		}

		void boardTitle(const BoardSection &board, bool continued)
		{
			QString title = board.title;
			if (continued)
				title += QLatin1Char(' ') + ReportPdfWriter::tr("(continued)");
			paragraph(title, font(11, true), Qt::black, 0, 0.5);
			if (!board.subtitle.isEmpty() && !continued)
				paragraph(board.subtitle, font(8), GREY, 0, 1.5);
			tableHeader();
		}

		void boards()
		{
			newPage();
			paragraph(ReportPdfWriter::tr("Characteristics of the boards"), font(15, true),
					  Qt::black, 0, 3);
			const QFont f = font(7);
			const QFont bold = font(7, true);
			for (const BoardSection &board : m_report.boards) {
				if (m_y + 25 > CONTENT_BOTTOM) newPage();
				boardTitle(board, false);
				if (board.rows.isEmpty())
					paragraph(ReportPdfWriter::tr("No circuit."), font(8), GREY, 0, 2);
				for (const ReportRow &row : board.rows) {
					double h = 0;
					for (int i = 0; i < row.size(); ++i)
						h = qMax(h, height(row.at(i).text,
										   row.at(i).state == CellState::NotPassed ? bold : f,
										   m_report.columnWidths.at(i) - 2 * CELL_PAD));
					h += 2 * CELL_PAD;
					if (m_y + h > CONTENT_BOTTOM) {
						newPage();
						boardTitle(board, true);
					}
					double x = MARGIN;
					for (int i = 0; i < row.size(); ++i) {
						const double w = m_report.columnWidths.at(i);
						const ReportCell &cell = row.at(i);
						QColor color = Qt::black;
						if (cell.state == CellState::Passed) color = PASSED;
						if (cell.state == CellState::NotPassed) color = FAILED;
						if (cell.state == CellState::NotPerformed) color = GREY;
						const bool number = isNumber(i);
						m_pages.back().push_back(textOp(
							cell.text, cell.state == CellState::NotPassed ? bold : f, color,
							x + CELL_PAD, m_y + CELL_PAD, w - 2 * CELL_PAD, h,
							Qt::TextWordWrap | Qt::AlignTop
								| (number ? Qt::AlignRight : Qt::AlignLeft)));
						x += w;
					}
					const double y_line = m_y + h;
					m_pages.back().push_back([=](QPainter &p) {
						p.setPen(QPen(LINE, px(0.1)));
						p.drawLine(QPointF(px(MARGIN), px(y_line)),
								   QPointF(px(MARGIN + CONTENT_W), px(y_line)));
					});
					m_y += h;
				}
				m_y += 5;
			}
		}

		void notes()
		{
			newPage();
			paragraph(ReportPdfWriter::tr("Results and warnings, circuit by circuit"),
					  font(15, true), Qt::black, 0, 3);
				//Each dwelling as a whole first
			for (const DwellingNotes &d : m_report.dwellingNotes) {
				const QString title = ReportPdfWriter::tr("Dwelling of %1 (ITC-BT-25): %2")
										  .arg(d.title, d.summary);
				if (m_y + height(title, font(10, true), CONTENT_W) + 8 > CONTENT_BOTTOM)
					newPage();
				paragraph(title, font(10, true),
						  d.outcome == rebt::CheckOutcome::NotPassed ? FAILED : Qt::black,
						  0, 0.8);
				for (const QString &line : d.lines)
					paragraph(line, font(8), Qt::black, 3, 0.3);
				m_y += 3;
			}
			for (const CircuitNotes &n : m_report.notes) {
					//The title is never left alone at the foot of a page
				const double first = height(n.title, font(10, true), CONTENT_W)
									 + (n.lines.isEmpty() ? 0
										: height(n.lines.first(), font(8), CONTENT_W - 4));
				if (m_y + first + 2 > CONTENT_BOTTOM) newPage();
				paragraph(n.title, font(10, true), n.hasProblem ? FAILED : Qt::black, 0, 0.8);
				for (const QString &line : n.lines) {
					QColor color = Qt::black;
					if (line.startsWith(QLatin1String("    "))) color = GREY;
					paragraph(line.trimmed(), font(8), color,
							  line.startsWith(QLatin1String("    ")) ? 8 : 3, 0.3);
				}
				m_y += 3;
			}
		}

		void rules()
		{
			newPage();
			paragraph(ReportPdfWriter::tr("Rules applied"), font(15, true), Qt::black, 0, 3);
			for (const rebt::NormRef &r : m_report.rules) {
				QString head = QString::fromStdString(r.id) + QStringLiteral(" — ")
							   + QString::fromStdString(r.source);
				if (r.section != "-")
					head += QLatin1Char(' ') + QString::fromStdString(r.section);
				const double first = height(head, font(10, true), CONTENT_W)
									 + height(QString::fromStdString(r.version), font(8), CONTENT_W - 4);
				if (m_y + first + 2 > CONTENT_BOTTOM) newPage();
				paragraph(head, font(10, true), Qt::black, 0, 0.5);
				paragraph(ReportPdfWriter::tr("Text: %1").arg(QString::fromStdString(r.version)),
						  font(8), Qt::black, 4, 0.3);
				paragraph(ReportPdfWriter::tr("Limitations: %1").arg(QString::fromStdString(r.limitations)),
						  font(8), GREY, 4, 3);
			}
			m_y += 3;
			paragraph(m_report.disclaimer, font(9), FAILED);
		}

		void titleBlock(QPainter &p, int page, int count) const
		{
			const ReportInfo &info = m_report.info;
			const double y = PAGE_H - MARGIN - BLOCK_H;
			const QList<QPair<QString, QString>> cells{
				{ReportPdfWriter::tr("Company"), info.company},
				{ReportPdfWriter::tr("Project"), m_report.projectTitle},
				{ReportPdfWriter::tr("Document"), info.documentTitle},
				{ReportPdfWriter::tr("Plan no."), info.planNumber},
				{ReportPdfWriter::tr("Rev."), info.revision},
				{ReportPdfWriter::tr("Date"), info.date},
				{ReportPdfWriter::tr("Author"), info.author},
				{ReportPdfWriter::tr("Page"), ReportPdfWriter::tr("%1 / %2").arg(page).arg(count)}};
			const double plain[] = {48, 70, 52, 28, 13, 22, 26, 18};    // 277 mm
			const double with_logo[] = {38, 62, 48, 26, 12, 21, 24, 18}; // + 28 mm of logo
			const double *widths = m_logo.isNull() ? plain : with_logo;
			p.setPen(QPen(Qt::black, px(0.3)));
			p.setBrush(Qt::NoBrush);
			p.drawRect(rect(MARGIN, y, CONTENT_W, BLOCK_H));
			double x = MARGIN;
			if (!m_logo.isNull()) {
				drawFitted(p, m_logo, QRectF(x + 1.5, y + 1.5, 28 - 3, BLOCK_H - 3),
						   Qt::AlignLeft);
				x += 28;
			}
			const QFont label = font(6);
			const QFont value = font(8.5, true);
			for (int i = 0; i < cells.size(); ++i) {
				const double w = widths[i];
				if (i > 0 || !m_logo.isNull()) {
					p.setPen(QPen(Qt::black, px(0.2)));
					p.drawLine(QPointF(px(x), px(y)), QPointF(px(x), px(y + BLOCK_H)));
				}
				p.setFont(label);
				p.setPen(GREY);
				p.drawText(rect(x + 1.2, y + 1, w - 2.4, 3), Qt::AlignLeft | Qt::AlignTop,
						   cells.at(i).first);
				p.setFont(value);
				p.setPen(Qt::black);
				const QFontMetricsF metrics(value, p.device());
					//Two lines at most; what does not fit ends with "..."
				p.drawText(rect(x + 1.2, y + 4.5, w - 2.4, BLOCK_H - 5.5),
						   Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
						   metrics.elidedText(cells.at(i).second, Qt::ElideRight,
											  2 * px(w - 2.4) - metrics.averageCharWidth()));
				x += w;
			}
		}

		const ElectricalReport &m_report;
		QPdfWriter &m_writer;
		const int m_dpi;
		static constexpr int MEASURE_DPI = 600;
		const QImage m_logo;
		QBuffer m_measure_buffer;   ///< never written to a file
		QPdfWriter m_measure;
		std::vector<std::vector<Op>> m_pages;
		double m_y = MARGIN;
};

void setMetadata(QPdfWriter &writer, const ElectricalReport &report)
{
	writer.setCreator(QStringLiteral("QElectroTech"));
	writer.setTitle(report.info.documentTitle
					+ (report.projectTitle.isEmpty() ? QString()
													 : QStringLiteral(" — ") + report.projectTitle));
	writer.setAuthor(report.info.author);
}

} // namespace

bool ReportPdfWriter::write(const ElectricalReport &report, const QString &path,
							QString *error)
{
	QPdfWriter writer(path);
	writer.setResolution(600);
	writer.setPageLayout(a4Landscape());
	setMetadata(writer, report);
	QPainter painter;
	if (!painter.begin(&writer)) {
		if (error) *error = tr("Cannot write %1").arg(path);
		return false;
	}
	Layout layout(report, writer);
	layout.build();
	layout.paint(painter, false);
	painter.end();
	return true;
}

int ReportPdfWriter::append(const ElectricalReport &report, QPdfWriter &writer,
							QPainter &painter)
{
	Layout layout(report, writer);
	layout.build();
	return layout.paint(painter, true);
}
