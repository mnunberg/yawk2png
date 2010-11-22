#ifndef WEBKITRENDERER_H
#define WEBKITRENDERER_H

#include <QObject>
#include <QWebPage>
#include <QImage>
#include <QNetworkRequest>

struct http_errinfo {
	QString url;
	int errcode;
};

class WebkitRenderer : public QObject
{
	Q_OBJECT;
public:
	WebkitRenderer(QNetworkRequest);
	QUrl currentUrl; /*URL that we may have been redirected to*/
	QImage image;
	QWebPage page;
	QNetworkRequest req;
	void load();
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
