#ifndef QNRWRAPPER_H
#define QNRWRAPPER_H

#include <QObject>
#include <QNetworkReply>
#include <QTimer>
class QNRWrapper : public QObject
{
Q_OBJECT
public:
	explicit QNRWrapper(QNetworkReply*, int msecs);
	QNetworkReply *reply;
signals:

public slots:
	void abortRequest();
	void connTimerStop();
private:
	QTimer timer;
};

#endif // QNRWRAPPER_H
