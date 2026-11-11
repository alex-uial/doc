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
    QVector<DataSlot> slots;
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
        slots.append(s);
    }
    return slots;
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

    QDomDocument doc;
    QString err;
    int errLine = 0;
    if (!doc.setContent(templateHtml, &err, &errLine)) {
        res.html = templateHtml; // fallback
        res.warnings << QString("Не удалось разобрать шаблон HTML: %1 (строка %2)").arg(err).arg(errLine);
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

    // Remove original sheets from body; we will rebuild
    QDomElement newBody = doc.createElement("body");
    // Keep any non-section content? usually none
    while (body.hasChildNodes()) {
        QDomNode n = body.firstChild();
        if (n.isElement() && n.toElement().tagName() == "section") {
            body.removeChild(n);
        } else {
            newBody.appendChild(n);
        }
    }

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
        QVector<DataSlot> slots = findDataSlots(sheet);
        int filled = 0;
        for (const DataSlot &slot : slots) {
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
    newBody.appendChild(s0);

    // Continuation sheets (repeat sheet1)
    while (itemIdx < items.size()) {
        QDomElement s = sheet1.cloneNode(true).toElement();
        int filled = fillSheet(s, false);
        if (filled == 0) break; // no more slots
        newBody.appendChild(s);
    }

    // Replace body
    QDomElement htmlRoot = doc.documentElement();
    htmlRoot.replaceChild(newBody, body);

    res.html = doc.toString(0);
    if (itemIdx < items.size()) {
        res.warnings << QString("Не все элементы поместились: осталось %1").arg(items.size() - itemIdx);
    }
    return res;
}
