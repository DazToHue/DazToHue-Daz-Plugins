
#include <QDateTime>
#include <QDir>

#include "dth_logger.h"

#include "dznode.h"

#include "../version.h"

DthLogger::DthLogger(QString exportDirectory, QString characterName, DzNode* selectedRootNode) : exportDirectory_(exportDirectory), characterName_(characterName), selectedRootNode_(selectedRootNode)
{
	QString logFilePath = exportDirectory_ + "/" + characterName_ + ".log";

	logFile_.setFileName(logFilePath);

	if (!logFile_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
	{
		qWarning("Logger Error: Could not open custom plugin log file.");
	}
	else
	{
		textStream_.setDevice(&logFile_);
	}
}

void DthLogger::log(LogLevel level, const QString& message)
{
	if (logFile_.isOpen())
	{
		textStream_ << "[" << getCurrentTime() << "] "
			<< "[" << levelToString(level) << "] "
			<< message << "\n";

		textStream_.flush();
	}
}

DthLogger::~DthLogger()
{
	if (logFile_.isOpen())
	{
		textStream_.flush();
		logFile_.close();
	}
}

QString DthLogger::levelToString(LogLevel level)
{
	switch (level)
	{
	case LogLevel::DTHDEBUG:	return "DEBUG";
	case LogLevel::DTHINFO:		return "INFO";
	case LogLevel::DTHWARNGING:	return "WARNING";
	case LogLevel::DTHERROR:	return "ERROR";
	default:					return "UNKNOWN";
	}
}

QString DthLogger::getCurrentTime()
{
	return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
}