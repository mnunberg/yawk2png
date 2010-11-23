#include <QApplication>
#include <QFile>
#include <QHash>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QNetworkAccessManager>
#include <QRegExp>

#include "webkitrenderer.h"
#include "twutil.h"

#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>

const char USAGE_STRING[] =
			"-o --outfile FILE\n"
			"-u --url URL\n"
			"-P --proxy[=http[s]://PROXY:PORT] (note the '=')\n"
			"-R --referrer REFERRER\n"
			"-H --with-header HEADER_FIELD:HEADER_VALUE\n"
			"-S --stdin-header indicate header is being piped to stdin\n";

const struct option _longopts[] = {
	{"outfile",			required_argument,	NULL, 'o'},
	{"url",				required_argument,	NULL, 'u'},
	{"debug",			optional_argument,	NULL, 'd'},
	{"proxy",			optional_argument,	NULL, 'P'},
	{"with-header",		required_argument,	NULL, 'H'},
	{"stdin-header",	no_argument,		NULL, 'S'},
	{"help",			no_argument,		NULL, 'h'},
	{NULL, 0, NULL, NULL}
};
const char *optstring = "o:u:d::P::H:Sh?";


class CLIOpts {
	/*Class to control, parse, and apply user specified options*/
public:
	CLIOpts() { headers = new QHash<QString,QString>; };
	~CLIOpts() { delete headers; };
	/*value fields*/
	char * outfile;
	QString url;
	QString proxy_host;
	quint16 proxy_port;
	bool stdin_header;
	bool daemonize;
	int debug;
	QHash<QString,QString>* headers;

	/*helper methods*/
	int header_append(QString text) {
		if(text.isNull())
			return 0;
		QRegExp regex("^([^:]+):(.*)$");
		if((regex.indexIn(text) == -1) ||
		   regex.captureCount() != 2)
			return 0;
		headers->insert(regex.capturedTexts()[1].trimmed(),
						regex.capturedTexts()[2].trimmed());
		return 1;
	}

	int set_proxy(QString text) {
		QRegExp regex("^https?://([^:]+):(\\d+)$");
		if((regex.indexIn(text) == -1) ||
		   /*not needed, because the first will always fail*/
		   (regex.captureCount() != 2))
			return 0;
		/*Capture objects generally have the entire match as their first element*/
		proxy_host = regex.capturedTexts()[1];
		proxy_port = regex.capturedTexts()[2].toInt();
		/*finally, succeed*/
		return 1;
	}
	int parse_options(int argc, char**argv) {
		int _optindex;
		char opt = getopt_long(argc, argv, optstring, _longopts, &_optindex);
		while(opt != -1) {
			switch (opt) {
			case 'o': outfile = optarg; break;
			case 'u': url = optarg; break;
			/*Debug.. not implemented yet*/
			case 'd': debug = (optarg) ? atoi(optarg) : 1; break;
			case 'H':
				if(header_append(QString(optarg)))
					break;
			case 'S': {
				QTextStream stream(stdin);
				QString line;
				bool success = true;
				line = stream.readLine();
				if(line.isNull() || (!header_append(line)))
					success = false;
				else {
					line = stream.readLine();
					while(!line.isNull()) {
						if(!header_append(line)) {
							break;
							success = false;
						}
						line = stream.readLine();
					}
				}
				if(success)
					break;
				else
					qCritical("Bad header on input!");
			}
			case 'P': {
				/*Proxy*/
				bool success = false;
				do {
					/*funky control structure so we can break without breaking, yo dawg*/
					QString s;
					if(optarg) {
						s = QString(optarg);
					} else if (getenv("http_proxy")) {
						s = QString(getenv("http_proxy"));
					} else if (getenv("https_proxy")) {
						s = QString(getenv("https_proxy"));
					} else {
						qCritical("No proxy specified and nothing found in environment");
						break;
					}
					if (set_proxy(s)) {
						success = true;
					}
				} while (false);
				if(success) {
					break;
				} /*fall through otherwise*/
			}
			case 'h':
			case '?':
			default:
				return 0;
			}
			opt = getopt_long(argc, argv, optstring, _longopts, &_optindex);
		}
		return postprocess();
	}
private:
	int postprocess()
	{
		int success = 1;
		/*some postprocessing here. This configures our options a bit more,
		  using information acquired from all arguments*/
		if ((!url.isNull()) && url.startsWith("https")) {
			if(getenv("https_proxy")) {
				QString s(getenv("https_proxy"));
				if(!set_proxy(s))
					success = 0;
			}
		}
		return success;
	}
};

