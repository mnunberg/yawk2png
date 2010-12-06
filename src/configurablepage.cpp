#include "configurablepage.h"
#include "twutil.h"
#include <QWebPage>
#include <QTimer>

ConfigurablePage::ConfigurablePage(QObject *parent) :
    QWebPage(parent)
{
	uaString = QString("ALL YOUR BASE ARE BELONG TO US");
	globalTimeout = 0;
	connect(this, SIGNAL(loadStarted()), SLOT(_configureTimeout()));
	connect(this, SIGNAL(loadFinished(bool)), SLOT(_unsetTimeoutAction(bool)));
	isLoading = false;
}

QString ConfigurablePage::userAgentForUrl(const QUrl & url) const
{
	return uaString;
}

void ConfigurablePage::_configureTimeout()
{
	isLoading = true;
	if(globalTimeout) {
		QTimer::singleShot(globalTimeout, this, SLOT(_triggerTimeout()));
	}
}

void ConfigurablePage::_triggerTimeout() {
	if(isLoading) {
		twlog_crit("Load timed out after %d msecs, aborting load", globalTimeout);
		triggerAction(QWebPage::Stop, true);
	}
}
void ConfigurablePage::_unsetTimeoutAction(bool)
{
	isLoading = false;
}
