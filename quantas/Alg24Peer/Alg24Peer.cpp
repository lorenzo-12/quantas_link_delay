/*
Copyright 2022

This file is part of QUANTAS.
QUANTAS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
QUANTAS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with QUANTAS. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Alg24Peer.hpp"

namespace quantas {

	//
	// Example Channel definitions
	//
	Alg24Peer::~Alg24Peer() {

	}

	Alg24Peer::Alg24Peer(const Alg24Peer& rhs) : Peer<Alg24Message>(rhs) {
		
	}

	Alg24Peer::Alg24Peer(long id) : Peer(id) {
		
	}

	void Alg24Peer::initParameters(const vector<Peer<Alg24Message>*>& _peers, json parameters) {
		const vector<Alg24Peer*> peers = reinterpret_cast<vector<Alg24Peer*> const&>(_peers);
		
		int f = parameters["f"];
		int n = parameters["n"];
		network_size = n;
		sender = parameters["sender"];
		percentage = parameters["percentage"];
		honest_group_0 = parameters["honest_group_0"].get<vector<interfaceId>>();
		honest_group_1 = parameters["honest_group_1"].get<vector<interfaceId>>();
		combination = parameters["combination"].get<vector<string>>();

		is_byzantine = true;
		if (parameters["byzantine_nodes"][id()] == 0) is_byzantine = false;

		honest_nodes.clear();
		for (int i = 0; i<n; i++){
			if (parameters["byzantine_nodes"][i] == 0) honest_nodes.push_back(i);
		}

		debug_prints = false;
		if (parameters.contains("debug_prints")) debug_prints = parameters["debug_prints"];
		
		ack_delivery_threshold = n-f-1;
		ack_vote1_threshold = n-2*f;
		vote1_vote2_threshold = n-f-1;
		vote2_vote2_threshold = f+1;
		vote2_delivery_threshold = n-f-1;

		// reset algorithm specific variables
		delivered = false;
		is_first_propose = true;
		ack_sent = false;
		vote1_sent = false;
		vote2_sent = false;
		ack_msgs.clear();
		vote1_msgs.clear();
		vote2_msgs.clear();
		
		finished_round = -1;
		final_value = -1;
		finishing_step = -1;
		total_msgs_sent = 0;
		
	}

	void Alg24Peer::performComputation() {

		// ------------------------------ STEP 1: Propose -----------------------------------------
		if (is_byzantine && getRound() == 0 && id() == sender) {
			Alg24Message m0;
			m0.type = "propose";
			m0.source = id();
			m0.value = 0;

			Alg24Message m1;
			m1.type = "propose";
			m1.source = id();
			m1.value = 1;

			byzantine_broadcast(m0, m1, honest_group_0, honest_group_1);
			if (debug_prints) cout << " sent byzantine propose messages" << endl;
		}

		if (!is_byzantine && getRound() == 0 && id() == sender) {
			Alg24Message m0;
			m0.type = "propose";
			m0.source = id();
			m0.value = 0;
			broadcast(m0);
			total_msgs_sent  += network_size;
			if (debug_prints) cout << " sent honest propose message" << endl;
		}

		// ------------------------------ Byzantine Ack/ Vote -----------------------------------------
		// Byzantine nodes send conflicting ack messages to honest groups
		// Honest nodes are split into two groups, each receiving a different value
		// This simulates a worst-case scenario where Byzantine nodes try to cause maximum confusion
		if (is_byzantine && getRound() == 0){
			Alg24Message ack_m0;
			ack_m0.type = "ack";
			ack_m0.source = id();
			ack_m0.value = 0;
			Alg24Message ack_m1;
			ack_m1.type = "ack";
			ack_m1.source = id();
			ack_m1.value = 1;
            Alg24Message vote1_m0;
			vote1_m0.type = "vote1";
			vote1_m0.source = id();
			vote1_m0.value = 0;
			Alg24Message vote1_m1;
			vote1_m1.type = "vote1";
			vote1_m1.source = id();
			vote1_m1.value = 1;
			Alg24Message vote2_m0;
			vote2_m0.type = "vote2";
			vote2_m0.source = id();
			vote2_m0.value = 0;
			Alg24Message vote2_m1;
			vote2_m1.type = "vote2";
			vote2_m1.source = id();
			vote2_m1.value = 1;

			// cout << "Combination: " << combination[0] << " - " << combination[1] << " - " << combination[2] << "  -->  ";
			if (combination[0] == "silent"){
				// do nothing
				//cout << "silent - ";
			}
			if (combination[0] == "same"){
				byzantine_broadcast(ack_m0, ack_m1, honest_group_0, honest_group_1);
				//cout << "same - ";
			}
			if (combination[0] == "opposite"){
				byzantine_broadcast(ack_m0, ack_m1, honest_group_1, honest_group_0);
				//cout << "opposite - ";
			}
			if (combination[1] == "silent"){
				// do nothing
				//cout << "silent - ";
			}
			if (combination[1] == "same"){
				byzantine_broadcast(vote1_m0, vote1_m1, honest_group_0, honest_group_1);
				//cout << "same - ";
			}
			if (combination[1] == "opposite"){
				byzantine_broadcast(vote1_m0, vote1_m1, honest_group_1, honest_group_0);
				//cout << "opposite - ";
			}
			if (combination[2] == "silent"){
				// do nothing
				//cout << "silent" << endl;
			}
			if (combination[2] == "same"){
				byzantine_broadcast(vote2_m0, vote2_m1, honest_group_0, honest_group_1);
				//cout << "same" << endl;
			}
			if (combination[2] == "opposite"){
				byzantine_broadcast(vote2_m0, vote2_m1, honest_group_1, honest_group_0);
				//cout << "opposite" << endl;
			}

		}
		// ----------------------------------------------------------------------------------------

		if (is_byzantine) {
			// Byzantine nodes do nothing else
			return;
		}

		if (delivered) {
			// Once delivered, do nothing
			return;
		}

		if (debug_prints) cout << "node_" << id() << " -------------------------------------" << endl;
		while (!inStreamEmpty()) {
			Packet<Alg24Message> newMsg = popInStream();
			Alg24Message m = newMsg.getMessage();
			addMsg(m);
			if (debug_prints) printf("<-- (%s, %ld, %d)\n", m.type.c_str(), m.source, m.value);
			
			// ------------------------------ STEP 2: Ack -----------------------------------------
			if (m.type == "propose" && is_first_propose) {
				Alg24Message ack_msg;
				ack_msg.type = "ack";
				ack_msg.source = id();
				ack_msg.value = m.value;
				broadcast(ack_msg);
				total_msgs_sent  += network_size;
				ack_sent = true;
				if (debug_prints) printf("--> step 2: (%s, %ld, %d)\n", ack_msg.type.c_str(), ack_msg.source, ack_msg.value);
			}
			// ------------------------------------------------------------------------------------

			if (m.type == "ack"){


				// ------------------------------ STEP 3: 2-Round Commit --------------------------
				if ((count(ack_msgs, m.value) >= ack_delivery_threshold) && (delivered == false)){
					Alg24Message vote1_msg;
					vote1_msg.type = "vote1";
					vote1_msg.source = id();
					vote1_msg.value = m.value;
					broadcast(vote1_msg);
					total_msgs_sent  += network_size;
					vote1_sent = true;
					if (debug_prints) printf("--> step 3: (%s, %ld, %d)\n", vote1_msg.type.c_str(), vote1_msg.source, vote1_msg.value);

					Alg24Message vote2_msg;
					vote2_msg.type = "vote2";
					vote2_msg.source = id();
					vote2_msg.value = m.value;
					broadcast(vote2_msg);
					total_msgs_sent  += network_size;
					vote2_sent = true;
					if (debug_prints) printf("--> step 3: (%s, %ld, %d)\n", vote2_msg.type.c_str(), vote2_msg.source, vote2_msg.value);

					delivered = true;
					finished_round = getRound();
					final_value = m.value;
					finishing_step = 2;
					if (debug_prints) cout << " step 3: DELIVERED value " << final_value << endl;
				}
				// --------------------------------------------------------------------------------


				// ------------------------------ STEP 4.1: Ack -> Vote1 --------------------------
				if ((count(ack_msgs, m.value) >= ack_vote1_threshold) && (vote1_sent == false)){
					Alg24Message vote1_msg;
					vote1_msg.type = "vote1";
					vote1_msg.source = id();
					vote1_msg.value = m.value;
					broadcast(vote1_msg);
					total_msgs_sent  += network_size;
					vote1_sent = true;
					if (debug_prints) printf("--> step 4.1: (%s, %ld, %d)\n", vote1_msg.type.c_str(), vote1_msg.source, vote1_msg.value);
				}
				// --------------------------------------------------------------------------------
			}

			// ------------------------------ STEP 4.2: Vote1 -> Vote2 ----------------------------
			if (m.type == "vote1"){
				if ((count(vote1_msgs, m.value) >= vote1_vote2_threshold) && (vote2_sent == false)){
					Alg24Message vote2_msg;
					vote2_msg.type = "vote2";
					vote2_msg.source = id();
					vote2_msg.value = m.value;
					broadcast(vote2_msg);
					total_msgs_sent  += network_size;
					vote2_sent = true;
					if (debug_prints) printf("--> step 4.2: (%s, %ld, %d)\n", vote2_msg.type.c_str(), vote2_msg.source, vote2_msg.value);
				}
			}
			// ------------------------------------------------------------------------------------


			if (m.type == "vote2"){
				// ------------------------------ STEP 4.3: Vote2 -> Vote2 ------------------------
				if ((count(vote2_msgs, m.value) >= vote2_vote2_threshold) && (vote2_sent == false)){
					Alg24Message vote2_msg;
					vote2_msg.type = "vote2";
					vote2_msg.source = id();
					vote2_msg.value = m.value;
					broadcast(vote2_msg);
					total_msgs_sent  += network_size;
					vote2_sent = true;
					if (debug_prints) printf("--> step 4.3: (%s, %ld, %d)\n", vote2_msg.type.c_str(), vote2_msg.source, vote2_msg.value);
				}
				// --------------------------------------------------------------------------------


				// ------------------------------ STEP 5: Commit ----------------------------------
				if ((count(vote2_msgs, m.value) >= vote2_delivery_threshold) && (delivered == false)){
					delivered = true;
					finished_round = getRound();
					final_value = m.value;
					finishing_step = 4;
					if (debug_prints) cout << " step 5: DELIVERED value " << final_value << endl;
				}
				// --------------------------------------------------------------------------------
			}
			
		}

		if (debug_prints) {
			cout << " ack_msgs: [";
			for (const auto& p : ack_msgs) {
				cout << "(" << p.first << "," << p.second << ") ";
			}
			cout << "]" << endl;
			cout << " vote1_msgs: [";
			for (const auto& p : vote1_msgs) {
				cout << "(" << p.first << "," << p.second << ") ";
			}
			cout << "]" << endl;
			cout << " vote2_msgs: [";
			for (const auto& p : vote2_msgs) {
				cout << "(" << p.first << "," << p.second << ") ";
			}
			cout << "]" << endl;
			cout << "--------------------------------------------" << endl << endl;
		}
	}

	void Alg24Peer::endOfRound(const vector<Peer<Alg24Message>*>& _peers) {
		if (debug_prints) cout << "-------------------------------------------------- End of round " << getRound() << "--------------------------------------------------" << endl << endl;
	}

	Simulation<quantas::Alg24Message, quantas::Alg24Peer>* generateSim() {
        
        Simulation<quantas::Alg24Message, quantas::Alg24Peer>* sim = new Simulation<quantas::Alg24Message, quantas::Alg24Peer>;
        return sim;
    }
}