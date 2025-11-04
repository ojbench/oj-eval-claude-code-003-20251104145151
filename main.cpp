#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <cctype>

using namespace std;

struct Submission {
    string problem;
    string status;
    int time;

    Submission(string p, string s, int t) : problem(p), status(s), time(t) {}
};

struct ProblemStatus {
    int wrong_attempts;
    bool solved;
    int solve_time;
    int frozen_submissions;

    ProblemStatus() : wrong_attempts(0), solved(false), solve_time(0), frozen_submissions(0) {}
};

struct Team {
    string name;
    map<string, ProblemStatus> problems;
    vector<Submission> submissions;
    int solved_count;
    int total_penalty;
    vector<int> solve_times;

    Team() : name(""), solved_count(0), total_penalty(0) {}
    Team(string n) : name(n), solved_count(0), total_penalty(0) {}

    void update_status(const string& problem, const string& status, int time) {
        submissions.emplace_back(problem, status, time);

        if (problems[problem].solved) return;

        if (status == "Accepted") {
            problems[problem].solved = true;
            problems[problem].solve_time = time;
            solved_count++;
            total_penalty += problems[problem].wrong_attempts * 20 + time;
            solve_times.push_back(time);
            sort(solve_times.rbegin(), solve_times.rend());
        } else {
            problems[problem].wrong_attempts++;
        }
    }

    void freeze_problem(const string& problem) {
        if (!problems[problem].solved) {
            problems[problem].frozen_submissions = 0;
        }
    }

    void add_frozen_submission(const string& problem) {
        if (!problems[problem].solved) {
            problems[problem].frozen_submissions++;
        }
    }

    bool is_frozen(const string& problem) const {
        return problems.at(problem).frozen_submissions > 0;
    }
};

struct TeamRank {
    Team* team;
    int rank;

    TeamRank(Team* t, int r) : team(t), rank(r) {}
};

class ICPCManager {
private:
    map<string, Team> teams;
    vector<Team*> team_ptrs;
    bool competition_started;
    bool frozen;
    int duration_time;
    int problem_count;
    vector<string> problem_names;
    vector<Submission> all_submissions;

public:
    ICPCManager() : competition_started(false), frozen(false), duration_time(0), problem_count(0) {}

    void add_team(const string& team_name) {
        if (competition_started) {
            cout << "[Error]Add failed: competition has started." << endl;
            return;
        }

        if (teams.find(team_name) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name." << endl;
            return;
        }

        teams.emplace(team_name, team_name);
        team_ptrs.push_back(&teams[team_name]);
        cout << "[Info]Add successfully." << endl;
    }

    void start_competition(int duration, int problems) {
        if (competition_started) {
            cout << "[Error]Start failed: competition has started." << endl;
            return;
        }

        competition_started = true;
        duration_time = duration;
        problem_count = problems;

        problem_names.clear();
        for (int i = 0; i < problems; i++) {
            problem_names.push_back(string(1, 'A' + i));
        }

        cout << "[Info]Competition starts." << endl;
    }

    void submit(const string& problem, const string& team_name, const string& status, int time) {
        if (!competition_started) return;

        teams[team_name].update_status(problem, status, time);

        if (frozen) {
            teams[team_name].add_frozen_submission(problem);
        }
    }

    void flush_scoreboard() {
        if (!competition_started) return;

        vector<Team*> ranked_teams = team_ptrs;
        sort(ranked_teams.begin(), ranked_teams.end(), [this](Team* a, Team* b) {
            return compare_teams(a, b);
        });

        cout << "[Info]Flush scoreboard." << endl;
        print_scoreboard(ranked_teams);
    }

    void freeze_scoreboard() {
        if (!competition_started) return;

        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen." << endl;
            return;
        }

