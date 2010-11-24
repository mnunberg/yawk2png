#include "customnam.h"
#include "qnrwrapper.h"
#include "twutil.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>

#include <iostream>
CustomNAM::CustomNAM(QObject *parent) :
    QNetworkAccessManager(parent)
{
}
QNetworkReply *CustomNAM::createRequest(
		Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
//	twlog_warn("Hello!");
//	QList<QByteArray>::iterator i;
//	QList<QByteArray> headerlist = request.rawHeaderList();
//	for(i=headerlist.begin(); i != headerlist.end(); ++i) {
////		std::cerr << ".";
//		qDebug("%s: %s", (*i).constData(), request.rawHeader(*i).constData());
//	}
	QNetworkReply *ret = QNetworkAccessManager::createRequest(op, request, outgoingData);
	if(connTimeout) {
		new QNRWrapper(ret, connTimeout);
	}
	return ret;
}
