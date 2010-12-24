#include "configurablepage.h"
#include "twutil.h"
#include "grabber.h"
#include <QWebPage>
#include <QTimer>
#include <time.h>
#include <unistd.h>
#include <iostream>

ConfigurablePage::ConfigurablePage(QObject *parent) :
    QWebPage(parent)
{
	uaString = QString("ALL YOUR BASE ARE BELONG TO US");
	globalTimeout = 0;
	connect(this, SIGNAL(loadStarted()), SLOT(_configureTimeout()));
	connect(this, SIGNAL(loadFinished(bool)), SLOT(_unsetTimeoutAction(bool)));
	connect(this, SIGNAL(loadProgress(int)), SLOT(setProgress(int)));
	isLoading = false;
}

QString ConfigurablePage::userAgentForUrl(const QUrl&) const
{
	return uaString;
}

void ConfigurablePage::_configureTimeout()
{
	startTime = time(NULL)-1; /*no FPE*/
	isLoading = true;
	if(globalTimeout) {
		QTimer::singleShot(globalTimeout, this, SLOT(_triggerTimeout()));
	}
	progress = 0;
	if(isatty(STDERR_FILENO)) {
		connect(&progressUpdateTimer, SIGNAL(timeout()), SLOT(showProgress()));
		progressUpdateTimer.start(500);
	}
}

void ConfigurablePage::_triggerTimeout() {
	if(isLoading) {
		twlog_crit(GRABBER_STAGE_NETWORK " "
				   GRABBER_DEBUG_TIMEOUT_ERROR " global timeout expired");
		triggerAction(QWebPage::Stop, true);
	}
}
void ConfigurablePage::_unsetTimeoutAction(bool)
{
	isLoading = false;
	progressUpdateTimer.stop();
}

void ConfigurablePage::setProgress(int progress) {
	this->progress = progress;
}

void ConfigurablePage::showProgress()
{
	std::cerr << qPrintable(QString("").sprintf("%d%% %d KB/s", progress,
					((bytesReceived() / (time(NULL) - startTime))) / 1024
					).leftJustified(15, ' ')) << "\r";
}
