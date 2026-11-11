// xlsxzipreader_p.h — internal wrapper over miniz ZIP reader.
// No Qt private headers required (portable across MSVC / MinGW / Linux).

#ifndef QXLSX_XLSXZIPREADER_P_H
#define QXLSX_XLSXZIPREADER_P_H

#include "xlsxglobal.h"

#include <QByteArray>
#include <QIODevice>
#include <QString>
#include <QStringList>

QT_BEGIN_NAMESPACE_XLSX

class ZipData;

class ZipReader
{
public:
    explicit ZipReader(const QString &fileName);
    explicit ZipReader(QIODevice *device);
    ~ZipReader();
    bool exists() const;
    QStringList filePaths() const;
    QByteArray fileData(const QString &fileName) const;

private:
    Q_DISABLE_COPY(ZipReader)
    void init();
    QScopedPointer<ZipData> m_d;
    QStringList m_filePaths;
};

QT_END_NAMESPACE_XLSX

#endif // QXLSX_XLSXZIPREADER_P_H
