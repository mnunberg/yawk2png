#include "qnrwrapper.h"
#include "twutil.h"
#include <QNetworkReply>
QNRWrapper::QNRWrapper(QNetworkReply *reply, int msecs) :
	QObject()
{
	this->reply = reply;
	timer.setInterval(msecs);
	timer.setSingleShot(true);
	connect(&timer, SIGNAL(timeout()), SLOT(abortRequest()));
	connect(reply, SIGNAL(destroyed()), SLOT(connTimerStop()));
	timer.start();
}

void QNRWrapper::abortRequest()
{
	twlog_warn("Aborting request for URL %s", qPrintable(reply->url().toString()));
	reply->abort();
}

void QNRWrapper::connTimerStop()
{
	twlog_debug("aborting timeout monitor because reply has been deleted");
	timer.stop();
	deleteLater();
}
