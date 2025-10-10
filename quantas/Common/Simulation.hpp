/*
Copyright 2022

This file is part of QUANTAS.
QUANTAS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
QUANTAS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with QUANTAS. If not, see <https://www.gnu.org/licenses/>.
*/
//
// This class handles reading in a configuration file, setting up log files for the simulation, 
// initializing the network class, and repeating a simulation according to the configuration file 
// (i.e., running multiple experiments with the same configuration).  It is templated with a user 
// defined message and peer class, used for the underlaying network instance. 

#ifndef Simulation_hpp
#define Simulation_hpp

#include <chrono>
#include <thread>
#include <fstream>
#include <any>
#include <string>

#include "Network.hpp"
#include "LogWriter.hpp"
#include "BS_thread_pool.hpp"


using std::ofstream;
using std::thread;

namespace quantas {

	class ResultCollector {
	public: 
		int n = 0;
		int f = 0;
		int c = 0;
		int p = 0;
		
		vector<double> delivery_nodes;
		vector<double> delivery_time;
		vector<double> disagreement;
		vector<double> disagreement_frequency;
		vector<double> termination_rate;
		

		void setParameters(int _n, int _f, int _p){
			n = _n;
			f = _f;
			c = n - f;
			p = _p;
			//cout << "n=" << n << ", f=" << f << ", c=" << c << ", p=" << p << endl;
		}
	
		void addResult(vector<int> final_values, vector<int> final_times){
			int counter_node_terminated = 0;
			int sum_delivery_time = 0;
			int counter_vote_0 = 0;
			int counter_vote_1 = 0;
			for (int i=0; i<final_values.size(); i++){
				int v = final_values[i];
				int t = final_times[i];
				if (v != -1) counter_node_terminated++;
				if (v == 0) counter_vote_0++;
				if (v == 1) counter_vote_1++;
				if (t != -1) sum_delivery_time += t;
			}
			
			if (sum_delivery_time==0 || counter_node_terminated==0){
				delivery_nodes.push_back(0);
				delivery_time.push_back(0);
				disagreement.push_back(0);
				termination_rate.push_back(0);
			}
			else{
				delivery_nodes.push_back((double)counter_node_terminated *100 / c);
				delivery_time.push_back((double)sum_delivery_time / c);
				termination_rate.push_back(1);

				double min_vote = min(counter_vote_0, counter_vote_1);
				double sum_vote = counter_vote_0 + counter_vote_1;
				double dis = min_vote *100 / sum_vote;
				disagreement.push_back(dis);
			}
			
		}

		vector<any> collectResults(){
			double sum_delivery = 0;
			double sum_delivery_time = 0;
			double sum_disagreement = 0;
			double avg_delivery = 0;
			double avg_delivery_time = 0;
			double avg_disagreement = 0;
			int termination_sum = 0;
			string termination_percentage;
			int counter = 0;
			int counter_disagreement = 0;
			for (int i=0; i<delivery_nodes.size(); i++){
				double x = delivery_nodes[i];
				double y = delivery_time[i];
				double z = disagreement[i];
				termination_sum += termination_rate[i];

				if (x!=0 && y != 0){
					sum_delivery += x;
					sum_delivery_time += y;
					counter++;
				}

				if (z!=0){
					sum_disagreement += z;
					counter_disagreement++;
				}
			}

			if (counter==0) return {0.0, 0.0, 0.0, 0.0, string("0% (0/0)")};

			avg_delivery = sum_delivery / counter;
			avg_delivery_time = sum_delivery_time / counter;
			double tp = (double)termination_sum *100 / termination_rate.size();
			termination_percentage = to_string(tp)+"% ("+to_string(termination_sum)+"/"+to_string(termination_rate.size())+")";
			if (counter_disagreement==0) avg_disagreement = 0;
			else avg_disagreement = sum_disagreement / counter_disagreement;
			return {avg_delivery, avg_delivery_time, avg_disagreement, (double)counter_disagreement, termination_percentage};
		}

