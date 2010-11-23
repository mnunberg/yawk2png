#include "configurablepage.h"
#include <QWebPage>

ConfigurablePage::ConfigurablePage(QObject *parent) :
    QWebPage(parent)
{
	uaString = QString("ALL YOUR BASE ARE BELONG TO US");
}

QString ConfigurablePage::userAgentForUrl(const QUrl & url) const
{
	return uaString;
}
