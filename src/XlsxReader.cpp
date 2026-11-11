#include "XlsxReader.h"
#include "xlsxdocument.h"
#include <QDebug>

using namespace QXlsx;

#include <QMap>
#include <QStringList>

QVector<BomItem> XlsxReader::read(const QString &path, QString *error) {
    QVector<BomItem> items;
    QXlsx::Document xlsx(path);

    if (!xlsx.load()) {
        if (error) *error = "Не удалось открыть XLSX: " + path;
        return items;
    }

    QXlsx::CellRange dim = xlsx.dimension();
    int rowCount = dim.rowCount();
    int colCount = dim.columnCount();

    if (rowCount < 2) {
        if (error) *error = "В файле нет строк данных (требуется минимум заголовок + 1 строка)";
        return items;
    }

    QMap<QString, int> headerMap;
    for (int c = 1; c <= colCount; ++c) {
        QString header = xlsx.read(1, c).toString().trimmed().toLower();
        if (!header.isEmpty()) {
            headerMap[header] = c;
        }
    }

    auto getCol = [&headerMap](const QStringList &possibleNames) -> int {
        for (const QString &name : possibleNames) {
            int idx = headerMap.value(name.toLower(), -1);
            if (idx != -1) {
                return idx;
            }
        }
        return -1; // Не найдено
    };

    int colType         = getCol({"type", "тип"});
    int colComment      = getCol({"comment", "комментарий", "примечание"});
    int colDescription  = getCol({"description", "описание"});
    int colOboznach     = getCol({"обозначение", "oboznach", "designation"});
    int colOboznachVp   = getCol({"обозначение вп", "oboznach vp", "vp designation"});
    int colQuantity     = getCol({"quantity", "кол-во", "количество", "qty"});
    int colManufacturer = getCol({"manufacturer", "производитель", "vendor", "maker"});
    int colDesignator   = getCol({"designator", "позиция", "ref", "reference"});
    int colLibRef       = getCol({"libref", "библиотека", "library ref", "library"});

    if (colQuantity == -1 && colOboznach == -1 && colDesignator == -1) {
        if (error) *error = "Не найдено ни одного распознаваемого столбца данных. Проверьте заголовки.";
        return items;
    }

    for (int r = 2; r <= rowCount; ++r) {
        BomItem it;

        it.type         = (colType != -1)         ? xlsx.read(r, colType).toString().trimmed()         : QString();
        it.comment      = (colComment != -1)      ? xlsx.read(r, colComment).toString().trimmed()      : QString();
        it.description  = (colDescription != -1)  ? xlsx.read(r, colDescription).toString().trimmed()  : QString();
        it.oboznach     = (colOboznach != -1)     ? xlsx.read(r, colOboznach).toString().trimmed()     : QString();
        it.oboznachVp   = (colOboznachVp != -1)   ? xlsx.read(r, colOboznachVp).toString().trimmed()   : QString();
        it.quantity     = (colQuantity != -1)     ? xlsx.read(r, colQuantity).toString().trimmed()     : QString();
        it.manufacturer = (colManufacturer != -1) ? xlsx.read(r, colManufacturer).toString().trimmed() : QString();
        it.designator   = (colDesignator != -1)   ? xlsx.read(r, colDesignator).toString().trimmed()   : QString();
        it.libRef       = (colLibRef != -1)       ? xlsx.read(r, colLibRef).toString().trimmed()       : QString();

        if (it.oboznach.isEmpty() && it.designator.isEmpty() && it.type.isEmpty() && it.comment.isEmpty()) {
            continue;
        }

        items.append(it);
    }

    return items;
}

QDebug operator<<(QDebug dbg, const BomItem &item) {
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "BomItem("
                  << "type=" << item.type
                  << ", comment=" << item.comment
                  << ", desc=" << item.description
                  << ", oboznach=" << item.oboznach
                  << ", oboznachVp=" << item.oboznachVp
                  << ", qty=" << item.quantity
                  << ", mfr=" << item.manufacturer
                  << ", designator=" << item.designator
                  << ", libRef=" << item.libRef
                  << ")";
    return dbg;
}
