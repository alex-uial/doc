#include <QCoreApplication>
#include <QFile>
#include <QDomDocument>
#include <QDebug>
#include <QRegularExpression>
int main(int argc,char**argv){ QCoreApplication a(argc,argv);
 QFile f("../templates/spec_template.html"); f.open(QIODevice::ReadOnly);
 QString html=QString::fromUtf8(f.readAll());
 for(const QString &tag : {QString("meta"),QString("br"),QString("link"),QString("img"),QString("hr"),QString("input")}){
   QRegularExpression re("<"+tag+"\\b[^>]*/>");
   auto it=re.globalMatch(html); int n=0; int line=-1;
   while(it.hasNext()){ auto m=it.next(); n++; if(line<0){ line=html.left(m.capturedStart()).count('\n')+1; } }
   qDebug()<<tag<<": self-closing count="<<n<<"first at line"<<line;
 }
 // try parse after removing /> on void tags
 QString h2=html;
 h2.replace(QRegularExpression("<(meta|br|link|img|hr|input)\\b([^>]*?)\\s*/>"), "<\\1\\2>");
 QDomDocument d; QString err; int ln=0,cl=0;
 bool ok=d.setContent(h2,false,&err,&ln,&cl);
 qDebug()<<"after fix ok="<<ok<<err<<ln<<cl;
 return 0;
}
