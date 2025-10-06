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

	void BrachaPeer::initParameters(const vector<Peer<BrachaMessage>*>& _peers, json parameters) {
		const vector<BrachaPeer*> peers = reinterpret_cast<vector<BrachaPeer*> const&>(_peers);
		
		int f = parameters["f"];
		int n = parameters["n"];
		sender = parameters["sender"];
		percentage = parameters["percentage"];
		honest_group_0 = parameters["honest_group_0"].get<vector<interfaceId>>();
		honest_group_1 = parameters["honest_group_1"].get<vector<interfaceId>>();

		is_byzantine = true;
		if (parameters["byzantine_nodes"][id()] == 0) is_byzantine = false;

		honest_nodes.clear();
		for (int i = 0; i<n; i++){
			if (parameters["byzantine_nodes"][i] == 0) honest_nodes.push_back(i);
		}

		debug_prints = false;
		if (parameters.contains("debug_prints")) debug_prints = parameters["debug_prints"];
		
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

		// ------------------------------ STEP 0: Init --------------------------------------------
		if (is_byzantine && getRound() == 0 && id() == sender) {
			BrachaMessage m0;
			m0.source = id();
			m0.type = "send";
			m0.value = 0;

			BrachaMessage m1;
			m1.source = id();
			m1.type = "send";
			m1.value = 1;

			byzantine_broadcast(m0, m1, honest_group_0, honest_group_1);
			if (debug_prints) cout << " sent byzantine send messages" << endl;
		}

		if (!is_byzantine && getRound() == 0 && id() == sender) {
			BrachaMessage m0;
			m0.source = id();
			m0.type = "send";
			m0.value = 0;
			broadcast(m0);
			if (debug_prints) cout << " sent honest send messages" << endl;
		}

		// ------------------------------ Byzantine Ack/ Vote -----------------------------------------
		// Byzantine nodes send conflicting ack messages to honest groups
		// Honest nodes are split into two groups, each receiving a different value
		// This simulates a worst-case scenario where Byzantine nodes try to cause maximum confusion
		if (is_byzantine && getRound() == 0){
			BrachaMessage m0;
			m0.type = "echo";
			m0.source = id();
			m0.value = 0;
			BrachaMessage m1;
			m1.type = "echo";
			m1.source = id();
			m1.value = 1;
			// sends m0 to honest_group_1 and m1 to honest_group_0
			byzantine_broadcast(m0, m1, honest_group_1, honest_group_0);

			m0.type = "ready";
			m1.type = "ready";
			// sends m0 to honest_group_1 and m1 to honest_group_0
			byzantine_broadcast(m0, m1, honest_group_1, honest_group_0);
		}
		// ----------------------------------------------------------------------------------------

		if (is_byzantine) {
			// Byzantine nodes do nothing else
			return;
		}
		
		if (debug_prints) cout << "node_" << id() << " -------------------------------------" << endl;
		while (!inStreamEmpty()) {
			Packet<BrachaMessage> newMsg = popInStream();
			BrachaMessage m = newMsg.getMessage();
			if (debug_prints) printf("<-- (%s, %ld, %d)\n", m.type.c_str(), m.source, m.value);
			
			if (m.type == "echo"){
				echo_msgs[m.source] = m.value;
			}
			else if (m.type == "ready"){
				ready_msgs[m.source] = m.value;
			}
			

			// ------------------------------ STEP 1: ECHO Phase ----------------------------------
			if (sent_echo == false && m.type == "send"){
				sent_echo = true;
				BrachaMessage echo_m;
				echo_m.source = id();
				echo_m.type = "echo";
				echo_m.value = m.value;
				broadcast(echo_m);
				if (debug_prints) printf("--> (%s, %ld, %d)\n", echo_m.type.c_str(), echo_m.source, echo_m.value);
			}
			// ------------------------------------------------------------------------------------


			// ------------------------------ STEP 2: READY Phase ---------------------------------
			int echo_val = check_echo();
			if (sent_ready == false && echo_val != -1){
				sent_ready = true;
				BrachaMessage ready_msg;
				ready_msg.source = id();
				ready_msg.type = "ready";
				ready_msg.value = echo_val;
				broadcast(ready_msg);
				if (debug_prints) printf("--> (%s, %ld, %d)\n", ready_msg.type.c_str(), ready_msg.source, ready_msg.value);
			}

			int ready_val = check_ready();
			if (sent_ready == false && ready_val != -1){
				sent_ready = true;
				BrachaMessage ready_msg;
				ready_msg.source = id();
				ready_msg.type = "ready";
				ready_msg.value = ready_val;
				broadcast(ready_msg);
				if (debug_prints) printf("--> (%s, %ld, %d)\n", ready_msg.type.c_str(), ready_msg.source, ready_msg.value);
			}
			// ------------------------------------------------------------------------------------

			
			// ------------------------------ STEP 3: Deliver -------------------------------------
			int deliver_val = check_delivery();
			if (delivered == false && deliver_val != -1){
				delivered = true;
				finished_round = getRound();
				final_value = deliver_val;
				if (debug_prints) cout << " DELIVERED value " << final_value << endl;
			}
			// ------------------------------------------------------------------------------------

		}
		
		if (debug_prints) {
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
			cout << "--------------------------------------------" << endl << endl;
		}
	}

	void BrachaPeer::endOfRound(const vector<Peer<BrachaMessage>*>& _peers) {
		if (debug_prints) cout << "-------------------------------------------------- End of round " << getRound() << "--------------------------------------------------" << endl << endl;
	}

	Simulation<quantas::BrachaMessage, quantas::BrachaPeer>* generateSim() {
        
        Simulation<quantas::BrachaMessage, quantas::BrachaPeer>* sim = new Simulation<quantas::BrachaMessage, quantas::BrachaPeer>;
        return sim;
    }
}