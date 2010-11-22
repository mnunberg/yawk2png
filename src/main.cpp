#include <QApplication>
#include <QFile>
#include <QNetworkRequest>

#include "webkitrenderer.h"

#include <getopt.h>
#include <string.h>
#define USAGE_STRING "-o FILE -u URL -U USER_AGENT [-f <-l LISTEN_ADDRESS>] [-d]"
struct cliopts {
	char *outfile;
	char *url;
	char *listen_address;
	char *user_agent;
	int daemonize;
	int debug;
};

static struct cliopts cliopts;
static QFile outfile;

static void parse_options(int argc, char **argv)
{
	memset(&cliopts, 0, sizeof(cliopts));
	const struct option longopts[] = {
		{"outfile", required_argument, NULL, 'o'},
		{"url", required_argument, NULL, 'u'},
		{"user-agent", required_argument, NULL, 'U'},
		{"daemonize", no_argument, NULL, 'f'},
		{"listen-address", required_argument, NULL, 'l'},
		{"debug", optional_argument, NULL, 'd'},
		{"help", no_argument, NULL, 'h'}
	};
	const char *optstring = "u:o:U:l:fdh?";
	int optindex;
	char opt = getopt_long(argc, argv, optstring, longopts, &optindex);

	while(opt != -1) {
		switch (opt) {
		case 'o':
			cliopts.outfile = optarg;
			break;
		case 'u':
			cliopts.url = optarg;
			break;
		case 'U':
			cliopts.user_agent = optarg;
			break;
		case 'f':
			cliopts.daemonize = 1;
			break;
		case 'l':
			cliopts.listen_address = optarg;
			break;
		case 'd':
			cliopts.debug = (optarg) ? atoi(optarg) : 1;
			break;
		case 'h':
		case '?':
			qCritical(USAGE_STRING);
			exit(EXIT_FAILURE);
		}
		opt = getopt_long(argc, argv, optstring, longopts, &optindex);
	}
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
//			QApplication::exit(1);
			exit(1);
		}
		if(!r->image.isNull()) {
			if(!r->image.save(&outfile, "png")) {
				qCritical("Problem saving %s: %s",
						  qPrintable(outfile.fileName()),
						  qPrintable(outfile.errorString()));
//				QApplication::exit(1);
				exit(1);
			}
		} else {
			qCritical("PageLoadError!");
			QApplication::exit(1);
		}
		if(r->resultCode != WebkitRenderer::OK) {
			qWarning("Got image, but result code was %d",
					 r->resultCode);
//			QApplication::exit(1);
			exit(1);
		}
//		QApplication::exit(0);
		exit(0);
	}
};
#include "main.moc"
/*end*/


int main(int argc, char **argv)
{
	QApplication a(argc, argv);
	parse_options(argc, argv);
	if(!(cliopts.outfile &&  cliopts.url)) {
		qCritical(USAGE_STRING);
		exit(EXIT_FAILURE);
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
		exit(EXIT_FAILURE);
	}
	QNetworkRequest req(QUrl(cliopts.url));
	WebkitRenderer *r = new WebkitRenderer(req);
	r->load();
	_SaveImage _si;
	QObject::connect(r, SIGNAL(done()), &_si, SLOT(save_image()));
	return a.exec();
}
