#include "mapivk.h"

#include "core/core.h"
#include "core/common.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QStringList>
#include <QDomDocument>
#include <QDebug>

#include <stdio.h>

void vkAudio::toVkRecord(QDomElement &in)
{
	in.setAttribute("oid",ownerId);
	in.setAttribute("aid",aid);
	in.setAttribute("artist",artist);
	in.setAttribute("title",title);
	in.setAttribute("duration",duration);
	in.setAttribute("url",url.toString());
	in.setAttribute("lyrics",lyricsId);
}

void vkAudio::fromVkRecord(QDomElement &in)
{
	ownerId		= in.attribute("oid","0").toLongLong();
	aid			= in.attribute("aid","0").toULongLong();
	artist		= in.attribute("artist","");
	title		= in.attribute("title","");
	duration	= in.attribute("duration","0").toUInt();
	setUrl(QUrl(in.attribute("url","0")));
	lyricsId	= in.attribute("lyrics","0").toULongLong();
}

void vkAudio::setUrl(QUrl newUrl)
{
	url			= newUrl;
	isValid		= url.isValid();

	if (isValid)
	{
		QString urlFile = newUrl.path();
		fileId = urlFile.right(urlFile.lastIndexOf('/')).left(urlFile.indexOf('.'));
	}
}

mApiVkRequest::mApiVkRequest(QObject *parent) :
	QObject(parent)
{
	rep = NULL;
}

mApiVkRequest::~mApiVkRequest()
{
	qDebug() << "~mApiVkRequest";
}

QUrl mApiVkRequest::getUrl()
{
	return url;
}