static QFile outfile;
static CLIOpts cliopts;


static WebkitRenderer *gen_renderer() {
	/*generates a WebkitRenderer object based on the command line options..*/
	QNetworkRequest req;
	QNetworkAccessManager *qnam = new QNetworkAccessManager();
	if(!cliopts.url.isNull())
		req.setUrl(QUrl(cliopts.url));
	if(!cliopts.proxy_host.isNull()) {
		QNetworkProxy proxy(QNetworkProxy::HttpProxy, cliopts.proxy_host, cliopts.proxy_port);
		qDebug("%s: PROXY: %s:%d", __PRETTY_FUNCTION__,
			   qPrintable(cliopts.proxy_host), cliopts.proxy_port);
		qnam->setProxy(proxy);
	}
	/*Header overrides*/
	QHashIterator<QString,QString> i(*(cliopts.headers));
	while(i.hasNext()) {
		i.next();
		qDebug("%s: %s: %s",  __PRETTY_FUNCTION__,
			   qPrintable(i.key()), qPrintable(i.value()));
		req.setRawHeader(i.key().toLocal8Bit(), i.value().toLocal8Bit());
	}
	WebkitRenderer *r = new WebkitRenderer(req, qnam);
	return r;
}

/*ugly C++ crap*/
class _SaveImage : public QObject
{
	Q_OBJECT
public:
	_SaveImage() : QObject() {}
public slots:
	void save_image() {
		qDebug("Hello!");
		/*Signal Handler*/
		WebkitRenderer *r = qobject_cast<WebkitRenderer*>(QObject::sender());
		if(!r) {
			qCritical("Couldn't get WebkitRenderer object");
			exit(1);
		}
		if(!r->image.isNull()) {
			if(!r->image.save(&outfile, "png")) {
				qCritical("Problem saving %s: %s",
						  qPrintable(outfile.fileName()),
						  qPrintable(outfile.errorString()));
				exit(1);
			}
		} else {
			qCritical("PageLoadError!");
			QApplication::exit(1);
		}
		if(r->resultCode != WebkitRenderer::OK) {
			qWarning("Got image, but result code was %d",
					 r->resultCode);
			exit(1);
		}
		exit(0);
	}
};
#include "main.moc"
/*end*/


static void debuglogger(QtMsgType type, const char *msg)
{
#define pretty_prefix(mtype) \
	if(type == mtype) { \
	std::cerr << #mtype << ": " << msg << "\n"; \
			return; \
	}
	if(cliopts.debug)
		pretty_prefix(QtDebugMsg);
	pretty_prefix(QtWarningMsg);
	pretty_prefix(QtCriticalMsg);
	pretty_prefix(QtFatalMsg);
#undef pretty_prefix
}

int main(int argc, char **argv)
{
	qInstallMsgHandler(debuglogger);
	QApplication a(argc, argv);
	if((!cliopts.parse_options(argc, argv)) ||
	   !(cliopts.url.size() && cliopts.outfile)) {
		std::cerr << USAGE_STRING;
		exit(1);
	}
	if (strcmp(cliopts.outfile, "-") == 0) {
		outfile.open(1, QIODevice::WriteOnly);
		qDebug("using stdout");
	} else {
		outfile.setFileName(cliopts.outfile);
		outfile.open(QIODevice::WriteOnly);
	}
	if (outfile.error()) {
		qCritical("%s: %s", qPrintable(outfile.fileName()),
				  qPrintable(outfile.errorString()));
		exit(1);
	}

	WebkitRenderer *r = gen_renderer();

	r->load();
	_SaveImage _si;

	QObject::connect(r, SIGNAL(done()), &_si, SLOT(save_image()));
	return a.exec();
}
