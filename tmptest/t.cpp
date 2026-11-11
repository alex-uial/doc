#include "HtmlFiller.h"
#include "DesignatorParser.h"
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
int main(int argc, char** argv){
    QCoreApplication app(argc, argv);
    QFile f("../templates/spec_template.html");
    if(!f.open(QIODevice::ReadOnly)){ qCritical() << "no template"; return 1; }
    QString html = QString::fromUtf8(f.readAll());
    QVector<BomItem> items;
    // simulate: unique designators sorted, with ranges + long names to test fit warnings
    auto add=[&](QString d,QString t,QString c,QString m,QString q){
        BomItem it; it.designator=d; it.type=t; it.comment=c; it.manufacturer=m; it.quantity=q; items<<it; };
    add("C1; C2; C3; C4","Конденсатор","КМ-6 10нФ 16В","Электронмаш","4");
    add("R1","Резистор","С2-23 1кОм 0.25Вт","Плоское","1");
    add("D1; D2","Диод","2A7"," ","2");
    for(int i=1;i<=40;i++){
        BomItem it; it.designator=QString("DA%1").arg(i); it.type="Микросхема";
        it.comment="KR140УД7Б  15.11.120.001  ТУАБВС.30412.001"; it.manufacturer="АО «Интеграл», г. Минск";
        it.quantity="1"; items<<it;
    }
    FillResult r = HtmlFiller::fill(html, items);
    qDebug() << "pagesUsed:" << r.pagesUsed << "capacityPerPage:" << r.capacityPerPage;
    for(const QString&w:r.warnings) qDebug() << "WARN:" << w.left(120);
    QFile out("out.html"); out.open(QIODevice::WriteOnly); out.write(r.html.toUtf8()); out.close();
    return 0;
}