void mApiVkRequest::process(QNetworkReply *rep)
{
	QString xmlData = rep->readAll();
	QDomDocument xml;
	if (!xml.setContent(xmlData))
	{
		// Api error!!!
		qDebug() << "XML parsing error!!!";
		emit apiError(0,tr("Неизвестный ответ API"));
		return;
	}
	if (!xml.firstChildElement("error").isNull())
	{
		QDomElement respErr = xml.firstChildElement("error");
		int eCode = respErr.firstChildElement("error_code").text().toInt();
		QString eMsg = respErr.firstChildElement("error_msg").text();

		if (214		== eCode)		eMsg = tr("Стена пользователя закрыта");
		if (14		== eCode)		eMsg = tr("Введите капчу");
		if (10005	== eCode)		eMsg = tr("Действие выполняется слишком часто! Пожалуйста, попробуйте позже");
		emit apiError(eCode,eMsg);
		return;
	}
	QDomElement resp = xml.firstChildElement("response");
	if (resp.isNull())
	{
		emit apiError(0,tr("Пустой ответ API"));
		return; // Error!
	}

	if (API_GETPROFILE == method)
	{
		// Get user profile...
		vkUserProfile profile;
		QDomElement user = resp.firstChildElement("user");

		profile.id = user.firstChildElement("uid").text().toUInt();
		profile.name_f = user.firstChildElement("first_name").text();
		profile.name_l = user.firstChildElement("last_name").text();
		profile.name_n = user.firstChildElement("nickname").text();
		profile.screen_name = user.firstChildElement("screen_name").text();
		profile.photo  = QUrl(user.firstChildElement("photo").text());
		profile.online  = user.firstChildElement("online").text().toInt() == 1;
		profile.sex  = (vkUserProfile::userSex)user.firstChildElement("sex").text().toInt();

		emit profileLoaded(profile);
		return;
	}

	if (API_GETGROUP == method)
	{
		vkGroup group;
		QDomElement groupTag = resp.firstChildElement("group");

		group.gid			= groupTag.firstChildElement("gid").text().toULongLong();
		group.title			= groupTag.firstChildElement("name").text();
		group.screen_name	= groupTag.firstChildElement("screen_name").text();

		emit groupinfoLoaded(group);
		return;
	}

	if (API_FRIENDS == method)
	{
		vkUserProfileList list;
		QDomNodeList lst = resp.elementsByTagName("user");
		for (int x=0; x<lst.size(); x++)
		{
			QDomElement user = lst.at(x).toElement();
			vkUserProfile profile;

			profile.id = user.firstChildElement("uid").text().toULongLong();
			profile.name_f = user.firstChildElement("first_name").text();
			profile.name_l = user.firstChildElement("last_name").text();
			profile.name_n = user.firstChildElement("nickname").text();
			profile.screen_name = user.firstChildElement("screen_name").text();
			profile.photo  = QUrl(user.firstChildElement("photo").text());
			profile.online  = user.firstChildElement("online").text().toInt() == 1;
			profile.sex  = (vkUserProfile::userSex)user.firstChildElement("sex").text().toInt();

			list.append(profile);
		}

		emit friendsLoaded(list);
		return;
	}

	if (API_USERADIO == method)
	{
		vkAudioList list;
		QDomNodeList lst = resp.elementsByTagName("audio");
		for (int x=0; x<lst.size(); x++)
		{
			QDomElement tag = lst.at(x).toElement();
			vkAudio audio;

			audio.aid		= tag.firstChildElement("aid").text().toULongLong();
			audio.ownerId	= tag.firstChildElement("owner_id").text().toLongLong();
			audio.lyricsId	= tag.firstChildElement("lyrics_id").text().toULongLong();
			audio.duration	= tag.firstChildElement("duration").text().toUInt();
			audio.artist	= normalizeString(tag.firstChildElement("artist").text()).trimmed();
			audio.title 	= normalizeString(tag.firstChildElement("title").text()).trimmed();
			audio.url		= QUrl(tag.firstChildElement("url").text());

			audio.isValid = audio.url.isValid();

			list.append(audio);
		}

		emit userAudioLoaded(list);
		return;
	}

	if (API_SEARCHAUDIO == method)
	{
		vkAudioList list;
		QDomNodeList lst = resp.elementsByTagName("audio");
		for (int x=0; x<lst.size(); x++)
		{
			QDomElement tag = lst.at(x).toElement();
			vkAudio audio;

			audio.aid		= tag.firstChildElement("aid").text().toULongLong();
			audio.ownerId	= tag.firstChildElement("owner_id").text().toLongLong();
			audio.lyricsId	= tag.firstChildElement("lyrics_id").text().toULongLong();
			audio.duration	= tag.firstChildElement("duration").text().toUInt();
			audio.artist	= normalizeString(tag.firstChildElement("artist").text()).trimmed();
			audio.title 	= normalizeString(tag.firstChildElement("title").text()).trimmed();
			audio.url		= QUrl(tag.firstChildElement("url").text());

			audio.isValid = audio.url.isValid();

			list.append(audio);
		}

		emit searchAudioLoaded(list);
		return;
	}

	if (API_GETAUDIO == method)
	{
		vkAudio audio;
		QDomElement tag = resp.firstChildElement("audio");

		audio.aid		= tag.firstChildElement("aid").text().toULongLong();
		audio.ownerId	= tag.firstChildElement("owner_id").text().toLongLong();
		audio.lyricsId	= tag.firstChildElement("lyrics_id").text().toULongLong();
		audio.duration	= tag.firstChildElement("duration").text().toUInt();
		audio.artist	= normalizeString(tag.firstChildElement("artist").text()).trimmed();
		audio.title 	= normalizeString(tag.firstChildElement("title").text()).trimmed();
		audio.url		= QUrl(tag.firstChildElement("url").text());

		audio.isValid = audio.url.isValid();

		emit getAudioLoaded(audio);
		return;
	}

	if (API_USERGROUPS == method)
	{
		vkGroupList list;
		QDomNodeList lst = resp.elementsByTagName("group");
		for (int x=0; x<lst.size(); x++)
		{
			QDomElement tag = lst.at(x).toElement();
			vkGroup group;

			group.gid			= tag.firstChildElement("gid").text().toULongLong();
			group.title			= normalizeString(tag.firstChildElement("name").text()).trimmed();
			group.screen_name	= normalizeString(tag.firstChildElement("screen_name").text()).trimmed();

			list.append(group);
		}

		emit userGroupsLoaded(list);
		return;
	}
	if (API_WALLPOST == method)
	{
		if (resp.elementsByTagName("post_id").size())
		{
			quint64 postId = resp.elementsByTagName("post_id").at(0).toElement().text().toULongLong();
			emit sendedWallAudio(posterUid, postId);
			return;
		}
	}
	if (API_SAVEAUDIO == method)
	{
		emit audioAdded();
		return;
	}
	if (API_STATUSONLINE)
	{
		emit statusOnline();
		return;
	}
	emit apiError(0,tr("Пустой ответ API"));
}

