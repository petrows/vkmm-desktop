#include "mapilastfm.h"
#include "core/common.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDomDocument>
#include <QDomElement>
#include <QUrlQuery>
#include <QDebug>

#include "config.h"

void lastFmTrack::toXml(QDomElement &in)
{
	in.setAttribute("artist", artist);
	in.setAttribute("title", title);
	in.setAttribute("mbid_t", mbid);
	in.setAttribute("mbid_a", mbidArtist);
}

void lastFmTrack::fromXml(QDomElement &in)
{
	artist = in.attribute("artist");
	title = in.attribute("title");
	mbid = in.attribute("mbid_t");
	mbidArtist = in.attribute("mbid_a");
}

mApiLastfmRequest::mApiLastfmRequest(QObject *parent) : QObject(parent)
{
	isPost = false;
}

mApiLastfmRequest::~mApiLastfmRequest()
{
}

QUrl mApiLastfmRequest::baseUrl(QString method)
{
	QUrl requestUrl = QUrl("http://ws.audioscrobbler.com/2.0/");
	QUrlQuery q;
	q.addQueryItem("method", method.toStdString().c_str());
	requestUrl.setQuery(q);
	return requestUrl;
}

void mApiLastfmRequest::process(QNetworkReply *rep)
{
	QString xmlData = rep->readAll();
	// qDebug() << xmlData;
	QDomDocument xml;
	if (!xml.setContent(xmlData))
	{
		// Api error!!!
		qDebug() << "XML parsing error!!!";
		emit apiError(0,tr("Неизвестный ответ API"));
		return;
	}

	QDomElement resp = xml.firstChildElement("lfm");
	if (resp.isNull())
	{
		emit apiError(0,tr("Пустой ответ API"));
		return; // Error!
	}

	if (API_SESSION_OPEN == method)
	{
		// Get user profile...
		QDomElement session = resp.firstChildElement("session");

		QString user = session.firstChildElement("name").text();
		QString sid  = session.firstChildElement("key").text();

		emit sessionOpenResult(!sid.isEmpty(), user, sid);
		return;
	}

	if (API_TRACK_SCROBBLE == method)
	{
		// Get user profile...
		QDomElement scrobble = resp.firstChildElement("scrobbles").firstChildElement("scrobble");

		QString artist = scrobble.firstChildElement("artist").text();
		QString title  = scrobble.firstChildElement("track").text();

		emit trackScrobbed(artist, title);
		return;
	}

	if (API_TRACK_LOVE == method)
	{
		return;
	}

	if (API_USER_LOVED == method)
	{
		lastFmTrackList list;
		QDomNodeList tracksList = resp.firstChildElement("lovedtracks").elementsByTagName("track");
		for (int x=0; x<tracksList.size(); x++)
		{
			QDomElement trackElement = tracksList.at(x).toElement();

			lastFmTrack track;
			track.artist		= trackElement.firstChildElement("artist").firstChildElement("name").text();
			track.mbidArtist	= trackElement.firstChildElement("artist").firstChildElement("mbid").text();
			track.title			= trackElement.firstChildElement("name").text();
			track.mbid			= trackElement.firstChildElement("mbid").text();
			list.push_back(track);
		}

		emit userLovedLoaded(list);
		emit tracksListLoaded(list);
		return;
	}

	if (API_USER_TOP == method)
	{
		lastFmTrackList list;
		QDomNodeList tracksList = resp.firstChildElement("toptracks").elementsByTagName("track");
		for (int x=0; x<tracksList.size(); x++)
		{
			QDomElement trackElement = tracksList.at(x).toElement();

			lastFmTrack track;
			track.artist		= trackElement.firstChildElement("artist").firstChildElement("name").text();
			track.mbidArtist	= trackElement.firstChildElement("artist").firstChildElement("mbid").text();
			track.title			= trackElement.firstChildElement("name").text();
			track.mbid			= trackElement.firstChildElement("mbid").text();
			list.push_back(track);
		}

		emit userTracksLoaded(list);
		emit tracksListLoaded(list);
		return;
	}

	if (API_USER_FRIENDS == method)
	{
		lastFmUserList list;
		QDomNodeList userlist = resp.firstChildElement("friends").elementsByTagName("user");
		for (int x=0; x<userlist.size(); x++)
		{
			QDomElement tag = userlist.at(x).toElement();

			lastFmUser user;
			user.name		= tag.firstChildElement("name").text();
			user.realname	= tag.firstChildElement("realname").text();
			list.push_back(user);
		}

		emit userFriendsLoaded(list);
		return;
	}

	if (API_ARTIST_TOP == method)
	{
		lastFmTrackList list;
		QDomNodeList tracksList = resp.firstChildElement("toptracks").elementsByTagName("track");
		for (int x=0; x<tracksList.size(); x++)
		{
			QDomElement trackElement = tracksList.at(x).toElement();

			lastFmTrack track;
			track.artist		= requestData1;
			track.mbidArtist	= requestData2;
			track.title			= trackElement.firstChildElement("name").text();
			track.mbid			= trackElement.firstChildElement("mbid").text();
			list.push_back(track);
		}

		emit tracksListLoaded(list);
		return;
	}

	if (API_SIM_TRACK == method)
	{
		lastFmTrackList list;
		QDomNodeList tracksList = resp.firstChildElement("similartracks").elementsByTagName("track");
		for (int x=0; x<tracksList.size(); x++)
		{
			QDomElement trackElement = tracksList.at(x).toElement();

			lastFmTrack track;
			track.artist		= trackElement.firstChildElement("artist").firstChildElement("name").text();
			track.mbidArtist	= trackElement.firstChildElement("artist").firstChildElement("mbid").text();
			track.title			= trackElement.firstChildElement("name").text();
			track.mbid			= trackElement.firstChildElement("mbid").text();
			list.push_back(track);
		}

		emit similarLoaded(list);
		emit tracksListLoaded(list);
		return;
	}

	// Emit error
	emit apiError(-1, tr("Неизвестный ответ API"));
}

