#include <QApplication>
#include <QFile>
#include <QHash>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QNetworkAccessManager>
#include <QRegExp>
#include <QWebSettings>

#include "webkitrenderer.h"
#include "customnam.h"
#include "customproxyfactory.h"
#include "twutil.h"
#include "grabber.h"
#include <argp.h>

#include "configurablepage.h"

#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>

#include <iostream>

#define BAD_ARGUMENT 4
#define MISSING_REQUIRED_ARGUMENTS 5
#define OPTION_OK 0

#define VERSION_STRING "1-17-2011.0-CURRENT"

static const char ABOUT[] =
		"qtgrabber v" VERSION_STRING "\n"
		"qtgrabber is a webpage capturing utility that is somewhat tolerant "
		"of errors and is able to provide notification about them. It supports "
		"piping a complete HTTP header on stdin, or specifying individual "
		"header values using the -H option\n"
		"PROXY BEHAVIOR:\n"
		"You can specify an explicity http/https proxy with --proxy-TYPE where "
		"type is http or https. You can also use the -P option (optionally with "
		"an argument) to use the environment http_* variables as an *override* "
		"to proxies specified on the command line. In all three of these cases "
		"the --proxy-policy parameter to tell us whether you would like any "
		"proxy specified to be used as a fallback for the others.. e.g. if an "
		"http proxy should be used for https connections and vice versa, despite "
		"only having mentioned http_proxy or --proxy-http";


struct argp_option opt_table[] = {
/* basic options*/
{"outfile",		'o',	"FILE",		0,  "output file, or - for standard output", 0},
{"url",			'u',	"URL",		0,  "url to fetch, use file:///abspath for local files", 0},
{"baseurl",		'b',	"URL",		0,  "base url to use for relative references "
 "in local files", 0},

/*Options for extended proxy behavior*/
{"proxy",		'P',	"http[s]://PROXY:PORT", OPTION_ARG_OPTIONAL, "Use a proxy. If a"
	 "proxy is not provided on the command line, it will use an http*_proxy "
	 "environment variable", 1},
#define OPTKEY_PROXY_HTTPS 256
{"proxy-https", OPTKEY_PROXY_HTTPS, "https://PROXY:PORT", 0,
		"use PROXY for SSL connections", 1},
#define OPTKEY_PROXY_HTTP 257
{"proxy-http", OPTKEY_PROXY_HTTP, "http://PROXY:PORT", 0,
		"use PROXY for HTTP (non-SSL) connections", 1},
#define OPTKEY_PROXY_POLICY 258
{"proxy-policy", OPTKEY_PROXY_POLICY, "<strict|liberal>", 0,
		"What to do if there is only one proxy specified. `liberal' will use "
		"the given proxy for all kinds of connections, while `strict' will "
		"confine the use of this proxy for that type of connection specified. "
		"Default is 'liberal'", 1},

/*connection/request options*/
{"with-header", 'H',	"HEADER:VALUE", 0, "add a header field to the request", 1},
{"stdin-header",'S',	NULL,		0,	"indicate that a header is being piped on stdin", 1},
{"per-connection-timeout", 'T', "msecs", 0, "timeout for *each entity request*", 1},
{"global-timeout", 't',	"msecs",	0, "timeout for the entire fetch", 1},
{"insecure",	'k',	NULL,		0, "Ignore SSL errors", 1},
{"user",		'U',	"USER",		0, "Drop to USER when fetching page", 1},

/*js*/
#define OPTGROUP_JS 3
#define OPTKEY_JAVASCRIPT_OFF 259
#define OPTKEY_JS_ALERT_DISABLE 260
#define OPTKEY_JS_PROMPT_AUTORESPOND 261
#define OPTKEY_JS_PROMPT_RESPONSE 262
//#define OPTKEY_JS_CONFIRM_AUTORESPOND 263
#define OPTKEY_JS_CONFIRM_RESPONSE 264
{"no-javascript", OPTKEY_JAVASCRIPT_OFF, NULL, 0, "disable javascript", OPTGROUP_JS },
{"js-alert-disable", OPTKEY_JS_ALERT_DISABLE, NULL, 0, "disable js alerts", OPTGROUP_JS},
{"js-prompt-autorespond", OPTKEY_JS_PROMPT_AUTORESPOND, NULL, 0,
 "autorespond to js prompts. by default this will check for a user defined "
 "response (js-prompt-response), then for the default response, and then a "
 "null response", OPTGROUP_JS},
{"js-prompt-response", OPTKEY_JS_PROMPT_RESPONSE, "RESPONSE", 0,
 "user-defined response for js input. implies js-prompt-autorespond",
 OPTGROUP_JS},
{"js-confirm-response", OPTKEY_JS_CONFIRM_RESPONSE, "1|0", 0,
 "boolean integer corresponding to yes/no for confirm-style js dialogs. "
 "the default behavior is to show a popup", OPTGROUP_JS},

/*misc*/
{"debug",		'd',	"LEVEL", OPTION_ARG_OPTIONAL, "debug level", 2},
{"version",		'V',	NULL,	0,	"print version and exit", 2},
{NULL,			0,		NULL,	0,	NULL,					0}
};

