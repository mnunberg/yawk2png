#ifndef CUSTOMNAM_H
#define CUSTOMNAM_H

#include <QNetworkAccessManager>

class CustomNAM : public QNetworkAccessManager
{
Q_OBJECT
public:
    explicit CustomNAM(QObject *parent = 0);
	virtual QNetworkReply*
			createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData);
signals:

public slots:

};

#endif // CUSTOMNAM_H