void mApiLastfmRequest::sessionOpen(QString user, QString token)
{
	method = API_SESSION_OPEN;
	url    = baseUrl("auth.getMobileSession");
	QUrlQuery q(url.query());
	q.addQueryItem("username", user.toStdString().c_str());
	q.addQueryItem("authToken", token.toStdString().c_str());
	url.setQuery(q);
}

void mApiLastfmRequest::trackNowPlaying(QString artist, QString title, QString mbId)
{
	method = API_TRACK_NOWPLAYING;
	isPost = true; // Write request
	url    = baseUrl("track.updateNowPlaying");
	QUrlQuery q(url.query());
	q.addQueryItem("track", title.toStdString().c_str());
	q.addQueryItem("artist", artist.toStdString().c_str());
	url.setQuery(q);
}

void mApiLastfmRequest::trackScrobble(QString artist, QString title, quint32 duration, QString mbId, quint64 streamId, uint tms)
{
	method = API_TRACK_SCROBBLE;
	isPost = true; // Write request
	url    = baseUrl("track.scrobble");
	QUrlQuery q(url.query());
	q.addQueryItem("track", title.toStdString().c_str());
	q.addQueryItem("artist", artist.toStdString().c_str());
	if (!mbId.isEmpty())	q.addQueryItem("mbid", mbId.toStdString().c_str());
	if (0 != streamId)		q.addQueryItem("streamId", QString::number(streamId).toStdString().c_str());
	if (0 != duration)		q.addQueryItem("duration", QString::number(duration).toStdString().c_str());
	if (0 == tms)
	{
		tms = QDateTime::currentDateTime().toTime_t() - 60;
	}
	q.addQueryItem("timestamp", QString::number(tms).toStdString().c_str());
	url.setQuery(q);
}

