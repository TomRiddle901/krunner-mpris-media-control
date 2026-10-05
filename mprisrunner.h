#pragma once

#include <KRunner/AbstractRunner>

#include <QString>
#include <QStringList>

class MprisRunner : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    MprisRunner(QObject *parent, const KPluginMetaData &data);

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context,
             const KRunner::QueryMatch &match) override;
};