void mApiVkRequest::profileLoad(quint64 userId)
{
	method = API_GETPROFILE;
	url    = baseUrl("users.get");
	QUrlQuery q;
	q.addQueryItem("uids",QString::number(userId));
	q.addQueryItem("fields","uid,first_name,last_name,nickname,screen_name,sex,photo,online");
	q.addQueryItem("name_case","nom");
	url.setQuery(q);
}

void mApiVkRequest::groupInfoLoad(quint64 groupId)
{
	method = API_GETGROUP;
	url    = baseUrl("groups.getById");
	QUrlQuery q;
	q.addQueryItem("gid",QString::number(groupId));
	q.addQueryItem("fields","");
	url.setQuery(q);
}

void mApiVkRequest::friendsLoad(quint64 userId)
{
	method = API_FRIENDS;
	url    = baseUrl("friends.get");
	QUrlQuery q;
	if (userId)
		q.addQueryItem("uid",QString::number(userId));
	q.addQueryItem("fields","uid,first_name,last_name,nickname,screen_name,sex,photo,online");
	q.addQueryItem("name_case","nom");
	q.addQueryItem("order","hints");
	url.setQuery(q);
}

void mApiVkRequest::userAudioLoad(quint64 userId)
{
	method = API_USERADIO;
	url    = baseUrl("audio.get");
	QUrlQuery q;
	if (userId)
		q.addQueryItem("uid",QString::number(userId));
	url.setQuery(q);
}

void mApiVkRequest::groupAudioLoad(quint64 groupId)
{
	method = API_USERADIO;
	url    = baseUrl("audio.get");
	QUrlQuery q;
	if (groupId)
		q.addQueryItem("gid",QString::number(groupId));
	url.setQuery(q);
}

void mApiVkRequest::userGroupsLoad(quint64 userId)
{
	method = API_USERGROUPS;
	url    = baseUrl("groups.get");
	QUrlQuery q;
	if (userId)
		q.addQueryItem("uid",QString::number(userId));
	q.addQueryItem("extended","1");
	url.setQuery(q);
}

void mApiVkRequest::sendWallAudio(quint64 userId, vkAudioList audio, QString message)
{
	method = API_WALLPOST;
	url    = baseUrl("wall.post");
	QUrlQuery q;
	if (userId)
		q.addQueryItem("owner_id",QString::number(userId));
	message = message.trimmed();
	if (message.length() > 0)
		q.addQueryItem("message",message);

	QStringList attaches;
	for (int x=0; x<audio.size() && x<10; x++)
		attaches << QString("audio") + QString::number(audio.at(x).ownerId) + "_" + QString::number(audio.at(x).aid);
	q.addQueryItem("attachments",attaches.join(","));
	url.setQuery(q);
	posterUid = userId;
}

void mApiVkRequest::sendStatusAudio(qint64 oid, quint64 aid)
{
	method = API_STATUSAUDIO;
	url    = baseUrl("status.set");
	QUrlQuery q;
	q.addQueryItem("text","");
	q.addQueryItem("audio",QString::number(oid) + "_" + QString::number(aid));
	url.setQuery(q);
}

void mApiVkRequest::sendStatusOnline()
{
	method = API_STATUSONLINE;
	url    = baseUrl("account.setOnline");
}

void mApiVkRequest::searchAudio(QString text, vkAudioSort sort, bool fixMisspell, uint offset, uint size)
{
	method = API_SEARCHAUDIO;
	url    = baseUrl("audio.search");
	QUrlQuery q;
	q.addQueryItem("q",text);
	q.addQueryItem("sort",QString::number((int)sort));
	q.addQueryItem("count",QString::number(size));
	q.addQueryItem("offset",QString::number(offset));
	q.addQueryItem("auto_complete",fixMisspell?"1":"0");
	url.setQuery(q);
}

