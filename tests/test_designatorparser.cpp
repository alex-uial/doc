// Standalone console test for DesignatorParser (no Qt needed).
// Mirrors the logic in src/DesignatorParser.cpp.
#include <algorithm>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <string>
#include <vector>

using namespace std;

static vector<string> extractTokens(const string &raw) {
    vector<string> out;
    regex sep("[,;/\\s]+");
    regex interval(R"(^([A-Za-zА-Яа-яЁё]+)(\d+)\s*[-–—~][A-Za-zА-Яа-яЁё]*(\d+)$)");
    regex simple(R"(^([A-Za-zА-Яа-яЁё]+)(\d+)$)");
    regex findRe(R"(([A-Za-z]+)(\d+))");
    // split on separators
    vector<string> parts;
    string cur;
    auto isSep = [](char c){ return c==','||c==';'||c=='/'||c==' '||c=='\t'||c=='\n'; };
    for (char c : raw) { if (isSep(c)) { if(!cur.empty()) parts.push_back(cur); cur.clear(); } else cur+=c; }
    if (!cur.empty()) parts.push_back(cur);

    auto toUpper = [](string s){ for(auto&c:s) if(c>='a'&&c<='z') c-=32; return s; };

    for (auto p : parts) {
        smatch m;
        if (regex_match(p, m, interval)) {
            string pref = toUpper(m[1]);
            int a = stoi(m[2]), b = stoi(m[3]);
            if (a > b) swap(a,b);
            for (int n=a;n<=b;++n) out.push_back(pref+to_string(n));
            continue;
        }
        if (regex_match(p, m, simple)) { out.push_back(toUpper(m[1])+to_string(stoi(m[2]))); continue; }
        bool any=false;
        for (sregex_iterator it(p.begin(),p.end(),findRe), e; it!=e; ++it) {
            out.push_back(toUpper((*it)[1])+string((*it)[2])); any=true;
        }
        if(!any) out.push_back(toUpper(p));
    }
    return out;
}

static bool splitToken(const string &t, string *pref, int *num) {
    regex re(R"(^([A-Za-z]+)(\d+)$)");
    smatch m;
    if (!regex_match(t, m, re)) return false;
    string p = m[1]; for(auto&c:p) if(c>='a'&&c<='z') c-=32;
    *pref = p; *num = stoi(m[2]);
    return true;
}

static string groupAndSort(const string &raw) {
    auto tokens = extractTokens(raw);
    if (tokens.empty()) return "";
    map<string, vector<int>> groups;
    vector<string> plain;
    for (auto &t : tokens) {
        string p; int n;
        if (splitToken(t,&p,&n)) groups[p].push_back(n);
        else if (find(plain.begin(),plain.end(),t)==plain.end()) plain.push_back(t);
    }
    vector<string> result;
    for (auto &kv : groups) {
        auto &nums = kv.second;
        sort(nums.begin(),nums.end());
        nums.erase(unique(nums.begin(),nums.end()),nums.end());
        size_t i=0;
        while (i<nums.size()) {
            int start=nums[i], end=start;
            while (i+1<nums.size() && nums[i+1]==end+1) { ++i; end=nums[i]; }
            if (end > start+1) result.push_back(kv.first+to_string(start)+"-"+kv.first+to_string(end));
            else if (end == start+1) { result.push_back(kv.first+to_string(start)); result.push_back(kv.first+to_string(end)); }
            else result.push_back(kv.first+to_string(start));
            ++i;
        }
    }
    for (auto &p : plain) result.push_back(p);
    string out;
    for (size_t i=0;i<result.size();++i){ if(i) out+=", "; out+=result[i]; }
    return out;
}

static bool checkSequentialFromOne(const vector<string> &tokens, string *problem) {
    map<string,set<int>> byPrefix;
    for (auto &t : tokens){ string p; int n; if(splitToken(t,&p,&n)) byPrefix[p].insert(n); }
    vector<string> issues;
    for (auto &kv : byPrefix) {
        vector<int> nums(kv.second.begin(), kv.second.end());
        if (nums.empty()) continue;
        if (nums.front()!=1) { issues.push_back(kv.first+": starts at "+to_string(nums.front())); continue; }
        for (size_t i=1;i<nums.size();++i)
            if (nums[i]!=nums[i-1]+1){ issues.push_back(kv.first+": gap after "+to_string(nums[i-1])); break; }
    }
    if(problem){ problem->clear(); for(size_t i=0;i<issues.size();++i){ if(i)*problem+="; "; *problem+=issues[i]; } }
    return issues.empty();
}

int failures = 0;
void check(const string &got, const string &want, const string &name) {
    if (got != want) { cout << "FAIL " << name << ": got [" << got << "] want [" << want << "]\n"; ++failures; }
    else cout << "ok   " << name << "\n";
}

int main() {
    check(groupAndSort("C1, C2, C3, C4"), "C1-C4", "four consecutive -> range");
    check(groupAndSort("C1, C2"), "C1, C2", "two consecutive stay as-is");
    check(groupAndSort("C1, C2, C3"), "C1-C3", "three consecutive -> range");
    check(groupAndSort("C1, C2, C3, C4, C6"), "C1-C4, C6", "range + single");
    check(groupAndSort("C4, C1, C3, C2"), "C1-C4", "unsorted input -> sorted");
    check(groupAndSort("R2, R1"), "R1, R2", "pair reordered");
    check(groupAndSort("C10, C11, C12, C9"), "C9-C12", "numeric not lexicographic");
    check(groupAndSort("r1, r2, r3, C1"), "C1, R1-R3", "case normalized, prefixes grouped");
    check(groupAndSort("C1-C4, C7"), "C1-C4, C7", "interval input expanded and regrouped");
    check(groupAndSort(""), "", "empty");

    vector<string> all = extractTokens("C1, C2, C3, R1, R2");
    string prob;
    check(checkSequentialFromOne(all,&prob) ? "seq" : ("not:"+prob), "seq", "C1..C3,R1,R2 sequential");
    vector<string> bad = extractTokens("C1, C3");
    check(checkSequentialFromOne(bad,&prob) ? "seq" : ("not:"+prob), "not:C: gap after 1", "gap detected");
    vector<string> bad2 = extractTokens("C2, C3");
    check(checkSequentialFromOne(bad2,&prob) ? "seq" : ("not:"+prob), "not:C: starts at 2", "must start from 1");
    cout << (failures? "TESTS FAILED\n":"ALL TESTS PASSED\n");
    return failures?1:0;
}
