#ifndef CONFIGURABLEPAGE_H
#define CONFIGURABLEPAGE_H

#include <QWebPage>
#include <time.h>
#include <QTimer>

struct jsopts {
    bool alertDisable;
    bool promptAutoRespond;
    QString promptResponse;
    bool confirmAutoRespond;
    bool confirmResponse;
};

class ConfigurablePage : public QWebPage
{
Q_OBJECT
public:
    explicit ConfigurablePage(QObject *parent = 0);
	QString uaString;
	virtual QString userAgentForUrl(const QUrl & url) const;
	int globalTimeout;
	bool isLoading;

	struct jsopts jsOpts;

	virtual void javaScriptAlert(QWebFrame *originatingFrame, const QString &msg);
	virtual bool javaScriptPrompt(QWebFrame *originatingFrame, const QString &msg, const QString &defaultValue, QString *result);
	virtual bool javaScriptConfirm(QWebFrame *originatingFrame, const QString &msg);

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
