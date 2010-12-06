#ifndef CUSTOMPROXYFACTORY_H
#define CUSTOMPROXYFACTORY_H

#include <QNetworkProxyFactory>

class CustomProxyFactory : public QNetworkProxyFactory
{
public:
	enum ProxyPolicy {
		Liberal,
		Strict
	};

	CustomProxyFactory(
			QNetworkProxy httpProxy, QNetworkProxy sslProxy,
			CustomProxyFactory::ProxyPolicy policy = CustomProxyFactory::Strict);

	QNetworkProxy httpProxy;
	QNetworkProxy sslProxy;
	QList<QNetworkProxy> queryProxy(const QNetworkProxyQuery &query);
};

#endif // CUSTOMPROXYFACTORY_H
