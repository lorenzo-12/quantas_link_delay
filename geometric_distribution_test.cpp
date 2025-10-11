

#include <iostream>
#include <random>
#include <map>

using namespace std;

// one global generator for the whole program
static std::mt19937 RANDOM_GENERATOR{std::random_device{}()};

int getDelay(double lambda) {
    // compute parameter for geometric distribution
    const double p = 1.0 - std::exp(-lambda);
    std::geometric_distribution<int> geom(p);

    // sample a delay (add +1 so smallest value is 1)
    return 1 + geom(RANDOM_GENERATOR);
}

int test(double l, map<int,string> m){
    map<string,int> res;
    int sum_val = 0;
    for(int i=0; i<100000; i++){
        int x = getDelay(l);
        sum_val+=x;
        string range_val = m[x];
        res[range_val]++;
    }
    
    vector<string> val = {"1","2","3","4-5","6-10","11-15","16-20","21-25"};
    //for(auto v : res){
    //    double p = ((double)v.second / 100000) * 100;
    //    cout << v.first << " " << v.second << " " << p << "%" << endl;
    //}
    double avg_val = (double)sum_val/100000;
    cout << "lambda = " << l <<  " --> avg_val: " << avg_val << endl;
    for(auto v : val){
        double p = ((double)res[v] / 100000) * 100;
        if (res[v] > 0){
            cout << v << " " << res[v] << " " << p << "%" << endl;
        }
        
    }
    return avg_val;
}

int main() {
    
    map<int,string> m;
    m[1] = "1";
    m[2] = "2";
    m[3] = "3";
    m[4] = "4-5";
    m[5] = "4-5";
    for (int i=6; i<40; i++){
        int val_lower = int(i/5)*5+1;
        int val_upper = val_lower+4;
        m[i] = to_string(val_lower)+"-"+to_string(val_upper);
    }
    
    double l;
    double sum_val = 0;
    for (int i=1; i<=40; i++){
        l = i*0.05;
        cout << "---------- " << l << " ----------" << endl;
        sum_val += test(l,m);
        cout << "------------------------------" << endl << endl;
    }

    
    return 0;
}



