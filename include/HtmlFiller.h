#pragma once
#include "XlsxReader.h"
#include <QString>
#include <QVector>

struct FillResult {
    QString html;
    QStringList warnings; // fit warnings, etc.
};

class HtmlFiller {
public:
    // Fills the GOST template (sheet1 unique + sheet2 repeated) with BOM items.
    // nameField = Type + Comment + Manufacturer
    // Returns HTML and any fit warnings.
    static FillResult fill(const QString &templateHtml, const QVector<BomItem> &items);
};
