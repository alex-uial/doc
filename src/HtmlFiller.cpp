#include "HtmlFiller.h"
#include "DesignatorParser.h"
#include <QDomDocument>
#include <QDomElement>
#include <QRegularExpression>
#include <QDebug>
#include <algorithm>

namespace {

struct DataSlot {
    QDomElement cellPos;   // C column
    QDomElement cellName;  // F
    QDomElement cellQty;   // J
    QDomElement cellNote;  // L or K
    double widthPos = 20.0;
    double widthName = 110.0;
    double height = 8.0;
};

double extractMm(const QString &style, const QString &key) {
    int idx = style.indexOf(key + ":");
    if (idx < 0) return 0;
    QString rest = style.mid(idx + key.size() + 1);
    return rest.section("mm", 0, 0).toDouble();
}

QVector<DataSlot> findDataSlots(QDomElement sheet) {
    QVector<DataSlot> dataSlots;
    // Collect cells by data-row
    QMap<int, QMap<QString, QDomElement>> rows;
    QDomNodeList cells = sheet.elementsByTagName("div");
    for (int i = 0; i < cells.size(); ++i) {
        QDomElement c = cells.at(i).toElement();
        if (!c.hasAttribute("data-addr")) continue;
        int row = c.attribute("data-row").toInt();
        QString col = c.attribute("data-col");
        rows[row][col] = c;
    }
    for (auto it = rows.begin(); it != rows.end(); ++it) {
        const auto &cols = it.value();
        if (!cols.contains("2") || !cols.contains("5") || !cols.contains("9")) continue;
        QString noteCol = cols.contains("11") ? "11" : (cols.contains("10") ? "10" : QString());
        if (noteCol.isEmpty()) continue;
        QDomElement posCell = cols["2"];
        if (!posCell.text().trimmed().isEmpty()) continue; // already filled or header

        DataSlot s;
        s.cellPos = posCell;
        s.cellName = cols["5"];
        s.cellQty = cols["9"];
        s.cellNote = cols[noteCol];
        s.widthPos = extractMm(posCell.attribute("style"), "width");
        s.widthName = extractMm(s.cellName.attribute("style"), "width");
        s.height = extractMm(posCell.attribute("style"), "height");
        if (s.widthPos < 1) s.widthPos = 20;
        if (s.widthName < 1) s.widthName = 110;
        if (s.height < 1) s.height = 8;
        dataSlots.append(s);
    }
    return dataSlots;
}

void setCellText(QDomElement cell, const QString &text, bool wrap) {
    // Clear children
    while (cell.hasChildNodes()) cell.removeChild(cell.firstChild());
    cell.appendChild(cell.ownerDocument().createTextNode(text));
    if (wrap) {
        QString cls = cell.attribute("class");
        if (!cls.contains("wr")) {
            cell.setAttribute("class", (cls + " wr").trimmed());
        }
    }
}

// Rough fit check: ~2.2 mm per char at 12pt, height allows ~2 lines at 8mm
bool fits(const QString &text, double widthMm, double heightMm) {
    if (text.isEmpty()) return true;
    const double charWidth = 2.3; // mm
    const double lineHeight = 4.0; // mm
    int maxCharsPerLine = qMax(1, int(widthMm / charWidth));
    int maxLines = qMax(1, int(heightMm / lineHeight));
    int maxChars = maxCharsPerLine * maxLines;
    return text.length() <= maxChars;
}

} // namespace