void mApiLastfmRequest::trackLove(QString artist, QString title, bool isLove)
{
	method = API_TRACK_LOVE;
	isPost = true; // Write request
	if (isLove)
	{
		url = baseUrl("track.love");
	} else {
		url = baseUrl("track.unlove");
	}
	QUrlQuery q(url.query());
	q.addQueryItem("track", title.toStdString().c_str());
	q.addQueryItem("artist", artist.toStdString().c_str());
	url.setQuery(q);
}

void mApiLastfmRequest::userLoved(QString user, uint limit)
{
	method = API_USER_LOVED;
	url    = baseUrl("user.getLovedTracks");
	QUrlQuery q(url.query());
	q.addQueryItem("user", user.toStdString().c_str());
	q.addQueryItem("limit", "880000");
	url.setQuery(q);
}

void mApiLastfmRequest::userRecomended(QString user, uint limit)
{
	method = API_USER_RECOMEND;
	url    = baseUrl("user.getLovedTracks");
	QUrlQuery q(url.query());
	q.addQueryItem("user", user.toStdString().c_str());
	q.addQueryItem("limit", "880000");
	url.setQuery(q);
}

void mApiLastfmRequest::userTracks(QString user, mApiLastfmRequest::apiTopPeriod period, uint limit)
{
	method = API_USER_TOP;
	url    = baseUrl("user.getTopTracks");
	QUrlQuery q(url.query());
	q.addQueryItem("user", user.toStdString().c_str());
	QString periodString = "overall";
	switch (period)
	{
		case mApiLastfmRequest::PERIOD_WEEK:
			periodString = "7day";
			break;
		case mApiLastfmRequest::PERIOD_MONTH_3:
			periodString = "3month";
			break;
		case mApiLastfmRequest::PERIOD_MONTH_6:
			periodString = "6month";
			break;
		case mApiLastfmRequest::PERIOD_MONTH_12:
			periodString = "12month";
			break;
		default:
			break;
	}
	q.addQueryItem("period", periodString.toStdString().c_str());
	q.addQueryItem("limit", QString::number(limit).toStdString().c_str());
	url.setQuery(q);
}

void mApiLastfmRequest::userFriends(QString user)
{
	method = API_USER_FRIENDS;
	url    = baseUrl("user.getFriends");
	QUrlQuery q(url.query());
	q.addQueryItem("user", user.toStdString().c_str());
	q.addQueryItem("recenttracks", "0");
	q.addQueryItem("limit", "2000");
	url.setQuery(q);
}

void mApiLastfmRequest::artistTracks(QString artist, QString mbid, uint limit)
{
	method = API_ARTIST_TOP;
	url    = baseUrl("artist.getTopTracks");
	QUrlQuery q(url.query());
	if (!mbid.isEmpty())
	{
		q.addQueryItem("mbid", mbid.toStdString().c_str());
	} else {
		q.addQueryItem("artist", artist.toStdString().c_str());
	}
	q.addQueryItem("limit", QString::number(limit).toStdString().c_str());
	q.addQueryItem("autocorrect", "1");
	url.setQuery(q);

	requestData1 = artist;
	requestData2 = mbid;
}

void mApiLastfmRequest::similarTrack(QString artist, QString title, QString mbid)
{
	method = API_SIM_TRACK;
	url    = baseUrl("track.getSimilar");
	QUrlQuery q(url.query());
	if (!mbid.isEmpty())
	{
		q.addQueryItem("mbid", mbid.toStdString().c_str());
	} else {
		q.addQueryItem("track", title.toStdString().c_str());
		q.addQueryItem("artist", artist.toStdString().c_str());
	}
	q.addQueryItem("limit", "2000");
	q.addQueryItem("autocorrect", "1");
	url.setQuery(q);
}

void mApiLastfmRequest::setSign()
{
	mApiLastfm::setSign(url);
}

