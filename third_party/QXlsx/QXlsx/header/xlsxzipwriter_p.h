// xlsxzipwriter_p.h — internal wrapper over miniz ZIP writer (writes to a QIODevice).
// No Qt private headers required (portable across MSVC / MinGW / Linux).

#ifndef QXLSX_ZIPWRITER_H
#define QXLSX_ZIPWRITER_H

#include "xlsxglobal.h"

#include <QByteArray>
#include <QIODevice>
#include <QString>

QT_BEGIN_NAMESPACE_XLSX

class ZipWriter
{
public:
    explicit ZipWriter(const QString &filePath);
    explicit ZipWriter(QIODevice *device);
    ~ZipWriter();

    void addFile(const QString &filePath, QIODevice *device);
    void addFile(const QString &filePath, const QByteArray &data);
    bool error() const;
    void close();

private:
    Q_DISABLE_COPY(ZipWriter)
    void finish();

    QIODevice *m_device = nullptr;
    QFile *m_file = nullptr;  // owned file when constructed from a path
    bool m_ownsDevice = false;
    QByteArray m_entries;   // serialized local file headers + data
    QByteArray m_central;   // serialized central directory
    quint16 m_numEntries = 0;
    quint32 m_offset = 0;   // running offset into the archive body
    bool m_finished = false;
    bool m_error = false;
};

QT_END_NAMESPACE_XLSX

#endif // QXLSX_ZIPWRITER_H
