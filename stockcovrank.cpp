//
// Created by shuhao on 7/19/19.
//

#include "stockcovrank.h"
#include <sys/types.h>
#include <dirent.h>
#include <string>
#include <fstream>
#include <vector>
#include <random>
#include <algorithm>
#include <iterator>
#include <bits/stdc++.h>
#include <iostream>
#include <armadillo>

using namespace std;

vector<string> splitString(string input, char delimiter) {
    vector<string> tokens;
    stringstream check1(input);
    string intermediate;
    while(getline(check1, intermediate, delimiter)){
        tokens.push_back(intermediate);
    }
    return tokens;

}

int compare_date(string date1, string date2) {
    vector<string> date1parts = splitString(date1, '-');
    vector<string> date2parts = splitString(date2, '-');
    int year1 = stoi(date1parts[0]);
    int year2 = stoi(date2parts[0]);
    if (year1 < year2) {
        return -1;
    }
    else if (year1 > year2){
        return 1;
    }
    int mon1 = stoi(date1parts[1]);
    int mon2 = stoi(date2parts[1]);
    if (mon1 < mon2) {
        return -1;
    }
    else if (mon1 > mon2){
        return 1;
    }
    int day1 = stoi(date1parts[2]);
    int day2 = stoi(date2parts[2]);
    if (day1 < day2) {
        return -1;
    }
    else if (day1 > day2){
        return 1;
    }
    return 0;
}

double mean(const vector<double> &vec1) {
    double sum = 0;
    for (double v : vec1) {
        sum += v;
    }
    return sum/vec1.size();
}

double variance(const vector<double> &vec1) {
    double m = mean(vec1);
    double sum = 0;
    for (double v : vec1) {
        double diff = v - m;
        sum += diff * diff;
    }
    return sum / (vec1.size() - 1);
}

double covariance(const vector<double> &vec1, const vector<double> &vec2) {
    if (vec1.size() != vec2.size()) {
        fprintf(stderr, "Two variables have different sample size when calculating covariance.");
        return -1;
    }
    double m1 = mean(vec1);
    double m2 = mean(vec2);
    double sum = 0;
    for (int i = 0; i < vec1.size(); i++) {
        sum += (vec1[i] - m1) * (vec2[i] - m2);
    }
    return sum / (vec1.size() - 1);
}

int matrix_rank(vector<vector<double>> cov_mat, double tol) {
    //Use armadillo library to do linear algebra calculation
    int dim = cov_mat.size();
    arma::mat m(dim, dim);
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
            m(i, j) = cov_mat[i][j];
        }
    }

    auto r = arma::rank(m, tol);
    return r;
}

stockrepo::stockrepo() {
    this->repo_path = "";
    this->sample_size = 0;
    this->start_date = "";
    this->end_date = "";
}

bool stockrepo::update_repo(string repo) {
    if (repo == "") {
        fprintf(stderr, "Empty repo path");
        return false;
    }
    //get all filenames in the specified repo path
    vector<string> files;
    DIR* dirp = opendir(repo.c_str());
    struct dirent * dp;
    while ((dp = readdir(dirp)) != NULL) {
        string filename = dp->d_name;
        if (filename == "." or filename == "..") {
            continue;
        }
        files.push_back(filename);
    }
    closedir(dirp);

    this->repo_files = move(files);
    this->repo_path = repo;

    /* for test purpose
    for (auto i = repo_files.begin(); i != repo_files.end(); i++) {
        printf("%s\n", (repo+"/"+(*i)).c_str());
    }*/
    return true;
}

bool stockrepo::update_samplesize(int s) {
    if (s < 1) {
        fprintf(stderr,"Fail to update sample size: size should be positive int\n");
        return false;
    }
    if (s > this->get_repo_size()){
        fprintf(stderr,"Fail to update sample size: sample size should be smaller than repo size\n");
        return false;
    }
    this->sample_size = s;
    return true;
}

