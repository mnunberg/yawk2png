#ifndef CUSTOMNAM_H
#define CUSTOMNAM_H

#include <QNetworkAccessManager>
#include <QHash>

class CustomNAM : public QNetworkAccessManager
{
Q_OBJECT
public:
	int connTimeout; /*How long we can give each connection*/
	QHash<QString,QString> *headers; /*Some headers that we will use*/
    explicit CustomNAM(QObject *parent = 0);
	virtual QNetworkReply*
			createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData);
signals:

public slots:

};

#endif // CUSTOMNAM_H
