#ifndef WEBKITRENDERER_H
#define WEBKITRENDERER_H

#include <QObject>
#include <QImage>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include "configurablepage.h"

struct http_errinfo {
	QString url;
	int errcode;
};

class WebkitRenderer : public QObject
{
	Q_OBJECT;
public:
	WebkitRenderer(QNetworkRequest, QNetworkAccessManager*);
	QUrl currentUrl; /*URL that we may have been redirected to*/
	QUrl baseUrl; /*base url, used to resolve relative paths*/
	QImage image;
	ConfigurablePage page; /*Needed to subclass QWebPage to getcustomizable user agent*/
	QNetworkRequest req;
	QNetworkAccessManager *qnam;
	bool load();
	void render();
	enum fetchResult {
		OK,
		PossibleHttpError,
		PageLoadError
	};
	WebkitRenderer::fetchResult resultCode;

public slots:
	void loadProgress(int);
	void loadFinished(bool);
	void qnamFinished(QNetworkReply*);
signals:
	void done();
private:
	QList<http_errinfo> errors;
};

#endif // WEBKITRENDERER_H