void mApiLastfmRequest::setReply(QNetworkReply *r)
{
	rep = r;
	connect(rep,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(requestProgress(qint64,qint64)),Qt::QueuedConnection);
	connect(rep,SIGNAL(finished()),this,SLOT(requestFinished()),Qt::QueuedConnection);
}

QUrl mApiLastfmRequest::getUrl()
{
	return url;
}

void mApiLastfmRequest::requestFinished()
{
	process(rep);
	rep->close();
	rep->abort();
	rep->deleteLater();
	deleteLater();
}

void mApiLastfmRequest::requestProgress(qint64 bytesReceived, qint64)
{
	emit loadProgress(bytesReceived);
}

mApiLastfm::mApiLastfm(QObject *parent) :
	QThread(parent)
{
	QObject::moveToThread(this);
	qRegisterMetaType<mApiLastfmRequest*>("mApiLastfmRequest*");
	qRegisterMetaType<lastFmTrackList>("lastFmTrackList");
}

mApiLastfm::~mApiLastfm()
{
	delete net;
}

mApiLastfm *mApiLastfm::instance()
{
	static mApiLastfm * obj = NULL;
	if (NULL == obj)
		obj = new mApiLastfm();
	return obj;
}

void mApiLastfm::run()
{
	moveToThread(this);
	net = new mNetowrkAccess(this);
	exec();
}

void mApiLastfm::request(mApiLastfmRequest *r)
{
	QMetaObject::invokeMethod(mApiLastfm::instance(),"addRequest",Qt::QueuedConnection,Q_ARG(mApiLastfmRequest*, r));
}

QString mApiLastfm::genToken(QString login, QString password)
{
	return md5(login.toLower() + md5(password));
}

void mApiLastfm::addRequest(mApiLastfmRequest *r)
{
	setProxy(net);
	r->setSign();
	QNetworkReply * ans;
	if (r->postMode())
	{
		QUrl postUrl = r->getUrl();
		QUrl origUrl = r->getUrl();
		QStringList params;
		QUrlQuery q = QUrlQuery(postUrl.query());
		QList< QPair<QString,QString> > urlKeys = q.queryItems();
		for (int x=0; x<urlKeys.size(); x++)
		{
			params.push_back(urlKeys.at(x).first + "=" + q.queryItemValue(urlKeys.at(x).first.toStdString().c_str(), QUrl::FullyEncoded));
		}
		postUrl.setQuery("");
		QNetworkRequest req(postUrl);
		req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
		ans = net->post(req, params.join("&").toStdString().c_str());
	} else {
		QNetworkRequest req(QUrl(r->getUrl()));
		ans = net->get(req);
	}
	r->setReply(ans);
}

QString mApiLastfm::genSign(QUrl requestUrl)
{
	QStringList params;
	QUrlQuery q = QUrlQuery(requestUrl.query());
	QList< QPair<QString,QString> > urlKeys = q.queryItems();
	for (int x=0; x<urlKeys.size(); x++)
	{
		params.push_back(urlKeys.at(x).first + urlKeys.at(x).second);
	}
	params.sort();
	return md5(params.join("") + LASTFM_SECRET);
}

void mApiLastfm::setSign(QUrl &requestUrl)
{
	QUrlQuery q = QUrlQuery(requestUrl.query());
	if (mApiLastfm::instance()->isActive()) q.addQueryItem("sk", mApiLastfm::instance()->sid.toStdString().c_str());
	q.addQueryItem("api_key", LASTFM_KEY);
	QString sign = mApiLastfm::genSign(requestUrl);
	q.addQueryItem("api_sig", sign.toStdString().c_str());
	requestUrl.setQuery(q);
}

void mApiLastfm::setSid(QString newSid)
{
	sid = newSid;
}

bool mApiLastfm::isActive()
{
	return !sid.isEmpty();
}
