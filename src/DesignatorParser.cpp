#include "DesignatorParser.h"
#include <QRegularExpression>
#include <QMap>
#include <algorithm>

QString DesignatorParser::groupAndSort(const QString &raw) {
    if (raw.trimmed().isEmpty()) return QString();

    QStringList parts = raw.split(',', Qt::SkipEmptyParts);
    QMap<QString, QVector<int>> groups;

    QRegularExpression re(R"(^([A-Za-z]+)(\d+)$)");
    for (QString p : parts) {
        p = p.trimmed();
        auto m = re.match(p);
        if (m.hasMatch()) {
            QString prefix = m.captured(1).toUpper();
            int num = m.captured(2).toInt();
            groups[prefix].append(num);
        } else if (!p.isEmpty()) {
            groups[p.toUpper()].append(-1);
        }
    }

    QStringList result;
    for (auto it = groups.begin(); it != groups.end(); ++it) {
        QString prefix = it.key();
        QVector<int> nums = it.value();
        std::sort(nums.begin(), nums.end());
        nums.erase(std::unique(nums.begin(), nums.end()), nums.end());

        if (nums.isEmpty()) continue;
        if (nums.first() == -1) {
            result.append(prefix);
            continue;
        }

        int i = 0;
        while (i < nums.size()) {
            int start = nums[i];
            int end = start;
            while (i + 1 < nums.size() && nums[i + 1] == end + 1) {
                ++i;
                end = nums[i];
            }
            if (end > start)
                result.append(QString("%1%2-%1%3").arg(prefix).arg(start).arg(end));
            else
                result.append(QString("%1%2").arg(prefix).arg(start));
            ++i;
        }
    }
    return result.join(", ");
}

QPair<QString, int> DesignatorParser::sortKey(const QString &raw) {
    if (raw.trimmed().isEmpty()) return {QString(), 999999};

    QStringList parts = raw.split(',', Qt::SkipEmptyParts);
    QRegularExpression re(R"(^([A-Za-z]+)(\d+))");

    QString bestPrefix;
    int bestNum = 999999;
    bool found = false;

    for (QString p : parts) {
        p = p.trimmed();
        auto m = re.match(p);
        if (m.hasMatch()) {
            QString pref = m.captured(1).toUpper();
            int num = m.captured(2).toInt();
            if (!found || pref < bestPrefix || (pref == bestPrefix && num < bestNum)) {
                bestPrefix = pref;
                bestNum = num;
                found = true;
            }
        }
    }
    if (!found) return {raw.trimmed().toUpper(), 0};
    return {bestPrefix, bestNum};
}
