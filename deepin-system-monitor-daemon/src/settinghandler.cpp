// SPDX-FileCopyrightText: 2011 ~ 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settinghandler.h"
#include "ddlog.h"
#include <DSettingsOption>

#include <QFile>
#include <QDebug>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QVariant>
#include <QString>

using namespace DDLog;

// 警报数据，由产品定义
const static int MinAlarmCpuUsage = 30;
const static int MaxAlarmCpuUsage = 100;
const static int MinAlarmMemUsage = 30;
const static int MaxAlarmMemUsage = 100;
const static int MinAlarmInterval = 5;
const static int MaxAlarmInterval = 60;

SettingHandler::SettingHandler(QObject *parent)
    : QObject(parent), mSettings(nullptr), mBackend(nullptr)
{
    QString orgName = qApp->organizationName();
    if (orgName.isEmpty()) {
        orgName = "deepin";
        qCInfo(app) << "Using default organization name: deepin";
    }

    //配置文件路径: ~/.config/deepin/deepin-system-monitor/protection.conf
    QString strConfigPath = QString("%1/%2/%3/protection.conf")
                                    .arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
                                    .arg(orgName)
                                    .arg("deepin-system-monitor");

    qCDebug(app) << "Loading settings from:" << strConfigPath;
    mBackend = new Dtk::Core::QSettingBackend(strConfigPath);
    mSettings = DSettings::fromJsonFile(":/resources/settings.json");

    auto opt = mSettings->option(AlarmCpuUsageOptionName);
    // 初始化有效值范围
    mValueRange[AlarmCpuUsageOptionName] = QPair<double, double>(MinAlarmCpuUsage, MaxAlarmCpuUsage);
    mValueRange[AlarmMemUsageOptionName] = QPair<double, double>(MinAlarmMemUsage, MaxAlarmMemUsage);
    mValueRange[AlarmIntervalOptionName] = QPair<double, double>(MinAlarmInterval, MaxAlarmInterval);

    auto keys = mValueRange.keys();

    foreach (auto it, keys) {
        qCDebug(app) << "Key:" << it << "Range:" << mValueRange[it];
    }

    if (mSettings != nullptr) {
        mSettings->setBackend(mBackend);
        qCInfo(app) << "Settings handler initialized successfully";
    }
}

SettingHandler::~SettingHandler()
{
    qCDebug(app) << "Cleaning up settings handler";
    if (mSettings != nullptr) {
        mSettings->deleteLater();
    }

    if (mBackend != nullptr) {
        mBackend->deleteLater();
    }
}

bool SettingHandler::isCompelted()
{
    return (mBackend != nullptr && mSettings != nullptr);
}

QVariant SettingHandler::getOptionValue(const QString key)
{
    if (isCompelted() && mSettings->keys().contains(key)) {
        return mSettings->getOption(key);
    } else {
        qCWarning(app) << "Failed to get option value for" << key
                       << "Completion status:" << isCompelted()
                       << "Available keys:" << mSettings->keys();
    }

    return QVariant();
}

bool SettingHandler::changedOptionValue(const QString key, const QVariant value)
{
    if (isCompelted() && mSettings->keys().contains(key)) {
        auto opt = mSettings->option(key);
        opt->setValue(value);
        mSettings->sync();
        qCInfo(app) << "Successfully changed option" << key << "to" << value;
        return true;
    } else {
        qCWarning(app) << "Failed to change option" << key << "to" << value
                       << "Completion status:" << isCompelted()
                       << "Available keys:" << mSettings->keys();
    }

    return false;
}

bool SettingHandler::isVaildValue(const QString key, const QVariant value)
{
    if (mValueRange.contains(key) && value.type() >= QVariant::Bool && value.type() <= QVariant::Double) {
        QPair<double, double> range = mValueRange[key];
        double valueD = value.toDouble();
        bool valid = (valueD >= range.first && valueD <= range.second);
        qCDebug(app) << "Value validation for" << key << ":" << valueD 
                     << "Range:" << range << "Valid:" << valid;
        return valid;
    }
    return true;
}

QPair<double, double> SettingHandler::getValueRange(const QString key)
{
    QPair<double, double> range(0, 0);
    if (mValueRange.contains(key)) {
        range = mValueRange[key];
        qCDebug(app) << "Retrieved value range for" << key << ":" << range;
    } else {
        qCDebug(app) << "No value range defined for" << key;
    }
    return range;
}

QList<QString> SettingHandler::itemKeys()
{
    return mSettings->keys();
}
