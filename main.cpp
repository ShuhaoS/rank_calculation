#include <iostream>
#include "stockcovrank.h"
using namespace std;

int main() {
    stockrepo testrepo;
    string testpath = "/home/shuhao/Desktop/files/2/Calendar/ticker data/shanghai_equity_data_transformed";
    testrepo.update_repo(testpath);
    testrepo.update_samplesize(10);
    testrepo.update_sampledate("2019-06-05", "2019-06-30");
    for (int i = 0; i < 100; i++) {
        int r =testrepo.get_rank(0.01);
        cout << "Rank: " << r << endl << endl;
    }
    return 0;

}