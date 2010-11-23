#ifndef CONFIGURABLEPAGE_H
#define CONFIGURABLEPAGE_H

#include <QWebPage>

class ConfigurablePage : public QWebPage
{
Q_OBJECT
public:
    explicit ConfigurablePage(QObject *parent = 0);
	QString uaString;
	virtual QString userAgentForUrl(const QUrl & url) const;
signals:

public slots:

};

#endif // CONFIGURABLEPAGE_H
