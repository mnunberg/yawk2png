#include <QApplication>
#include <QFile>
#include <QHash>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QNetworkAccessManager>
#include <QRegExp>

#include "webkitrenderer.h"
#include "customnam.h"
#include "twutil.h"

#include <argp.h>

#include <string.h>
#include <stdlib.h>
#include <iostream>

#define BAD_ARGUMENT 4
#define MISSING_REQUIRED_ARGUMENTS 5
#define OPTION_OK 0

#define VERSION_STRING "11-24-2010.3-CURRENT"

static const char ABOUT[] =
		"qtgrabber v" VERSION_STRING "\n"
		"qtgrabber is a webpage capturing utility that is somewhat tolerant "
		"of errors and is able to provide notification about them. It supports "
		"piping a complete HTTP header on stdin, or specifying individual "
		"header values using the -H option";


struct argp_option opt_table[] = {
/* basic options*/
{"outfile",		'o',	"FILE",		0,  "output file, or - for standard output", 0},
{"url",			'u',	"URL",		0,  "url to fetch, use file:///abspath for local files", 0},
{"baseurl",		'b',	"URL",		0,  "base url to use for relative references in local files", 0},
/*connection/request options*/
{"proxy",		'P',	"http[s]://PROXY:PORT", OPTION_ARG_OPTIONAL, "Use a proxy. If a"
	 "proxy is not provided on the command line, it will use an http*_proxy "
	 "environment variable", 1},
{"with-header", 'H',	"HEADER:VALUE", 0, "add a header field to the request", 1},
{"stdin-header", 'S',	NULL,		0,	"indicate that a header is being piped on stdin", 1},
{"per-connection-timeout", 'T', "msecs", 0, "timeout for *each entity request*", 1},
/*miscelanny*/
{"debug",		'd',	"LEVEL", OPTION_ARG_OPTIONAL, "debug level", 2},
{"version",		'V',	NULL,	0,	"print version and exit", 2},
{NULL,NULL,NULL,NULL,NULL,NULL}
};

class CLIOpts {
	/*Class to control, parse, and apply user specified options*/
public:
	CLIOpts() { headers = new QHash<QString,QString>; };
	~CLIOpts() { delete headers; };
	/*value fields*/
	char * outfile;
	QString url;
	QString baseurl;
	bool use_proxy;
	QString proxy_host;
	quint16 proxy_port;
	bool stdin_header;
	int debug;
	int connection_timeout;
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
		   (regex.captureCount() != 2)) {
			twlog_warn("Problem parsing proxy!");
			return BAD_ARGUMENT;
			}
		/*Capture objects generally have the entire match as their first element*/
		proxy_host = regex.capturedTexts()[1];
		proxy_port = regex.capturedTexts()[2].toInt();
		twlog_warn("%s:%d", qPrintable(proxy_host), proxy_port);
		/*finally, succeed*/
		return 1;
	}	
	error_t parse_opt(int key, char *arg, struct argp_state *state) {
		switch (key) {
		case 'o': outfile = arg; break;
		case 'u': url = arg; break;
		case 'b': baseurl = arg; break;
		/*Debug.. not implemented yet*/
		case 'd': debug = (arg) ? atoi(arg) : 1; break;
		case 'H':
			if(header_append(QString(arg)))
				break;
			else {
				twlog_crit("Couldn't parse header '%s'", arg);
				return BAD_ARGUMENT;
			}
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
			else {
				twlog_crit("Bad header on input!");
					return BAD_ARGUMENT;
			}
		}
		case 'P': {
			/*Proxy*/
			use_proxy = true;
			bool success = false;
			do {
				/*funky control structure so we can break without breaking, yo dawg*/
				QString s;
				if(arg) {
					s = QString(arg);
				} else if (getenv("http_proxy")) {
					s = QString(getenv("http_proxy"));
				} else if (getenv("https_proxy")) {
					s = QString(getenv("https_proxy"));
				} else {
					twlog_crit("No proxy specified and nothing found in environment");
					break;
				}
				if (set_proxy(s)) {
					success = true;
				}
			} while (false);
			if(success) {
				break;
			} /*fall through otherwise*/
			return BAD_ARGUMENT;
		}
		case 'T':
			connection_timeout = atoi(arg);
			if(!connection_timeout) {
				twlog_crit("Bad timeout value!");
				return BAD_ARGUMENT;
			}
			break;
		case 'V': std::cerr << VERSION_STRING << "\n"; exit(1);

		case ARGP_KEY_END:
			/*make sure required arguments are present*/
			if(!(url.size() && outfile)) {
				std::cerr << "Need URL and outfile\n";
				argp_state_help(state, stderr, ARGP_HELP_LONG);
				return MISSING_REQUIRED_ARGUMENTS;
			}
			break;
		case ARGP_KEY_ARG:
		default:
			return ARGP_ERR_UNKNOWN;
		}

		return OPTION_OK;
	}
	int postprocess()
	{
		int success = 1;
		/*some postprocessing here. This configures our options a bit more,
		  using information acquired from all arguments*/
		if (url.size() && url.startsWith("https") && use_proxy) {
			if(getenv("https_proxy")) {
				QString s(getenv("https_proxy"));
				twlog_warn(qPrintable(s));
				if(!set_proxy(s))
					success = 0;
			}
		}
		return success;
	}
};

