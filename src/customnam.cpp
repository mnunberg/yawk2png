#include "customnam.h"
#include "qnrwrapper.h"
#include "twutil.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QSslError>
#include <iostream>
CustomNAM::CustomNAM(QObject *parent) :
    QNetworkAccessManager(parent)
{
	ignoreSSLErrors = true;
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
	connect(this, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),
			SLOT(handleSslErrors(QNetworkReply*,QList<QSslError>)));
	return ret;
}

void CustomNAM::handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
	QListIterator<QSslError> i(errors);
	while(i.hasNext()) {
		if(!ignoreSSLErrors)
			twlog_warn("SSL Error: %s",qPrintable(((i.next()).errorString())));
		else
			twlog_debug("SSL Error: %s",qPrintable(((i.next()).errorString())));

	}
	if (ignoreSSLErrors) {
		twlog_debug("Ignoring SSL Errors as requested");
		reply->ignoreSslErrors();
	}
}
