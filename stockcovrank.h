//
// Created by shuhao on 7/19/19.
//

#ifndef C___STOCKCOVRANK_H
#define C___STOCKCOVRANK_H

#include <string>
#include <vector>
using namespace std;

class stockrepo {
public:
    stockrepo ();
    bool update_repo (string repo);//update and fetch all the files from repo
    bool update_samplesize(int s);//update sample size
    void update_sampledate(string start, string end);
    int get_repo_size();
    int get_rank(double tol);

private:
    bool cache_sample(); //parse and cache files in a random sample
    int calculate_rank(double tol);//calculate the rank

    //parameters
    string repo_path;
    string start_date;
    string end_date;
    vector<string> repo_files;
    vector<pair<string, vector<pair<string, double>>>> sample_cache;
    int sample_size;

    //for test_purpose
    void print_sample();


};
#endif //C___STOCKCOVRANK_H
