#include "MainWindow.h"
#include "XlsxReader.h"
#include "HtmlFiller.h"
#include "DesignatorParser.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QPrinter>
#include <QTextDocument>
#include <QLabel>
#include <QStandardPaths>
#include <QSet>
#include <QDir>
#include <QCoreApplication>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("DocGenerator — GOST documents from BOM");
    resize(860, 560);

    auto central = new QWidget;
    auto layout = new QVBoxLayout(central);

    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (defaultDir.isEmpty()) defaultDir = QDir::homePath();

    // Document type
    auto typeLay = new QHBoxLayout;
    docTypeCombo = new QComboBox;
    docTypeCombo->addItem("ПЭ3 (Перечень элементов)");
    docTypeCombo->addItem("ВП (Ведомость покупных)");
    docTypeCombo->addItem("Спецификация");
    typeLay->addWidget(new QLabel("Тип документа:"));
    typeLay->addWidget(docTypeCombo, 1);
    layout->addLayout(typeLay);

    // XLSX
    auto xlsxLay = new QHBoxLayout;
    xlsxEdit = new QLineEdit;
    auto xlsxBtn = new QPushButton("...");
    xlsxLay->addWidget(new QLabel("XLSX:"));
    xlsxLay->addWidget(xlsxEdit);
    xlsxLay->addWidget(xlsxBtn);
    layout->addLayout(xlsxLay);

    // Template
    auto tplLay = new QHBoxLayout;
    QString defaultTpl = QCoreApplication::applicationDirPath() + "/templates/spec_template.html";
    if (!QFile::exists(defaultTpl))
        defaultTpl = QCoreApplication::applicationDirPath() + "/../templates/spec_template.html";
    templateEdit = new QLineEdit(defaultTpl);
    auto tplBtn = new QPushButton("...");
    tplLay->addWidget(new QLabel("Шаблон:"));
    tplLay->addWidget(templateEdit);
    tplLay->addWidget(tplBtn);
    layout->addLayout(tplLay);

    // HTML output
    auto htmlLay = new QHBoxLayout;
    outHtmlEdit = new QLineEdit(defaultDir + "/spec_filled.html");
    auto htmlBtn = new QPushButton("...");
    htmlLay->addWidget(new QLabel("HTML:"));
    htmlLay->addWidget(outHtmlEdit);
    htmlLay->addWidget(htmlBtn);
    layout->addLayout(htmlLay);

    // PDF output
    auto pdfLay = new QHBoxLayout;
    outPdfEdit = new QLineEdit(defaultDir + "/spec_filled.pdf");
    auto pdfBtn = new QPushButton("...");
    pdfLay->addWidget(new QLabel("PDF:"));
    pdfLay->addWidget(outPdfEdit);
    pdfLay->addWidget(pdfBtn);
    layout->addLayout(pdfLay);

    genBtn = new QPushButton("Сгенерировать");
    layout->addWidget(genBtn);

    log = new QTextEdit;
    log->setReadOnly(true);
    layout->addWidget(log);

    setCentralWidget(central);

    connect(xlsxBtn, &QPushButton::clicked, this, &MainWindow::browseXlsx);
    connect(tplBtn, &QPushButton::clicked, this, &MainWindow::browseTemplate);
    connect(htmlBtn, &QPushButton::clicked, this, &MainWindow::browseHtml);
    connect(pdfBtn, &QPushButton::clicked, this, &MainWindow::browsePdf);
    connect(genBtn, &QPushButton::clicked, this, &MainWindow::onGenerate);
}

void MainWindow::browseXlsx() {
    auto p = QFileDialog::getOpenFileName(this, "Выберите XLSX", "", "Excel (*.xlsx)");
    if (!p.isEmpty()) xlsxEdit->setText(p);
}

void MainWindow::browseTemplate() {
    auto p = QFileDialog::getOpenFileName(this, "Шаблон HTML", templateEdit->text(), "HTML (*.html)");
    if (!p.isEmpty()) templateEdit->setText(p);
}

