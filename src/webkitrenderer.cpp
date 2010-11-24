#include "webkitrenderer.h"
#include <QWebFrame>
#include <QtWebKit>
#include <QList>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QPainter>
#include <iostream>
#include "configurablepage.h"
#include "twutil.h"
WebkitRenderer::WebkitRenderer(QNetworkRequest req, QNetworkAccessManager *qnam): QObject()
{
	currentUrl = req.url();
	this->req = req;
	this->qnam = qnam;
	if(!req.rawHeader("User-Agent").isNull()) {
		page.uaString = QString(req.rawHeader("User-Agent"));
	}
	page.setNetworkAccessManager(qnam);

	twlog_debug("current URL is %s", qPrintable(currentUrl.toString()));

	connect((&page)->networkAccessManager(), SIGNAL(finished(QNetworkReply*)),
			SLOT(qnamFinished(QNetworkReply*)));
	connect(&page, SIGNAL(loadFinished(bool)), SLOT(loadFinished(bool)));
	connect(&page, SIGNAL(loadProgress(int)), SLOT(loadProgress(int)));

	/*initialize style*/
	page.mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
	page.mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
	page.setViewportSize(QSize(1024, 768));
}

bool WebkitRenderer::load()
{
	/*maybe we can put more here?*/
	if (req.url().toString().startsWith("file://")) {
		twlog_warn("Using baseurl %s", qPrintable(baseUrl.toString()));
		QFile file(req.url().toLocalFile());
		if(!file.open(QIODevice::ReadOnly)) {
			twlog_crit("Error opening file %s: %s",
					   qPrintable(file.fileName()),
					   qPrintable(file.errorString()));
			return false;
		}
		QByteArray fdata = file.readAll();

		file.close();
		if(fdata.isNull()) {
			twlog_crit("data from %s is empty", qPrintable(file.fileName()));
			file.close();
			return false;
		}
		file.close();
		page.mainFrame()->setContent(fdata, "text/html", baseUrl);
		twlog_debug("frame is currently displaying %d bytes", page.mainFrame()->toHtml().length());

		return true;
	} else {
		page.mainFrame()->load(req);
		return true;
	}
	/*Load the request, and connect the handler...*/
}

void WebkitRenderer::render()
{
	QSize size = page.mainFrame()->contentsSize();
	image = QImage(size, QImage::Format_ARGB32_Premultiplied);
	image.fill(Qt::transparent);
	QPainter p(&image);

	p.setRenderHint(QPainter::Antialiasing, true);
	p.setRenderHint(QPainter::TextAntialiasing, true);
	p.setRenderHint(QPainter::SmoothPixmapTransform, true);

	page.setViewportSize(page.mainFrame()->contentsSize());
//	page.mainFrame()->render(&p, QWebFrame::ContentsLayer);
	page.mainFrame()->render(&p);
}

void WebkitRenderer::loadFinished(bool status)
{
	if(!status) {
		qCritical("Something went wrong!");
	}
	if (!status) {
		resultCode = PageLoadError;
	} else {
		if(!errors.isEmpty()) {
			resultCode = PossibleHttpError;
		} else {
			resultCode = OK;
		}
	}
	if(resultCode != PageLoadError) {
		render();
	} else {
		qCritical("PageLoadError!");
	}
	emit done();
}

void WebkitRenderer::loadProgress(int progress)
{
	std::cerr << progress << "%\r";
}

void WebkitRenderer::qnamFinished(QNetworkReply *reply)
{
	/*This tries to detect some errors with a bit of brains. We keep track of
	  what our 'real' URL is, by changing our currentUrl to whatever we are
	  redirected to*/
	if(reply->error()) {
		twlog_crit("error for %s: %s",
				   qPrintable(reply->url().toString()),
				   qPrintable(reply->errorString()));
	}
	if(reply->url().toEncoded(QUrl::StripTrailingSlash) !=
	   currentUrl.toEncoded(QUrl::StripTrailingSlash)) {
//		qDebug("Not operating on URL %s (our URL is %s)",
//			   qPrintable(reply->url().toString()),
//			   qPrintable(currentUrl.toString()));
		return;
	}
	int statuscode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if(statuscode >= 400 && statuscode < 600) {
		qDebug("%d for %s", statuscode, qPrintable(reply->url().toString()));
		/*http error*/
		struct http_errinfo errinfo;
		errinfo.errcode = statuscode;
		errinfo.url = reply->url().toString();
		errors << errinfo;
	} else if (statuscode >= 300 && statuscode < 400) {
		/*Redirect*/
//		qDebug("Redirect!");
		QVariant v = reply->header(QNetworkRequest::LocationHeader);
		if (!v.isValid()) {
			qCritical("Got 3xx response without Location!");
			return;
		}
		QUrl location = v.toUrl();
		currentUrl =
				(location.isRelative()) ? (currentUrl.resolved(location)) : location;
	}
}