static QFile outfile;
static CLIOpts cliopts;

static error_t _parseopts_wrapper(int key, char *arg, struct argp_state *state)
{
	return cliopts.parse_opt(key, arg, state);
}

static int parse_options(int argc, char **argv) {
	struct argp argp = {opt_table, _parseopts_wrapper, NULL, ABOUT, NULL, NULL, NULL};
	if(argp_parse(&argp, argc, argv, 0, 0, NULL) != 0) {
		return 0;
	}
	return cliopts.postprocess();
}

static WebkitRenderer *gen_renderer() {
	/*generates a WebkitRenderer object based on the command line options..*/
	QNetworkRequest req;
	CustomNAM *qnam = new CustomNAM();
	qnam->connTimeout = cliopts.connection_timeout;

	/*make each request have these headers*/
	qnam->headers = cliopts.headers;
	if(!cliopts.url.isNull())
		req.setUrl(QUrl(cliopts.url));
	if(!cliopts.proxy_host.isNull()) {
		QNetworkProxy proxy(QNetworkProxy::HttpProxy, cliopts.proxy_host, cliopts.proxy_port);
		twlog_debug("PROXY: %s:%d", qPrintable(cliopts.proxy_host), cliopts.proxy_port);
		qnam->setProxy(proxy);
	}
	WebkitRenderer *r = new WebkitRenderer(req, qnam);
	r->baseUrl = QUrl(cliopts.baseurl);
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
		/*Signal Handler*/
		WebkitRenderer *r = qobject_cast<WebkitRenderer*>(QObject::sender());
		if(!r) {
			twlog_crit("Couldn't get WebkitRenderer object");
			exit(1);
		}
		if(!r->image.isNull()) {
			if(!r->image.save(&outfile, "png")) {
				twlog_crit("Problem saving %s: %s",
						  qPrintable(outfile.fileName()),
						  qPrintable(outfile.errorString()));
				exit(1);
			}
		} else {
			twlog_crit("PageLoadError!");
			QApplication::exit(1);
		}
		if(r->resultCode != WebkitRenderer::OK) {
			twlog_warn("Got image, but result code was %d",
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
#define pretty_prefix(mtype, prefix) \
	if(type == mtype) { \
	std::cerr << prefix << ": " << msg << "\n"; \
			return; \
	}
	if(cliopts.debug)
		pretty_prefix(QtDebugMsg, "DEBUG");
	pretty_prefix(QtWarningMsg, "WARNING");
	pretty_prefix(QtCriticalMsg, "CRITICAL");
	pretty_prefix(QtFatalMsg, "FATAL");
#undef pretty_prefix
}

int main(int argc, char **argv)
{
	qInstallMsgHandler(debuglogger);
	if(!parse_options(argc, argv))
		exit(1);

	QApplication a(argc, argv);

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

	if(!r->load()) {
		twlog_crit("Problem loading %s", qPrintable(cliopts.url));
		exit(1);
	}
	_SaveImage _si;

	QObject::connect(r, SIGNAL(done()), &_si, SLOT(save_image()));
	return a.exec();
}
