#pragma once
#include <QString>
#include <QVector>
#include <QPair>

// Parses and groups designator lists like "C1, C2, C3, C4, C6" -> "C1-C4, C6"
// Consecutive numbers with same prefix are collapsed to range.
// Two consecutive elements (C1, C2) stay as "C1, C2" (per requirement).
// Result is sorted naturally (C1 before C2 before C10).
class DesignatorParser {
public:
    // Input: raw designator string from Excel (comma / space / semicolon separated)
    // Output: grouped and sorted string
    static QString groupAndSort(const QString &raw);

    // Extract all individual designators (C1, R12, ...) from an arbitrary string
    // ("C1, C2", "C1-C4", "С1 С2" ...). Latin prefixes are normalized to upper.
    static QStringList extractTokens(const QString &raw);

    // Split one designator token into prefix + number ("R12" -> {"R", 12}).
    // Returns false if the token has no trailing number.
    static bool splitToken(const QString &token, QString *prefix, int *number);

    // Natural sort key for a designator string (for sorting rows):
    // prefix of the smallest element + that number. Empty -> last.
    static QPair<QString, int> sortKey(const QString &grouped);

    // Check that numbering per prefix goes strictly in order starting from 1
    // (C1, C2, C3 ... without gaps/duplicates). Returns true if OK, otherwise
    // fills *problem with a human-readable description.
    static bool checkSequentialFromOne(const QStringList &tokens, QString *problem = nullptr);
};
