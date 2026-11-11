#pragma once
#include <QString>
#include <QVector>
#include <QMap>

struct BomItem {
    QString type;
    QString comment;
    QString description;
    QString oboznach;
    QString oboznachVp;
    QString quantity;
    QString manufacturer;
    QString designator;
    QString libRef;
};

QDebug operator<<(QDebug dbg, const BomItem &item);

class XlsxReader {
public:
    static QVector<BomItem> read(const QString &path, QString *error = nullptr);
};
