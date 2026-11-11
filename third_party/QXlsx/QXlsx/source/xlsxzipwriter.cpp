// xlsxzipwriter.cpp — ZIP writer implemented on top of miniz (public domain).
// Replaces the old Qt-private QZipWriter usage so QXlsx builds with any Qt kit.

#include "xlsxzipwriter_p.h"

#include <QFile>

#include "../3rdparty/miniz.h"

QT_BEGIN_NAMESPACE_XLSX

namespace {

void appendLE16(QByteArray &a, quint16 v)
{
    a.append(char(v & 0xFF));
    a.append(char((v >> 8) & 0xFF));
}

void appendLE32(QByteArray &a, quint32 v)
{
    a.append(char(v & 0xFF));
    a.append(char((v >> 8) & 0xFF));
    a.append(char((v >> 16) & 0xFF));
    a.append(char((v >> 24) & 0xFF));
}

quint32 crc32_of(const QByteArray &data)
{
    return mz_crc32(MZ_CRC32_INIT,
                    reinterpret_cast<const unsigned char *>(data.constData()),
                    size_t(data.size()));
}

} // namespace

ZipWriter::ZipWriter(const QString &filePath)
{
    m_file = new QFile(filePath);
    if (m_file->open(QIODevice::WriteOnly)) {
        m_device = m_file;
        m_ownsDevice = true;
        m_error = false;
    } else {
        m_error = true;
    }
}

ZipWriter::ZipWriter(QIODevice *device)
    : m_device(device)
{
    m_error = !(device && device->isWritable());
}

ZipWriter::~ZipWriter()
{
    close();
    delete m_file;
}

bool ZipWriter::error() const
{
    return m_error || (m_device && m_device->error() != QIODevice::NoError);
}

void ZipWriter::addFile(const QString &filePath, QIODevice *device)
{
    addFile(filePath, device ? device->readAll() : QByteArray());
}

void ZipWriter::addFile(const QString &filePath, const QByteArray &data)
{
    if (error() || m_finished)
        return;

    const QByteArray name = filePath.toUtf8();

    // Compress with raw deflate (method 8); fall back to stored if compression doesn't pay off.
    QByteArray comp;
    quint16 method = 0;
    {
        const size_t bound = mz_deflateBound(nullptr, size_t(data.size())) + 64;
        comp.resize(int(bound));
        tdefl_compressor cmpr;
        if (MZ_SUCCESS == tdefl_create_comp_flags_from_zip_params(6, -15, MZ_DEFAULT_STRATEGY, &cmpr)) {
            int status = tdefl_compress_mem_to_output(
                &cmpr,
                reinterpret_cast<const unsigned char *>(data.constData()), size_t(data.size()),
                [](const void *buf, size_t size, void *pOpaque) -> mz_bool {
                    auto *out = static_cast<QByteArray *>(pOpaque);
                    out->append(static_cast<const char *>(buf), int(size));
                    return MZ_TRUE;
                },
                &comp);
            if (status == MZ_STREAM_END && !comp.isEmpty() && comp.size() < data.size()) {
                comp.resize(int(comp.size()));
                method = 8;
            } else {
                comp = data; // stored
                method = 0;
            }
        } else {
            comp = data;
            method = 0;
        }
    }

    const quint32 crc = crc32_of(data);

    // ---- Local file header ----
    QByteArray lfh;
    appendLE32(lfh, 0x04034b50); // signature
    appendLE16(lfh, 20);         // version needed
    appendLE16(lfh, 0);          // flags (no data descriptor; sizes known up-front)
    appendLE16(lfh, method);
    appendLE16(lfh, 0);          // dos time
    appendLE16(lfh, 0x21);       // dos date (1980-01-01)
    appendLE32(lfh, crc);
    appendLE32(lfh, quint32(comp.size()));
    appendLE32(lfh, quint32(data.size()));
    appendLE16(lfh, quint16(name.size()));
    appendLE16(lfh, 0);          // extra len
    lfh.append(name);

    m_entries.append(lfh);
    m_entries.append(comp);

    const quint32 localHeaderOffset = m_offset;
    m_offset += quint32(lfh.size() + comp.size());
    ++m_numEntries;

    // ---- Central directory record ----
    QByteArray cdr;
    appendLE32(cdr, 0x02014b50); // signature
    appendLE16(cdr, 20);         // version made by
    appendLE16(cdr, 20);         // version needed
    appendLE16(cdr, 0);          // flags
    appendLE16(cdr, method);
    appendLE16(cdr, 0);          // dos time
    appendLE16(cdr, 0x21);       // dos date
    appendLE32(cdr, crc);
    appendLE32(cdr, quint32(comp.size()));
    appendLE32(cdr, quint32(data.size()));
    appendLE16(cdr, quint16(name.size()));
    appendLE16(cdr, 0);          // extra len
    appendLE16(cdr, 0);          // comment len
    appendLE16(cdr, 0);          // disk number start
    appendLE16(cdr, 0);          // internal attrs
    appendLE32(cdr, 0);          // external attrs
    appendLE32(cdr, localHeaderOffset);
    cdr.append(name);

    m_central.append(cdr);
}

void ZipWriter::close()
{
    finish();
}

void ZipWriter::finish()
{
    if (m_finished || !m_device || m_error)
        return;
    m_finished = true;

    const quint32 cdOffset = m_offset;
    const quint32 cdSize = quint32(m_central.size());

    QByteArray eocd;
    appendLE32(eocd, 0x06054b50); // EOCD signature
    appendLE16(eocd, 0);          // disk num
    appendLE16(eocd, 0);          // disk with CD
    appendLE16(eocd, m_numEntries);
    appendLE16(eocd, m_numEntries);
    appendLE32(eocd, cdSize);
    appendLE32(eocd, cdOffset);
    appendLE16(eocd, 0);          // comment len

    m_device->write(m_entries);
    m_device->write(m_central);
    m_device->write(eocd);
    m_device->flush();

    if (m_ownsDevice)
        m_device->close();
}

QT_END_NAMESPACE_XLSX