        frozen = true;
        cout << "[Info]Freeze scoreboard." << endl;
    }

    void scroll_scoreboard() {
        if (!competition_started) return;

        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen." << endl;
            return;
        }

        cout << "[Info]Scroll scoreboard." << endl;

        vector<Team*> before_teams = team_ptrs;
        sort(before_teams.begin(), before_teams.end(), [this](Team* a, Team* b) {
            return compare_teams(a, b);
        });

        print_scoreboard(before_teams);

        frozen = false;

        for (auto& team_pair : teams) {
            for (const auto& problem_name : problem_names) {
                team_pair.second.freeze_problem(problem_name);
            }
        }

        vector<Team*> after_teams = team_ptrs;
        sort(after_teams.begin(), after_teams.end(), [this](Team* a, Team* b) {
            return compare_teams(a, b);
        });

        for (size_t i = 0; i < before_teams.size(); i++) {
            if (before_teams[i] != after_teams[i]) {
                cout << before_teams[i]->name << " " << after_teams[i]->name << " " << before_teams[i]->solved_count << " " << before_teams[i]->total_penalty << endl;
            }
        }

        print_scoreboard(after_teams);
    }

    void query_ranking(const string& team_name) {
        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team." << endl;
            return;
        }

        vector<Team*> ranked_teams = team_ptrs;
        sort(ranked_teams.begin(), ranked_teams.end(), [this](Team* a, Team* b) {
            return compare_teams(a, b);
        });

        int rank = 1;
        for (size_t i = 0; i < ranked_teams.size(); i++) {
            if (ranked_teams[i]->name == team_name) {
                rank = i + 1;
                break;
            }
        }

        cout << "[Info]Complete query ranking." << endl;
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled." << endl;
        }
        cout << team_name << " NOW AT RANKING " << rank << endl;
    }

    void query_submission(const string& team_name, const string& problem, const string& status) {
        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team." << endl;
            return;
        }

        cout << "[Info]Complete query submission." << endl;

        const auto& team = teams[team_name];
        const auto& submissions = team.submissions;

        for (auto it = submissions.rbegin(); it != submissions.rend(); ++it) {
            bool problem_match = (problem == "ALL" || it->problem == problem);
            bool status_match = (status == "ALL" || it->status == status);

            if (problem_match && status_match) {
                cout << team_name << " " << it->problem << " " << it->status << " " << it->time << endl;
                return;
            }
        }

        cout << "Cannot find any submission." << endl;
    }

    void end_competition() {
        cout << "[Info]Competition ends." << endl;
    }

private:
    bool compare_teams(Team* a, Team* b) {
        if (a->solved_count != b->solved_count) {
            return a->solved_count > b->solved_count;
        }

        if (a->total_penalty != b->total_penalty) {
            return a->total_penalty < b->total_penalty;
        }

        for (size_t i = 0; i < min(a->solve_times.size(), b->solve_times.size()); i++) {
            if (a->solve_times[i] != b->solve_times[i]) {
                return a->solve_times[i] < b->solve_times[i];
            }
        }

        return a->name < b->name;
    }

    void print_scoreboard(const vector<Team*>& ranked_teams) {
        for (size_t i = 0; i < ranked_teams.size(); i++) {
            Team* team = ranked_teams[i];
            cout << team->name << " " << (i + 1) << " " << team->solved_count << " " << team->total_penalty;

            for (const auto& problem_name : problem_names) {
                const auto& problem = team->problems[problem_name];
                cout << " ";

                if (problem.solved) {
                    if (problem.wrong_attempts == 0) {
                        cout << "+";
                    } else {
                        cout << "+" << problem.wrong_attempts;
                    }
                } else if (frozen && problem.frozen_submissions > 0) {
                    if (problem.wrong_attempts == 0) {
                        cout << "0/" << problem.frozen_submissions;
                    } else {
                        cout << "-" << problem.wrong_attempts << "/" << problem.frozen_submissions;
                    }
                } else {
                    if (problem.wrong_attempts == 0) {
                        cout << ".";
                    } else {
                        cout << "-" << problem.wrong_attempts;
                    }
                }
            }

            cout << endl;
        }
    }
};

int main() {
    ICPCManager manager;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;

        istringstream iss(line);
        string command;
        iss >> command;

        if (command == "ADDTEAM") {
            string team_name;
            iss >> team_name;
            manager.add_team(team_name);
        } else if (command == "START") {
            string dummy;
            int duration, problems;
            iss >> dummy >> duration >> dummy >> problems;
            manager.start_competition(duration, problems);
        } else if (command == "SUBMIT") {
            string problem, dummy1, team_name, dummy2, status, dummy3;
            int time;
            iss >> problem >> dummy1 >> team_name >> dummy2 >> status >> dummy3 >> time;
            manager.submit(problem, team_name, status, time);
        } else if (command == "FLUSH") {
            manager.flush_scoreboard();
        } else if (command == "FREEZE") {
            manager.freeze_scoreboard();
        } else if (command == "SCROLL") {
            manager.scroll_scoreboard();
        } else if (command == "QUERY_RANKING") {
            string team_name;
            iss >> team_name;
            manager.query_ranking(team_name);
        } else if (command == "QUERY_SUBMISSION") {
            string team_name, dummy1, problem_part, dummy2, status_part;
            iss >> team_name >> dummy1 >> problem_part >> dummy2 >> status_part;

            // Parse PROBLEM=xxx and STATUS=xxx format
            string problem = problem_part.substr(8); // Remove "PROBLEM=" prefix
            string status = status_part.substr(7);   // Remove "STATUS=" prefix

            manager.query_submission(team_name, problem, status);
        } else if (command == "END") {
            manager.end_competition();
            break;
        }
    }

    return 0;
}