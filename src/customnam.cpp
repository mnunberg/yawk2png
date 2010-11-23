#include "customnam.h"
#include "twutil.h"
#include <QNetworkRequest>
CustomNAM::CustomNAM(QObject *parent) :
    QNetworkAccessManager(parent)
{
}
QNetworkReply *CustomNAM::createRequest(
		Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
//	twlog_warn("Hello!");
//	QList<QByteArray>::iterator i;
//	for(i=request.rawHeaderList().begin(); i != request.rawHeaderList().end(); i++)
//	{
//	}
	return QNetworkAccessManager::createRequest(op, request, outgoingData);
}