FillResult HtmlFiller::fill(const QString &templateHtml, const QVector<BomItem> &items) {
    FillResult res;
    if (templateHtml.isEmpty()) {
        res.html = "<html><body>Шаблон не загружен</body></html>";
        return res;
    }

    // The template is HTML, not XHTML: <meta>, <br>, <col ...> may be left
    // unclosed and CSS inside <style> contains raw '<'. QDomDocument needs
    // well-formed XML, so sanitize before parsing; otherwise we get
    // "tag mismatch (line ~20)" and an empty output document.
    QString sanitized = templateHtml;

    // 1) Self-close void tags that are not already closed
    static const QRegularExpression voidTagRe(
        "<\\s*(meta|br|hr|img|input|link|col|area|base|embed|source|track|wbr)\\b([^<>]*?)(/?)>",
        QRegularExpression::CaseInsensitiveOption);
    sanitized = sanitized.replace(voidTagRe, "<$1$2/>");

    // 2) Hide CSS/comments from the XML parser (they may contain '<', '&')
    static const QRegularExpression styleRe("<style[^>]*>.*?</style>",
                                            QRegularExpression::DotMatchesEverythingOption
                                            | QRegularExpression::CaseInsensitiveOption);
    sanitized = sanitized.replace(styleRe, "<style/>");
    static const QRegularExpression commentRe("<!--.*?-->",
                                              QRegularExpression::DotMatchesEverythingOption);
    sanitized = sanitized.replace(commentRe, QString());

    QDomDocument doc;
    QString err;
    int errLine = 0, errCol = 0;
    // Namespace processing must stay OFF: the exported template has no xmlns,
    // so namespaced parsing dies at the first void tag (<meta> — line 20).
    if (!doc.setContent(sanitized, false, &err, &errLine, &errCol)) {
        res.html = templateHtml; // fallback: original untouched template
        res.warnings << QString("Не удалось разобрать шаблон HTML: %1 (строка %2, колонка %3). "
                                "В выходном файле данные НЕ заполнены — проверьте шаблон.")
                        .arg(err).arg(errLine).arg(errCol);
        return res;
    }

    QDomElement body = doc.documentElement().firstChildElement("body");
    if (body.isNull()) body = doc.documentElement();

    QDomNodeList sections = body.elementsByTagName("section");
    if (sections.size() < 2) {
        res.warnings << "В шаблоне меньше двух листов";
        res.html = templateHtml;
        return res;
    }

    QDomElement sheet0 = sections.at(0).toElement();
    QDomElement sheet1 = sections.at(1).toElement();

    // Original sheets are removed from the body right before serialization;
    // we work on deep clones and append them to the same body.

    auto processItem = [](const BomItem &it) -> QStringList {
        QString pos = DesignatorParser::groupAndSort(it.designator);
        QStringList nameParts;
        if (!it.type.isEmpty()) nameParts << it.type;
        if (!it.comment.isEmpty()) nameParts << it.comment;
        if (!it.manufacturer.isEmpty()) nameParts << it.manufacturer;
        QString name = nameParts.join(" ");
        QString qty = it.quantity;
        return {pos, name, qty, QString()}; // note empty
    };

    int itemIdx = 0;
    auto fillSheet = [&](QDomElement sheet, bool isFirst) -> int {
        QVector<DataSlot> dataSlots = findDataSlots(sheet);
        if (!isFirst) res.capacityPerPage = qMax(res.capacityPerPage, dataSlots.size());
        int filled = 0;
        for (const DataSlot &slot : dataSlots) {
            if (itemIdx >= items.size()) break;
            const BomItem &it = items[itemIdx];
            auto fields = processItem(it);
            QString pos = fields[0];
            QString name = fields[1];
            QString qty = fields[2];

            bool posFit = fits(pos, slot.widthPos, slot.height);
            bool nameFit = fits(name, slot.widthName, slot.height);
            if (!posFit) {
                res.warnings << QString("Поз. «%1» может не поместиться (ширина ~%2 мм)")
                                .arg(pos.left(30)).arg(slot.widthPos, 0, 'f', 1);
            }
            if (!nameFit) {
                res.warnings << QString("Наименование «%1…» может не поместиться (ширина ~%2 мм)")
                                .arg(name.left(40)).arg(slot.widthName, 0, 'f', 1);
            }

            setCellText(slot.cellPos, pos, !posFit);
            setCellText(slot.cellName, name, !nameFit);
            setCellText(slot.cellQty, qty, false);
            setCellText(slot.cellNote, QString(), false);
            ++itemIdx;
            ++filled;
        }
        return filled;
    };

    // First sheet (unique)
    QDomElement s0 = sheet0.cloneNode(true).toElement();
    fillSheet(s0, true);
    body.appendChild(s0);
    ++res.pagesUsed;

    // Continuation sheets (repeat sheet2 of the template until everything fits)
    while (itemIdx < items.size()) {
        QDomElement s = sheet1.cloneNode(true).toElement();
        int filled = fillSheet(s, false);
        if (filled == 0) break; // no more dataSlots
        body.appendChild(s);
        ++res.pagesUsed;
    }

    // Drop the two original template sheets (clones are already in the body)
    body.removeChild(sheet0);
    body.removeChild(sheet1);

    // Restore the original <style> block (it was replaced by <style/> during
    // sanitization so the XML parser could handle raw CSS).
    QDomElement styleEl = doc.documentElement().elementsByTagName("head")
                              .at(0).toElement().firstChildElement("style");
    if (!styleEl.isNull()) {
        static const QRegularExpression origStyleRe("<style[^>]*>(.*?)</style>",
                                                    QRegularExpression::DotMatchesEverythingOption
                                                    | QRegularExpression::CaseInsensitiveOption);
        auto m = origStyleRe.match(templateHtml);
        if (m.hasMatch()) {
            while (styleEl.hasChildNodes()) styleEl.removeChild(styleEl.firstChild());
            styleEl.appendChild(doc.createTextNode(m.captured(1)));
        }
    }

    QString out = doc.toString(0);
    // <!DOCTYPE html> is mandatory: without it browsers render in quirks mode
    // and the absolutely-positioned cell layout collapses.
    if (!out.startsWith("<!DOCTYPE", Qt::CaseInsensitive))
        out = "<!DOCTYPE html>\n" + out;
    res.html = out;
    if (itemIdx < items.size()) {
        res.warnings << QString("Не все элементы поместились: осталось %1").arg(items.size() - itemIdx);
    }
    return res;
}
