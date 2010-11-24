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
	/*We need to create a new request since the one we were passed is marked as const*/
	QNetworkRequest newreq(request);
	QHashIterator<QString,QString> i(*(headers));
	while(i.hasNext()) {
		i.next();
		twlog_debug("%s: %s", qPrintable(i.key()), qPrintable(i.value()));
		newreq.setRawHeader(i.key().toLocal8Bit(), i.value().toLocal8Bit());
	}

	QNetworkReply *ret = QNetworkAccessManager::createRequest(op, newreq, outgoingData);
	if(connTimeout) {
		new QNRWrapper(ret, connTimeout);
	}
	return ret;
}
