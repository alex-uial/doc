#pragma once
#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QComboBox>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
private slots:
    void onGenerate();
    void browseXlsx();
    void browseHtml();
    void browsePdf();
    void browseTemplate();
private:
    QLineEdit *xlsxEdit;
    QLineEdit *templateEdit;
    QLineEdit *outHtmlEdit;
    QLineEdit *outPdfEdit;
    QComboBox *docTypeCombo;
    QTextEdit *log;
    QPushButton *genBtn;
};
