#include "DesignatorParser.h"
#include <QRegularExpression>
#include <QMap>
#include <algorithm>

namespace {

struct TokenInfo { QString prefix; int number; };

// "C1-C4" style interval inside a single token
bool parseInterval(const QString &tok, QString *prefix, int *from, int *to) {
    static QRegularExpression re(R"(^([A-Za-zА-Яа-яЁё]+)(\d+)\s*[-–—~]\1?(\d+)$)");
    auto m = re.match(tok);
    if (!m.hasMatch()) return false;
    *prefix = m.captured(1).toUpper();
    *from = m.captured(2).toInt();
    *to = m.captured(3).toInt();
    return true;
}

} // namespace

QStringList DesignatorParser::extractTokens(const QString &raw) {
    QStringList out;
    if (raw.trimmed().isEmpty()) return out;

    // separators: comma, semicolon, slash or whitespace
    QStringList parts = raw.split(QRegularExpression("[,;/\\s]+"), Qt::SkipEmptyParts);
    static QRegularExpression simple(R"(^([A-Za-zА-Яа-яЁё]+)(\d+)$)");

    for (QString p : parts) {
        p = p.trimmed();
        if (p.isEmpty()) continue;

        QString pref;
        int a = 0, b = 0;
        if (parseInterval(p, &pref, &a, &b)) {
            if (a > b) std::swap(a, b);
            for (int n = a; n <= b; ++n)
                out << QString("%1%2").arg(pref).arg(n);
            continue;
        }

        auto m = simple.match(p);
        if (m.hasMatch()) {
            out << QString("%1%2").arg(m.captured(1).toUpper()).arg(m.captured(2).toInt());
            continue;
        }

        // Fallback: pull every embedded designator out of noise, e.g. "(R1, R2)"
        static QRegularExpression findRe(R"(([A-Za-z]+)(\d+))");
        auto it = findRe.globalMatch(p);
        bool any = false;
        while (it.hasNext()) {
            auto mm = it.next();
            out << QString("%1%2").arg(mm.captured(1).toUpper()).arg(mm.captured(2).toInt());
            any = true;
        }
        if (!any) out << p.toUpper(); // keep unknown tokens so nothing is lost
    }
    return out;
}

bool DesignatorParser::splitToken(const QString &token, QString *prefix, int *number) {
    static QRegularExpression re(R"(^([A-Za-z]+)(\d+)$)");
    auto m = re.match(token.trimmed());
    if (!m.hasMatch()) return false;
    if (prefix) *prefix = m.captured(1).toUpper();
    if (number) *number = m.captured(2).toInt();
    return true;
}

QString DesignatorParser::groupAndSort(const QString &raw) {
    const QStringList tokens = extractTokens(raw);
    if (tokens.isEmpty()) return QString();

    QMap<QString, QVector<int>> groups;   // prefix -> numbers
    QStringList plain;                     // tokens without a number
    for (const QString &t : tokens) {
        QString pref;
        int num = 0;
        if (splitToken(t, &pref, &num))
            groups[pref].append(num);
        else if (!plain.contains(t))
            plain << t;
    }

    QStringList result;
    for (auto it = groups.begin(); it != groups.end(); ++it) {
        QString prefix = it.key();
        QVector<int> nums = it.value();
        std::sort(nums.begin(), nums.end());
        nums.erase(std::unique(nums.begin(), nums.end()), nums.end());
        if (nums.isEmpty()) continue;

        int i = 0;
        while (i < nums.size()) {
            int start = nums[i];
            int end = start;
            while (i + 1 < nums.size() && nums[i + 1] == end + 1) {
                ++i;
                end = nums[i];
            }
            // 1 element -> "C1"; 2 -> "C1, C2" (not collapsed); 3+ -> "C1-C3"
            if (end > start + 1)
                result.append(QString("%1%2-%1%3").arg(prefix).arg(start).arg(end));
            else if (end == start + 1)
                result << QString("%1%2").arg(prefix).arg(start)
                       << QString("%1%2").arg(prefix).arg(end);
            else
                result.append(QString("%1%2").arg(prefix).arg(start));
            ++i;
        }
    }
    result.append(plain);
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

bool DesignatorParser::checkSequentialFromOne(const QStringList &tokens, QString *problem) {
    QMap<QString, QVector<int>> byPrefix;
    for (const QString &t : tokens) {
        QString pref;
        int num = 0;
        if (splitToken(t, &pref, &num))
            byPrefix[pref].append(num);
    }

    QStringList issues;
    for (auto it = byPrefix.begin(); it != byPrefix.end(); ++it) {
        QVector<int> nums = it.value();
        std::sort(nums.begin(), nums.end());
        nums.erase(std::unique(nums.begin(), nums.end()), nums.end());
        if (nums.isEmpty()) continue;
        if (nums.first() != 1) {
            issues << QString("%1: нумерация начинается с %2, ожидается %11")
                          .arg(it.key()).arg(nums.first());
            continue;
        }
        for (int i = 1; i < nums.size(); ++i) {
            if (nums[i] != nums[i - 1] + 1) {
                issues << QString("%1: пропуск после %1%2 (следующий %1%3)")
                              .arg(it.key()).arg(nums[i - 1]).arg(nums[i]);
                break;
            }
        }
    }

    if (!issues.isEmpty() && problem)
        *problem = issues.join("; ");
    return issues.isEmpty();
}
