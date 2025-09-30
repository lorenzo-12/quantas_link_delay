/*
Copyright 2022

This file is part of QUANTAS.
QUANTAS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
QUANTAS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with QUANTAS. If not, see <https://www.gnu.org/licenses/>.
*/

#include "BrachaPeer.hpp"
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace quantas {

	//
	// Example Channel definitions
	//
	BrachaPeer::~BrachaPeer() {

	}

	BrachaPeer::BrachaPeer(const BrachaPeer& rhs) : Peer<BrachaMessage>(rhs) {
		
	}

	BrachaPeer::BrachaPeer(long id) : Peer(id) {
		
	}

	int BrachaPeer::check_echo() {
		unordered_map<int, int> freq;
		for (const auto& m : echo_msgs) {
			int v = m.second;
			int c = ++freq[v];
			if (c >= echo_threshold) return v;
		}
		return -1;
	}

	int BrachaPeer::check_ready() {
		unordered_map<int, int> freq;
		for (const auto& m : ready_msgs) {
			int v = m.second;
			int c = ++freq[v];
			if (c >= ready_threshold) return v;
		}
		return -1;
	}

	int BrachaPeer::check_delivery() {
		unordered_map<int, int> freq;
		for (const auto& m : ready_msgs) {
			int v = m.second;
			int c = ++freq[v];
			if (c >= delivery_threshold) return v;
		}
		return -1;
	}

	void BrachaPeer::initParameters(const vector<Peer<BrachaMessage>*>& _peers, json parameters) {
		const vector<BrachaPeer*> peers = reinterpret_cast<vector<BrachaPeer*> const&>(_peers);
		
		int f = parameters["f"];
		int n = parameters["n"];
		sender = parameters["sender"];
		percentage = parameters["percentage"];

		is_byzantine = true;
		if (parameters["byzantine_nodes"][id()] == 0) is_byzantine = false;

		honest_nodes.clear();
		for (int i = 0; i<n; i++){
			if (parameters["byzantine_nodes"][i] == 0) honest_nodes.push_back(i);
		}
		
		echo_threshold = static_cast<int>(std::ceil((n + f + 1) / 2.0));
		ready_threshold = f + 1;
		delivery_threshold = 2 * f + 1;

		sent_echo = false;
		sent_ready = false;
		delivered = false;
		echo_msgs.clear();
		ready_msgs.clear();
		finished_round = -1;
		final_value = -1;

	}

	void BrachaPeer::performComputation() {

		if (delivered) {
			// already delivered, do nothing
			return;
		}

		if (is_byzantine && getRound() == 0 && id() == sender) {
			BrachaMessage m1;
			m1.source = id();
			m1.type = "send";
			m1.value = 0; 

			BrachaMessage m2;
			m2.source = id();
			m2.type = "send";
			m2.value = 1;

			byzantine_broadcast(m1, m2, percentage, honest_nodes);
		}

		if (is_byzantine) {
			// Byzantine nodes do nothing else
			return;
		}
		

		while (!inStreamEmpty()) {
			Packet<BrachaMessage> newMsg = popInStream();
			BrachaMessage m = newMsg.getMessage();
			
			if (m.type == "echo"){
				echo_msgs[m.source] = m.value;
			}
			else if (m.type == "ready"){
				ready_msgs[m.source] = m.value;
			}
			
			if (sent_echo == false && m.type == "send"){
				//printf("Node_%ld: <-- (%ld, %s, %d)\n", id(), m.source, m.type.c_str(), m.value);
				sent_echo = true;
				BrachaMessage send_m;
				send_m.source = id();
				send_m.type = "echo";
				send_m.value = m.value;
				broadcast(send_m);
				//printf("Node_%ld: --> (%s, %d)\n", id(), send_m.type.c_str(), send_m.value);
			}

			int echo_val = check_echo();
			if (sent_ready == false && echo_val != -1){
				//printf("Node_%ld: (echo threshold met for value %d)\n", id(), echo_val);
				sent_ready = true;
				BrachaMessage m;
				m.source = id();
				m.type = "ready";
				m.value = echo_val;
				broadcast(m);
				//printf("Node_%ld: --> (%s, %d)\n", id(), m.type.c_str(), m.value);
			}

			int ready_val = check_ready();
			if (sent_ready == false && ready_val != -1){
				//printf("Node_%ld: (ready threshold met for value %d)\n", id(), ready_val);
				sent_ready = true;
				BrachaMessage m;
				m.source = id();
				m.type = "ready";
				m.value = ready_val;
				broadcast(m);
				//printf("Node_%ld: --> (%s, %d)\n", id(), m.type.c_str(), m.value);
			}

			int deliver_val = check_delivery();
			if (delivered == false && deliver_val != -1){
				//printf("Node_%ld: (delivery threshold met for value %d)\n", id(), deliver_val);
				delivered = true;
				finished_round = getRound();
				final_value = deliver_val;
			}

		}
		/*
       	cout << "Node_" << id() << " echo_msgs:  [";
		for (auto const& p : echo_msgs){
			cout << p.second << ", ";
		}
		cout << "]" << endl;
		cout << "Node_" << id() << " ready_msgs: [";
		for (auto const& p : ready_msgs){
			cout << p.second << ", ";
		}
		cout << "]" << endl;
		cout << endl;
		*/
	}

	void BrachaPeer::endOfRound(const vector<Peer<BrachaMessage>*>& _peers) {
		//cout << "End of round " << getRound() << endl;
	}

	Simulation<quantas::BrachaMessage, quantas::BrachaPeer>* generateSim() {
        
        Simulation<quantas::BrachaMessage, quantas::BrachaPeer>* sim = new Simulation<quantas::BrachaMessage, quantas::BrachaPeer>;
        return sim;
    }
}