void MainWindow::browseHtml() {
    auto p = QFileDialog::getSaveFileName(this, "Сохранить HTML", outHtmlEdit->text(), "HTML (*.html)");
    if (!p.isEmpty()) outHtmlEdit->setText(p);
}

void MainWindow::browsePdf() {
    auto p = QFileDialog::getSaveFileName(this, "Сохранить PDF", outPdfEdit->text(), "PDF (*.pdf)");
    if (!p.isEmpty()) outPdfEdit->setText(p);
}

void MainWindow::onGenerate() {
    int docType = docTypeCombo->currentIndex();
    if (docType != 0) {
        log->append("Шаблон для «" + docTypeCombo->currentText() + "» пока не добавлен. Доступен только ПЭ3.");
        QMessageBox::information(this, "Информация", "Пока реализован только тип ПЭ3.\nОстальные шаблоны будут добавлены позже.");
        return;
    }

    QString xlsx = xlsxEdit->text();
    if (xlsx.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Выберите XLSX файл");
        return;
    }

    QString err;
    auto items = XlsxReader::read(xlsx, &err);
    if (items.isEmpty()) {
        log->append("Ошибка чтения XLSX: " + err);
        return;
    }
    log->append(QString("Загружено %1 строк").arg(items.size()));

    // Unique designators across the whole BOM (first column = Designator)
    QSet<QString> uniqueTokens;
    for (const BomItem &it : items)
        for (const QString &t : DesignatorParser::extractTokens(it.designator))
            uniqueTokens.insert(t);
    log->append(QString("Уникальных Designator: %1").arg(uniqueTokens.size()));

    // Check that numbering per prefix goes in order starting from C1/R1/...
    QString seqProblem;
    if (!DesignatorParser::checkSequentialFromOne(uniqueTokens.values(), &seqProblem))
        log->append("⚠ Нумерация не последовательна от 1: " + seqProblem);
    else
        log->append("Нумерация последовательна (каждый префикс от 1 без пропусков)");

    // Sort rows by designator: alphabetically by prefix, then numerically (C1,C2,...C10)
    std::sort(items.begin(), items.end(), [](const BomItem &a, const BomItem &b) {
        auto ka = DesignatorParser::sortKey(a.designator);
        auto kb = DesignatorParser::sortKey(b.designator);
        if (ka.first != kb.first) return ka.first < kb.first;
        if (ka.second != kb.second) return ka.second < kb.second;
        return a.designator < b.designator;
    });
    log->append("Отсортировано по Designator (по алфавиту, затем по номеру)");

    // Load template
    QString tplPath = templateEdit->text();
    QFile tf(tplPath);
    if (!tf.open(QIODevice::ReadOnly)) {
        log->append("Не удалось открыть шаблон: " + tplPath);
        QMessageBox::warning(this, "Ошибка", "Не найден шаблон HTML");
        return;
    }
    QString templateHtml = QString::fromUtf8(tf.readAll());
    tf.close();

    auto result = HtmlFiller::fill(templateHtml, items);
    for (const QString &w : result.warnings) {
        log->append("⚠ " + w);
    }
    log->append(QString("Листов в документе: %1 (вместимость продолжения: %2 строк/лист)")
                    .arg(result.pagesUsed).arg(result.capacityPerPage));

    QString htmlPath = outHtmlEdit->text();
    QString pdfPath = outPdfEdit->text();

    QFile f(htmlPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        f.write(result.html.toUtf8());
        f.close();
        log->append("HTML сохранён: " + htmlPath);
    } else {
        log->append("Не удалось записать HTML: " + htmlPath);
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(pdfPath);
    printer.setPageSize(QPageSize::A4);
    QTextDocument doc;
    doc.setHtml(result.html);
    doc.print(&printer);
    log->append("PDF сохранён: " + pdfPath);

    QMessageBox::information(this, "Готово",
        QString("Сгенерировано.\nСтрок: %1\nПредупреждений: %2\n\n%3\n%4")
            .arg(items.size()).arg(result.warnings.size()).arg(htmlPath, pdfPath));
}
