#pragma once

#include <QString>
#include <QFile>
#include <QTextStream>

#include "dznode.h"

enum class LogLevel
{
	DTHDEBUG,
	DTHINFO,
	DTHWARNGING,
	DTHERROR
};

class DthLogger
{

public:

	DthLogger(QString exportDirectory, QString characterName, DzNode* selectedRootNode);
	~DthLogger();

	void log(LogLevel level, const QString& message);

private:

	QString levelToString(LogLevel level);
	QString getCurrentTime();

	QString exportDirectory_;
	QString characterName_;
	DzNode* selectedRootNode_;
	QFile logFile_;
	QTextStream textStream_;

};