#ifndef CUSTOMNAM_H
#define CUSTOMNAM_H

#include <QNetworkAccessManager>
#include <QHash>
#include <QList>
#include <QSslError>
#include <QNetworkReply>

class CustomNAM : public QNetworkAccessManager
{
Q_OBJECT
public:
	int connTimeout; /*How long we can give each connection*/
	QHash<QString,QString> *headers; /*Some headers that we will use*/
	bool ignoreSSLErrors;
    explicit CustomNAM(QObject *parent = 0);
	virtual QNetworkReply*
			createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData);
	
signals:
public slots:
	void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);


};

#endif // CUSTOMNAM_H