#define _is_default_proxy(qp) \
	qp.type() == QNetworkProxy::DefaultProxy

class CLIOpts {
	/*Class to control, parse, and apply user specified options*/
public:
	CLIOpts() {
		headers = new QHash<QString,QString>;
		ignoreSSLErrors = false;
		userid = -1;
		proxyPolicy = CustomProxyFactory::Liberal;
		global_timeout = 0;
	}
	~CLIOpts() { delete headers; }
	/*value fields*/
	char * outfile;
	QString url;
	QString baseurl;
	uid_t userid;
	bool stdin_header;
	int debug;
	int connection_timeout;
	int global_timeout;
	QHash<QString,QString>* headers;
	QNetworkProxy sslProxy;
	QNetworkProxy httpProxy;
	CustomProxyFactory::ProxyPolicy proxyPolicy;
	bool ignoreSSLErrors;
	bool javascriptDisabled;

	struct jsopts jsOpts;

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
	bool set_proxy(QString text, QNetworkProxy *proxy) {
		QRegExp regex("^https?://([^:]+):(\\d+)$");
		if((regex.indexIn(text) == -1) ||
		   /*not needed, because the first will always fail*/
		   (regex.captureCount() != 2)) {
			twlog_warn("Problem parsing proxy!");
			return false;
			}
		/*Capture objects generally have the entire match as their first element*/

		proxy->setHostName((regex.capturedTexts()[1]));
		proxy->setPort(regex.capturedTexts()[2].toInt());
		proxy->setType(QNetworkProxy::HttpProxy);
		twlog_warn("Setting proxy: %s:%d", qPrintable(proxy->hostName()),proxy->port());
		/*finally, succeed*/
		return true;
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
			bool success = false;
			do {
#define apply_proxy_or_die(s, proxy) if(s.size() && !(success = set_proxy(s, proxy))) { break; }
				/*Check environment first, allow argument for overrides*/
				QString http_proxy(getenv("http_proxy"));
				QString https_proxy(getenv("https_proxy"));
				QString proxy_overrides(arg);
				apply_proxy_or_die(http_proxy, &httpProxy);
				apply_proxy_or_die(https_proxy, &sslProxy);
				if(proxy_overrides.startsWith("https")) {
					apply_proxy_or_die(proxy_overrides, &sslProxy);
				} else if(proxy_overrides.size()) {
					apply_proxy_or_die(proxy_overrides, &httpProxy);
				}
			} while (false);
			if(success) {
				break;
			} /*fall through otherwise*/
			twlog_crit("Proxy requested but none specified in the command line, "
					   "and none found in the environment!");
			return BAD_ARGUMENT;
		}
#undef apply_proxy_or_die
#define apply_proxy_or_die(proxy) if(set_proxy(QString(arg),proxy)) \
			{ break; } else { return BAD_ARGUMENT; }
		case OPTKEY_PROXY_HTTP: apply_proxy_or_die(&httpProxy);
		case OPTKEY_PROXY_HTTPS: apply_proxy_or_die(&sslProxy);
		case OPTKEY_PROXY_POLICY: {
			QString _policy = QString(arg).toLower();
			if(_policy=="liberal") { proxyPolicy=CustomProxyFactory::Liberal; }
			else if(_policy=="strict") { proxyPolicy=CustomProxyFactory::Strict; }
			else { return BAD_ARGUMENT; }
			break;
		}
#define _apply_timeout(dest) dest=atoi(arg); \
			if(!dest) { twlog_crit("Bad timeout value!"); return BAD_ARGUMENT; } break;
		case 'T': _apply_timeout(connection_timeout);
		case 't': _apply_timeout(global_timeout);
#undef _apply_timeout

