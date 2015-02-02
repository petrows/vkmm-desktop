#include "core/common.h"
#include "core/core.h"
#include "version.h"
#include "build-time.h"

#include <QCryptographicHash>
#include <QFile>
#include <QDir>
#include <QPushButton>
#include <QDebug>

QString getCss(QString name)
{
    QString out;
    // Platform-file:
    out += getFile(QString(":/css/") + name + "-" + OS_NAME + ".css");

	// qDebug() << "Css:" << name << out;
    return out;
}

QString getFile(QString name)
{
    QFile f(name);
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    QString ret = f.readAll();
    f.close();
	return ret;
}

void getVersion(int & major, int & minor, int & build)
{
	int vArray[4] = {VER_PRODUCTVERSION};
	major = vArray[0];
	minor = vArray[1];
	build = vArray[2];
}

QString getArch()
{
    return VER_BUILD_ARCH;
}

QString getVersion()
{
	int major, minor, build;
	getVersion(major,minor,build);
	QString ret = QString("%1.%2.%3").arg(major).arg(minor).arg(build);
	return ret;
}

QDateTime getBuildTime()
{
	QString buildDate = __DATE__" "__TIME__; // Jun 16 2012 16:55:47
	QStringList buildDateComp = buildDate.split(" ");

	int monthNum = getMonthNum(buildDateComp.at(0));

	buildDate = QObject::tr("%1.%2.%3 %4").arg(buildDateComp.at(1)).arg(monthNum).arg(buildDateComp.at(2)).arg(buildDateComp.at(3));

	return QDateTime::fromString(buildDate,"dd.M.yyyy HH:mm:ss");
}

int getMonthNum(const QString & m)
{
	const char * mids[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

	for (int x=0; x<12; x++)
	{
		if (m == mids[x]) return x+1;
	}
	return -1;
}

void removeDir(QString fname)
{
	QFileInfo finf(fname);
	if (finf.isDir())
	{
		// It is a dir...
		QDir dr(fname);
		QStringList dr_data = dr.entryList(QDir::AllEntries | QDir::NoDotAndDotDot|QDir::Hidden|QDir::System);
		for (int x=0; x<dr_data.count(); x++)
		{
			if (dr_data.at(x) == "." || dr_data.at(x) == "..") continue;
			removeDir(fname + "/" + dr_data.at(x));
		}
		dr.rmpath(fname);
	} else {
		// File?
		QFile::remove(fname);
	}
}

langButtons::langButtons()
{
	btnOk		= QObject::tr("ОК");
	btnCancel	= QObject::tr("Отмена");
	btnApply	= QObject::tr("Применить");
	btnSave		= QObject::tr("Сохранить");
}

void langButtons::setButtons(QDialogButtonBox *box)
{
	langButtons * obj = new langButtons();
	if (box->standardButtons()&QDialogButtonBox::Ok)
	{
		box->button(QDialogButtonBox::Ok)->setText(obj->btnOk);
		box->button(QDialogButtonBox::Ok)->setIcon(QIcon(":/icons/tick.png"));
	}
	if (box->standardButtons()&QDialogButtonBox::Cancel)
	{
		box->button(QDialogButtonBox::Cancel)->setText(obj->btnCancel);
		box->button(QDialogButtonBox::Cancel)->setIcon(QIcon(":/icons/cross.png"));
	}
	if (box->standardButtons()&QDialogButtonBox::Apply)
	{
		box->button(QDialogButtonBox::Apply)->setText(obj->btnApply);
		box->button(QDialogButtonBox::Apply)->setIcon(QIcon(":/icons/tick.png"));
	}
	if (box->standardButtons()&QDialogButtonBox::Save)
	{
		box->button(QDialogButtonBox::Save)->setText(obj->btnSave);
		box->button(QDialogButtonBox::Save)->setIcon(QIcon(":/icons/tick.png"));
	}
	delete obj;
}

QString formatSize(quint64 sz, QString format)
{
    QString pt;
    float   out = 0.0f;

	if (sz < 1024L)
    {
        pt  = QObject::tr("б");
        out = sz;
	} else if (sz < 1024L*1024L) {
        pt  = QObject::tr("Кб");
		out = (long double)sz/(long double)(1024L);
	} else if (sz < 1024L*1024L*1024L) {
        pt  = QObject::tr("Мб");
		out = (long double)sz/(long double)(1024L*1024L);
    } else {
        pt  = QObject::tr("Гб");
		out = (long double)sz/(long double)(1024L*1024L*1024L);
    }
    
    QString ret = QString().sprintf(format.toStdString().c_str(),out);
    ret = ret.replace("{S}", pt);
    return  ret;    
}

QString md5(QString in)
{
	QCryptographicHash hash(QCryptographicHash::Md5);
	hash.addData(in.toStdString().c_str());
	return QString(hash.result().toHex());
}

int levenshteinDistance(const QString& s1, const QString& s2)
{
	const int len1 = s1.size();
	const int len2 = s2.size();

	QVector<int> col(len2 + 1);
	QVector<int> prevCol(len2 + 1);

	for (int i = 0; i < prevCol.size(); ++i)
		prevCol[i] = i;

	for (int i = 0; i < len1; ++i)
	{
		col[0] = i + 1;
		for (int j = 0; j < len2; ++j)
			col[j + 1] = qMin(qMin(col[j] + 1, prevCol[j + 1] + 1), prevCol[j] + (s1[i] == s2[j] ? 0 : 1));
		prevCol = col;
	}

	return prevCol[len2];
}

QString clearGroupTitle(QString title)
{
	QString out;
	title = title.replace(QRegExp("&[a-z0-9]+;"),"");
	for (int x=0; x<title.length(); x++)
	{
		QChar ch = title[x];
		if (ch.isLetterOrNumber())
			out.append(ch);
		else
			out.append(" ");
	}
	out = out.replace(QRegExp("\\s+")," ");
	out = out.trimmed();
	return out;
}

void setProxy(QNetworkAccessManager * mng)
{
	if (NULL == mng) return; // Fuck off

	QNetworkProxy proxy;
	proxy.setType((QNetworkProxy::ProxyType)mCore::instance()->settings->value("proxy/type",(int)QNetworkProxy::NoProxy).toInt());
	proxy.setHostName(mCore::instance()->settings->value("proxy/addr","").toString());
	proxy.setPort(mCore::instance()->settings->value("proxy/port",0).toInt());
	proxy.setUser(mCore::instance()->settings->value("proxy/login","").toString());
	proxy.setPassword(mCore::instance()->settings->value("proxy/passw","").toString());

	mng->setProxy(proxy);
}
