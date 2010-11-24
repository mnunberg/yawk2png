#ifndef TWUTIL_H
#define TWUTIL_H

#include <QObject>
#include <QFileInfo>
#define fn_log(level, fmt, ...) \
	q ## level (qPrintable( \
							QString().sprintf("%s:%d(%s): ", qPrintable(QFileInfo(__FILE__).baseName()), __LINE__, __func__) + \
					   QString().sprintf(fmt, ## __VA_ARGS__)))

#define twlog_debug(fmt, ...) fn_log(Debug, fmt, ## __VA_ARGS__)
#define twlog_warn(fmt, ...) fn_log(Warning, fmt, ## __VA_ARGS__)
#define twlog_crit(fmt, ...) fn_log(Critical, fmt, ## __VA_ARGS__)

#define fn_begin twlog_debug("begin");
#define fn_end twlog_debug("end");

class TWUtil : public QObject
{
Q_OBJECT
public:
    explicit TWUtil(QObject *parent = 0);
signals:

public slots:
	 void dumpDestroyed(void);
	 void logCreation(QObject*);
};

extern TWUtil *twutil;

#endif // TWUTIL_H