		json getResults(){
			json results;
			vector<any> results_collected = collectResults();
			results["n"] = n;
			results["f"] = f;
			results["c"] = c;
			results["p"] = p;
			results["avg_delivery"] = std::any_cast<double>(results_collected[0]);
			results["avg_delivery_time"] = std::any_cast<double>(results_collected[1]);
			results["avg_disagreement"] = std::any_cast<double>(results_collected[2]);
			results["disagreement_frequency"] = std::any_cast<double>(results_collected[3]);
			results["termination_rate"] = std::any_cast<string>(results_collected[4]);
			return results;
		}

	};

	class SimWrapper {
	public:
    	virtual void run(json) = 0;
	};

	template<class type_msg, class peer_type>
    class Simulation : public SimWrapper{
    private:
        Network<type_msg, peer_type> 		system;
    public:
        // Name of log file, will have Test number appended
        void 				run			(json);
    };

	template<class type_msg, class peer_type>
	void Simulation<type_msg, peer_type>::run(json config) {
		ofstream out;
		if (config["logFile"] == "cout") {
			LogWriter::instance()->setLog(cout); // Set the log file to the console
		}
		else {
			string file = config["logFile"];
			out.open(file);
			if (out.fail()) {
				cout << "Error: could not open file " << file << ". Writing to console" << endl;
				LogWriter::instance()->setLog(cout); // If the file doesn't open set the log file to the console
			}
			else {
				LogWriter::instance()->setLog(out); // Otherwise set the log file to the user given file
			}
		}

		std::chrono::time_point<std::chrono::high_resolution_clock> startTime, endTime; // chrono time points
   		std::chrono::duration<double> duration; // chrono time interval
		startTime = std::chrono::high_resolution_clock::now();

		int _threadCount = thread::hardware_concurrency(); // By default, use as many hardware cores as possible
		if (config.contains("threadCount") && config["threadCount"] > 0) {
			_threadCount = config["threadCount"];
		}
		cout << "Using " << _threadCount << " threads." << endl;
		if (_threadCount > config["topology"]["totalPeers"]) {
			_threadCount = config["topology"]["totalPeers"];
		}
		int networkSize = static_cast<int>(config["topology"]["totalPeers"]);
		
		BS::thread_pool pool(_threadCount);
		ResultCollector rc;
		rc.setParameters(config["parameters"]["n"], config["parameters"]["f"], config["parameters"]["percentage"]);
		
		for (int i = 0; i < config["tests"]; i++) {
			LogWriter::instance()->setTest(i);

			// Configure the delay properties and initial topology of the network
			system.setDistribution(config["distribution"]);
			system.initNetwork(config["topology"], config["rounds"]);
			if (config.contains("parameters")) {
				system.initParameters(config["parameters"]);
			}
			
			//cout << "Test " << i + 1 << endl;
			for (int j = 0; j < config["rounds"]; j++) {
				//cout << "ROUND " << j << endl;
				LogWriter::instance()->setRound(j); // Set the round number for logging

				// do the receive phase of the round

				BS::multi_future<void> receive_loop = pool.parallelize_loop(networkSize, [this](int a, int b){system.receive(a, b);});
				receive_loop.wait();

				BS::multi_future<void> compute_loop = pool.parallelize_loop(networkSize, [this](int a, int b){system.performComputation(a, b);});
				compute_loop.wait();

				system.endOfRound(); // do any end of round computations

				BS::multi_future<void> transmit_loop = pool.parallelize_loop(networkSize, [this](int a, int b){system.transmit(a, b);});
				transmit_loop.wait();
			}

			
			vector<int> final_values;
			vector<int> final_times;
			for (auto const& p : system.peers()){
				auto bp = dynamic_cast<peer_type*>(p);
				final_values.push_back(bp->final_value);
				final_times.push_back(bp->finished_round);
			}
			rc.addResult(final_values, final_times);

			//cout << "Test " << i + 1 << " completed." << endl;

			ofstream status("status.txt", std::ios::app);
			if (!status) {
				std::cerr << "Error: could not open status.txt for writing\n";
			}
			if (status) {
				status << config["algorithm"] << endl;
				status.flush(); // ensure it hits disk promptly
			}


		}

		json results = rc.getResults();
		
		endTime = std::chrono::high_resolution_clock::now();
   		duration = endTime - startTime;
		LogWriter::instance()->data["RunTime"] = duration.count();
		LogWriter::instance()->data["Results"] = results;

		LogWriter::instance()->print();
		out.close();
	}

	
}

#endif /* Simulation_hpp */
