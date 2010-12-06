#include "customproxyfactory.h"
#include "twutil.h"
#define _is_default_proxy(qp) \
	qp.type() == QNetworkProxy::DefaultProxy
CustomProxyFactory::CustomProxyFactory(QNetworkProxy httpProxy,
		QNetworkProxy sslProxy, CustomProxyFactory::ProxyPolicy policy)
{
	this->httpProxy = httpProxy;
	this->sslProxy = sslProxy;
	if(policy == Liberal) {
		/*This logic seems a bit convoluted at first, but it makes sense:
		  check both of our proxies, and if it's default, assign the *other*
		  to it, if the other is default as well, it's virtually a no-op*/
		if(_is_default_proxy(sslProxy)) {
			this->sslProxy = httpProxy;
		}
		if(_is_default_proxy(httpProxy)) {
			this->httpProxy = sslProxy;
		}
	}
}

	//TODO: in the future, maybe make this return a real list,
	//perhaps randomized
QList<QNetworkProxy> CustomProxyFactory::queryProxy(
		const QNetworkProxyQuery &query) {

	QList<QNetworkProxy> ret;
	if(query.protocolTag() == "http") {
		ret.append(httpProxy);
	} else if(query.protocolTag() == "https") {
		ret.append(sslProxy);
	}
	return ret;
}
