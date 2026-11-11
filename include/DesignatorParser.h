#pragma once
#include <QString>
#include <QVector>
#include <QPair>

// Parses and groups designator lists like "C1, C2, C3, C4, C6" -> "C1-C4, C6"
// Consecutive numbers with same prefix are collapsed to range.
// Result is sorted naturally (C1 before C2 before C10).
class DesignatorParser {
public:
    // Input: raw designator string from Excel (comma-separated)
    // Output: grouped and sorted string
    static QString groupAndSort(const QString &raw);

    // Natural sort key for a designator string (for sorting rows)
    // Returns pair: prefix (uppercase), first number (or 0)
    static QPair<QString, int> sortKey(const QString &grouped);
};