void mApiVkRequest::getAudio(qint64 oid, quint64 aid)
{
	method = API_GETAUDIO;
	url    = baseUrl("audio.getById");
	QUrlQuery q;
	q.addQueryItem("audios",QString::number(oid)+"_"+QString::number(aid));
	url.setQuery(q);
}

void mApiVkRequest::saveAudio(qint64 oid, quint64 aid)
{
	method = API_SAVEAUDIO;
	url    = baseUrl("audio.add");
	QUrlQuery q;
	q.addQueryItem("aid",QString::number(aid));
	q.addQueryItem("oid",QString::number(oid));
	url.setQuery(q);
}

void mApiVkRequest::setReply(QNetworkReply *r)
{
	rep = r;
	connect(rep,SIGNAL(finished()),this,SLOT(requestFinished()),Qt::QueuedConnection);
}

QUrl mApiVkRequest::baseUrl(QString method)
{
	QUrl url_out = QUrl("https://api.vk.com/method/" + method + ".xml");
	QUrlQuery q;
	q.addQueryItem("access_token",mCore::instance()->getApiTokenVk());
	url_out.setQuery(q);
	return url_out;
}

QString mApiVkRequest::normalizeString(QString s)
{
	s = s.replace("&amp;","&");
	s = s.replace("&#33;","!");
	s = s.replace("&#34;","\"");
	s = s.replace("&#35;","#");
	s = s.replace("&#036;","$");
	s = s.replace("&#37;","%");
	s = s.replace("&#38;","&");
	s = s.replace("&#39;","'");
	s = s.replace("&#092;","\\");
	s = s.replace("&#92;","\\");
	s = s.replace("&quot;","\"");
	s = s.replace("&lt;","<");
	s = s.replace("&gt;",">");
	return s;
}

void mApiVkRequest::requestFinished()
{
	process(rep);
	rep->close();
	rep->abort();
	rep->deleteLater();
	deleteLater();
}

mApiVk::mApiVk(QObject *parent) :
	QThread(parent)
{
	QObject::moveToThread(this);

	qRegisterMetaType<mApiVkRequest*>("mApiVkRequest*");
	qRegisterMetaType<vkUserProfile>("vkUserProfile");
	qRegisterMetaType< QList<vkUserProfile> >("QList<vkUserProfile>");

	reqTimer.start();
}

mApiVk::~mApiVk()
{
	delete net;
}

mApiVk * mApiVk_glob_instance = NULL;
mApiVk * mApiVk::instance()
{
	if (NULL == mApiVk_glob_instance)
		mApiVk_glob_instance = new mApiVk();
	return mApiVk_glob_instance;
}

void mApiVk::run()
{
	moveToThread(this);
	net = new mNetowrkAccess(this);
	exec();
}

void mApiVk::addRequest(mApiVkRequest *r)
{
	// Go last request timeout
	int tmsWait = calcWait();
	if (0 != tmsWait)
	{
		qDebug() << "Slow-down request to prevent ban... Time: " << tmsWait << " ms";
		usleep(tmsWait);
	}
	setProxy(net);
	QNetworkRequest req(QUrl(r->getUrl()));
	QNetworkReply * ans = net->get(req);
	r->setReply(ans);
}

int mApiVk::calcWait()
{
	if (requestHistory.size() < 3) return 0;
	// get time of 3-rd request
	int reqTime = reqTimer.elapsed();
	int reqTimeDiff = reqTime - requestHistory.at(2);
	requestHistory.push_front(reqTime);
	if (requestHistory.size() > 3) requestHistory.pop_back();
	if (reqTimeDiff < 1000)
	{
		return 1000 - reqTimeDiff;
	} else {
		return 0;
	}
}

void mApiVk::request(mApiVkRequest *r)
{
	// Create request...
	QMetaObject::invokeMethod(mApiVk::instance(),"addRequest",Qt::QueuedConnection,Q_ARG(mApiVkRequest*, r));
}

