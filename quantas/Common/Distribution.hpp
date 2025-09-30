/*
Copyright 2022

This file is part of QUANTAS.
QUANTAS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
QUANTAS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with QUANTAS. If not, see <https://www.gnu.org/licenses/>.
*/

// This class handles the distribution of channel delays in the network. The distribution can be uniform, Poisson or one. 


#ifndef Distribution_hpp
#define Distribution_hpp

#include <string>
#include <random>
#include <iostream>
#include <thread>
#include <map>
#include "Json.hpp"

using namespace std;

namespace quantas{

    using std::string;
    using std::uniform_int_distribution;
    using std::poisson_distribution;
    using std::default_random_engine;
    using nlohmann::json;
    using std::cerr;

    // random number generator that will be created and seeded uniquely once in
    // each thread
    extern thread_local default_random_engine RANDOM_GENERATOR;

    // convenience function for using the random number generator to get a
    // random int in the range [min, max]
    int uniformInt(const int min, const int max);

    // convenience function for using the random number generator to get a
    // random int in the range [0, exclusiveMax) (like calling rand() %
    // exclusiveMax, but thread-safe)
    int randMod(const int exclusiveMax);

    // returns true with odds of 1 in x (e.g. 1 in 100)
    bool oneInXChance(const int x);

    // returns true with probability p (e.g. if p==0.5 it will return true half
    // of the time)
    bool trueWithProbability(const double p);

    static const string                POISSON = "POISSON";
    static const string                UNIFORM = "UNIFORM";
    static const string                ONE     = "ONE";
    static const string                SPECIFIC = "SPECIFIC";
    static const string                GEOMETRIC = "GEOMETRIC";
    class Distribution {
    private:
        int                                 _avgDelay = 1;
        int                                 _maxDelay = 1;
        int                                 _minDelay = 1;
        string                              _type = UNIFORM;
        map<long,map<long,double>>          _global_delays;
        double                              _global = 1;

    public:
        Distribution                                                 () {
            _avgDelay = 1;
            _maxDelay = 1;
            _minDelay = 1;
            _global   = 1;
            _type = UNIFORM;
        }

        Distribution                                                 (const Distribution&);
        ~Distribution                                                ();

        // setters
        
        void                                setDistribution     (json distribution);

        // getters
        int                                 maxDelay            ()const                                         {return _maxDelay;};
        int                                 avgDelay            ()const                                         {return _avgDelay;};
        int                                 minDelay            ()const                                         {return _minDelay;};
        string                              type                ()const                                         {return _type;};
        int                                 getDelay(long s, long d);
        double                              getGlobalDelay      (long s, long d);

    };

    inline Distribution::Distribution(const Distribution &rhs){
        if(this == &rhs){
            return;
        }

        _avgDelay = rhs._avgDelay;
        _maxDelay = rhs._maxDelay;
        _minDelay = rhs._minDelay;
        _global   = rhs._global;
        _type = rhs._type;
        _global_delays = rhs._global_delays;
    }

    inline Distribution::~Distribution(){
        
    }

    inline void Distribution::setDistribution(json distribution) {
        if (distribution.contains("avgDelay")) {
            _avgDelay = distribution["avgDelay"];
        }
        if (distribution.contains("maxDelay")) {
            _maxDelay = distribution["maxDelay"];
        }
        if (distribution.contains("minDelay")) {
            _minDelay = distribution["minDelay"];
        }
        if (distribution.contains("global")){
            _global = distribution["global"];
        }

        if (distribution.contains("type")) {
            string type = distribution["type"];
            if (type == UNIFORM) {
                _type = UNIFORM;
            }
            else if (type == POISSON) {
                _type = POISSON;
            }
            else if (type == ONE) {
                _type = ONE;
            }
            else if (type == SPECIFIC){
                //cout << "[Distribution] Setting SPECIFIC distribution" << endl;
                _type = SPECIFIC;

                map<long,map<long,double>> dict;
                if (distribution.contains("global_delays")){
                    for (auto& [i,val] : distribution["global_delays"].items()){
                        for (auto& [j,delay] : val.items()){
                            dict[stol(i)][stol(j)] = delay;
                        }
                    }
                }
                _global_delays = dict;
            }
            else if (type == GEOMETRIC){
                //cout << "[Distribution] Setting GEOMETRIC distribution" << endl;
                _type = GEOMETRIC;
                
                map<long,map<long,double>> dict;
                if (distribution.contains("global_delays")){
                    for (auto& [i,val] : distribution["global_delays"].items()){
                        for (auto& [j,delay] : val.items()){
                            dict[stol(i)][stol(j)] = delay;
                        }
                    }
                }
                _global_delays = dict;
            }
        }
    }

    inline double Distribution::getGlobalDelay(long s, long d) {
        // set the default delay that will be used in case the map does not contain the link (source, destination)
        double default_delay = _global;

        // guard against default_delay being out of bounds
        if (default_delay > _maxDelay) {
            default_delay = static_cast<double>(_maxDelay);
        }
        if (default_delay < _minDelay) {
            default_delay = static_cast<double>(_minDelay);
        }

        // if map is empy return 
        if (_global_delays.empty()) {
            return default_delay;
        }

        // if the source is not in the map return 
        auto itSender = _global_delays.find(s);
        if (itSender == _global_delays.end()) {
            return default_delay;  // source not found
        }

        // if the pair (source,destination) is not in the map return 
        auto itDestination = itSender->second.find(d);
        if (itDestination == itSender->second.end()) {
            return default_delay;  // destination not found
        }
    
        // return the value associated to _global_delays[source][destination]
        if ((itDestination->second <= _maxDelay) && (itDestination->second >= _minDelay)) return itDestination->second;
        else return default_delay; // guard against out of bounds values
    }
    
    inline int Distribution::getDelay(long src, long dest){
        int delay = -1;
        do {
            if (_type == UNIFORM) {
                delay = uniformInt(_minDelay, _maxDelay);
            }
            if (_type == POISSON) {
                poisson_distribution<int> poissonDistribution(_avgDelay);
                delay = poissonDistribution(RANDOM_GENERATOR);
            }
            if (_type == ONE) {
                delay = 1;
            }
            if (_type == SPECIFIC){
                double link_delay = getGlobalDelay(src, dest);
                delay = static_cast<int>(link_delay);
                return delay; // prevent from infinite loop if maxDelay is lower than delay
            }
            if (_type == GEOMETRIC){
                double lambda = getGlobalDelay(src, dest);
                const double p = 1.0 - exp(-lambda);
                geometric_distribution<int> geom(p); 
                delay = 1 + geom(RANDOM_GENERATOR);
                return delay; // prevent from infinite loop if maxDelay is lower than delay
            }
            // guard against 0 and negative numbers
        } while (delay < 1 || delay > _maxDelay || delay < _minDelay);

        return delay;
    }
}
#endif /* Distribution_hpp */
