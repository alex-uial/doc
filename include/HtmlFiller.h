#pragma once
#include "XlsxReader.h"
#include <QString>
#include <QVector>

struct FillResult {
    QString html;
    QStringList warnings; // fit warnings, etc.
    int pagesUsed = 0;    // sheets written into the document
    int capacityPerPage = 0; // data slots on a continuation sheet
};

class HtmlFiller {
public:
    // Fills the GOST template (sheet1 unique + sheet2 repeated) with BOM items.
    // nameField = Type + Comment + Manufacturer
    // Items must be pre-sorted by caller.
    // Returns HTML and any fit warnings.
    static FillResult fill(const QString &templateHtml, const QVector<BomItem> &items);
};
