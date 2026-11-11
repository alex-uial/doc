// xlsxzipreader.cpp — reimplemented on top of miniz (public domain),
// so QXlsx no longer needs Qt private headers (private/qzipreader_p.h).
// This makes the library buildable with ANY Qt5/Qt6 kit: MSVC, MinGW (msys2), Linux.

#include "xlsxzipreader_p.h"

#include <QFile>
#include <QScopedPointer>

#include "../3rdparty/miniz.h"

QT_BEGIN_NAMESPACE_XLSX

struct ReaderData
{
    QByteArray buffer;                       // whole file kept in memory (xlsx files are small)
    mz_zip_archive archive;
    bool archiveInited = false;

    ~ReaderData()
    {
        if (archiveInited)
            mz_zip_end(&archive);
    }
};

ZipReader::ZipReader(const QString &filePath)
    : m_d(new ReaderData)
{
    QFile f(filePath);
    if (f.open(QIODevice::ReadOnly))
        m_d->buffer = f.readAll();

    init();
}

ZipReader::ZipReader(QIODevice *device)
    : m_d(new ReaderData)
{
    if (device && device->isOpen())
        m_d->buffer = device->readAll();
    else if (device && device->open(QIODevice::ReadOnly)) {
        m_d->buffer = device->readAll();
    }

    init();
}

ZipReader::~ZipReader()
{
}

void ZipReader::init()
{
    if (m_d->buffer.isEmpty())
        return;

    mz_zip_zero_struct(&m_d->archive);
    if (!mz_zip_reader_init_mem(&m_d->archive, m_d->buffer.data(),
                                static_cast<size_t>(m_d->buffer.size()), 0)) {
        m_d->archiveInited = false;
        return;
    }
    m_d->archiveInited = true;

    const mz_uint count = mz_zip_reader_get_num_files(&m_d->archive);
    for (mz_uint i = 0; i < count; ++i) {
        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&m_d->archive, i, &stat))
            continue;
        if (stat.m_is_directory)
            continue;
        m_filePaths.append(QString::fromUtf8(stat.m_filename));
    }
}

bool ZipReader::exists() const
{
    return m_d->archiveInited;
}

QStringList ZipReader::filePaths() const
{
    return m_filePaths;
}

QByteArray ZipReader::fileData(const QString &fileName) const
{
    if (!m_d->archiveInited)
        return QByteArray();

    const int index = mz_zip_reader_find_file(&m_d->archive, fileName.toUtf8().constData());
    if (index < 0)
        return QByteArray();

    size_t uncompSize = 0;
    void *pData = mz_zip_reader_extract_to_heap(&m_d->archive,
                                                static_cast<mz_uint>(index),
                                                &uncompSize, 0);
    if (!pData)
        return QByteArray();

    QByteArray result(static_cast<int>(uncompSize), '\0');
    memcpy(result.data(), pData, uncompSize);
    free(pData);
    return result;
}

QT_END_NAMESPACE_XLSX