		case 'k':
			ignoreSSLErrors = true;
			break;
		case OPTKEY_JAVASCRIPT_OFF:
			QWebSettings::globalSettings()->setAttribute(QWebSettings::JavascriptEnabled, false);
			break;
		case OPTKEY_JS_ALERT_DISABLE: jsOpts.alertDisable = true; break;
		case OPTKEY_JS_CONFIRM_RESPONSE:
			jsOpts.confirmAutoRespond = true;
			jsOpts.confirmResponse = atoi(arg);
			break;
		case OPTKEY_JS_PROMPT_AUTORESPOND: jsOpts.promptAutoRespond = true; break;
		case OPTKEY_JS_PROMPT_RESPONSE:
			jsOpts.promptResponse = QString(arg);
			jsOpts.promptAutoRespond = true;
			break;
		case 'U': {
			struct passwd *passwd = getpwnam(arg);
			if(!passwd) {
				twlog_crit("Error getting userid for %s: %s",
						   arg, strerror(errno));
				exit(1);
			}
			userid = passwd->pw_uid;
			break;
		}
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
	qnam->ignoreSSLErrors = cliopts.ignoreSSLErrors;

	/*make each request have these headers*/
	qnam->headers = cliopts.headers;
	if(!cliopts.url.isNull())
		req.setUrl(QUrl(cliopts.url));
	twlog_debug("PROXIES: SSL: %d HTTP: %d",
				!_is_default_proxy(cliopts.sslProxy),
				!_is_default_proxy(cliopts.httpProxy));
	CustomProxyFactory *factory = new CustomProxyFactory(
			cliopts.httpProxy, cliopts.sslProxy, cliopts.proxyPolicy);
	qnam->setProxyFactory(factory);
	WebkitRenderer *r = new WebkitRenderer(req, qnam);
	r->baseUrl = QUrl(cliopts.baseurl);
	r->page.globalTimeout = cliopts.global_timeout;
	r->page.jsOpts = cliopts.jsOpts;
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
		if(r->image.isNull()) {
			twlog_crit(GRABBER_STAGE_RENDER " got null image!");
			exit(1);
		}
		if(!r->image.save(&outfile, "png")) {
			twlog_crit(GRABBER_STAGE_OUTPUT " %s: %s",
					  qPrintable(outfile.fileName()),
					  qPrintable(outfile.errorString()));
			exit(1);
		}
		if(r->resultCode != WebkitRenderer::OK) {
			twlog_warn("got an image, but result code was %d", r->resultCode);
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
	if(cliopts.userid != -1) {
		twlog_debug("Dropping privileges to userid %d", cliopts.userid);
		if(setuid(cliopts.userid) == -1) {
			twlog_crit("Failed to drop privileges to userid %d: %s",
					   cliopts.userid, strerror(errno));
			exit(1);
		}
	}
	if(!r->load()) {
		twlog_crit("Problem loading %s", qPrintable(cliopts.url));
		exit(1);
	}
	_SaveImage _si;

	QObject::connect(r, SIGNAL(done()), &_si, SLOT(save_image()));
	return a.exec();
}
