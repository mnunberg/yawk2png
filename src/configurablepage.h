#ifndef CONFIGURABLEPAGE_H
#define CONFIGURABLEPAGE_H

#include <QWebPage>
#include <time.h>
#include <QTimer>

class ConfigurablePage : public QWebPage
{
Q_OBJECT
public:
    explicit ConfigurablePage(QObject *parent = 0);
	QString uaString;
	virtual QString userAgentForUrl(const QUrl & url) const;
	int globalTimeout;
	bool isLoading;
	time_t startTime;
private:
	int progress;
	QTimer progressUpdateTimer;
signals:

public slots:
	void _configureTimeout();
	void _triggerTimeout();
	void _unsetTimeoutAction(bool);
	void showProgress();

private slots:
	void setProgress(int);
};

#endif // CONFIGURABLEPAGE_H