void stockrepo::update_sampledate(string start, string end) {
    this->start_date = start;
    this->end_date = end;
}

int stockrepo::get_repo_size() {
    return repo_files.size();
}

int stockrepo::get_rank(double tol) {
    if (this->sample_size == 0 || this->repo_path == ""
    || this->start_date == "" || this->end_date == ""){
        fprintf(stderr, "Please update repo, sample size and dates before get rank");
        return -1;
    }
    if (this->cache_sample()) {
        return this->calculate_rank(tol);
    }
    else {
        fprintf(stderr, "Fail to cache sample\n");
        return -1;
    }
}

bool stockrepo::cache_sample() {
    //sample the files
    vector<string> files_sample;
    sample(this->repo_files.begin(), this->repo_files.end(), back_inserter(files_sample),
            this->sample_size, mt19937{random_device{}()});
    //process sample one file by one file
    vector<pair<string, vector<pair<string, double>>>> total_cache;
    for (auto filep = files_sample.begin(); filep != files_sample.end(); filep++) {
        string filepath = this->repo_path + "/" + (*filep);
        ifstream fs(filepath);
        string line = "";
        getline(fs,line); //read first line
        vector<pair<string, double>> file_cache;
        while (getline(fs, line)){
            vector<string> line_data = splitString(line, ',');
            string date = line_data[0];
            //select rows
            if (compare_date(this->start_date, date) > 0) {
                continue;
            } else if (compare_date(this->end_date, date) < 0){
                break;
            }
            //select date and open columns
            file_cache.push_back(make_pair(date, stod(line_data[1])));
        }
        total_cache.push_back(make_pair(filepath, file_cache));
    }
    this->sample_cache = move(total_cache);
    //this->print_sample();
    return true;
}


int stockrepo::calculate_rank(double tol) {
    int dim = sample_cache.size();
    vector<vector<double>> cov_mat(dim, vector<double>(dim, 0));
    //prepare covariance matrix for further calculation
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++){
            auto iv = this->sample_cache[i].second;
            if (i == j) { //calculate variance beforehand if i == j
                vector<double> vec1;
                for (auto val : iv) {
                    vec1.push_back(val.second);
                }
                cov_mat[i][j] = variance(vec1);
                continue;
            }
            auto jv = this->sample_cache[j].second;
            //find intersection of dates to make sample size same
            auto ip = iv.begin();
            auto jp = jv.begin();
            vector<double> vec_x, vec_y; // store the intersection price
            while (ip != iv.end() && jp != jv.end()){
                int cmp_result = compare_date((*ip).first, (*jp).first);
                if (cmp_result == 0) {
                    vec_x.push_back((*ip).second);
                    vec_y.push_back((*jp).second);
                    ip++;
                    jp++;
                } else if (cmp_result > 0) {
                    jp++;
                } else if (cmp_result < 0) {
                    ip++;
                }
            }

            //print out vec_x, for debug purpose
            /*
            cout << i << ","<< j << endl;
            for (double v : vec_x) {
                cout<<v<<endl;

            }
            cout<<endl;
            */

            //calculate covariance
            cov_mat[i][j] = covariance(vec_x, vec_y);
        }
    }

    //print out the covairance matrix, for debug purpose
    for (int i = 0; i < dim; i++) {
        cout << this->sample_cache[i].first << ": ";
        for (int j = 0; j < dim; j++) {
            cout << cov_mat[i][j] << ",";
        }
        cout << endl;
    }

    //calculate the rank from cov_mat
    return matrix_rank(move(cov_mat), tol);
}

void stockrepo::print_sample() {
    for (auto i = this->sample_cache.begin(); i != this->sample_cache.end(); i++){
        auto iv = *i;
        string fname = iv.first;
        auto table = iv.second;
        cout << fname << endl;
        for (auto j = table.begin(); j != table.end(); j++) {
            auto jv = *j;
            string date = jv.first;
            double price = jv.second;
            cout << date << "," << price << endl;
        }
        cout << endl;
    }
}