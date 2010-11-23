#include "twutil.h"

#define getname(object) ((!object->objectName().isNull()) ? object->objectName() : "<unnamed>")

void pretty_print(QString title, QObject *object) {
	qDebug(
			"====%s====: %-15s [%-10p (parent: %-10s %-10p)]",
			qPrintable(title),
			qPrintable(getname(object)),
			object,
			qPrintable(object->parent() ? getname(object->parent()) : "NULL"),
			object->parent()
			);
}

TWUtil::TWUtil(QObject *parent) :
    QObject(parent)
{
}
void TWUtil::dumpDestroyed()
{
	/*to be called when sent a destroyed object*/
	pretty_print("Deleted", sender());

}

void TWUtil::logCreation(QObject* object)
{
	pretty_print("Created", object);
}


TWUtil *twutil = new TWUtil(0);
