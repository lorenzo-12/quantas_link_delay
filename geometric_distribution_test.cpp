

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

void test(double l){
    map<double,int> res;
    for(int i=0; i<10000; i++){
        int x = getDelay(l);
        res[x]++;
    }
    
    for(auto v : res){
        double p = ((double)v.second / 10000) * 100;
        cout << v.first << " " << v.second << " " << p << "%" << endl;
    }
}

int main() {
    
    double l;
    for (int i=1; i<=20; i++){
        l = i*0.05;
        cout << "---------- " << l << " ----------" << endl;
        test(l);
        cout << "------------------------------" << endl << endl;
    }

    int delay = getDelay(l);

    cout << "Sampled delay: " << delay << '\n';
    return 0;
}



