/*
Copyright 2022

This file is part of QUANTAS.
QUANTAS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
QUANTAS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with QUANTAS. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Alg23Peer.hpp"

namespace quantas {

	//
	// Example Channel definitions
	//
	Alg23Peer::~Alg23Peer() {

	}

	Alg23Peer::Alg23Peer(const Alg23Peer& rhs) : Peer<Alg23Message>(rhs) {
		
	}

	Alg23Peer::Alg23Peer(long id) : Peer(id) {
		
	}

	void Alg23Peer::initParameters(const vector<Peer<Alg23Message>*>& _peers, json parameters) {
		const vector<Alg23Peer*> peers = reinterpret_cast<vector<Alg23Peer*> const&>(_peers);
		
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

		debug_prints = false;
		if (parameters.contains("debug_prints")) debug_prints = parameters["debug_prints"];
		
		ack_ack_threshold = n-2*f;
		ack_delivery_threshold = n-f-1;

		delivered = false;
		is_first_propose = true;
		ack_msgs.clear();
		sent_ack_msgs.clear();
		
		finished_round = -1;
		final_value = -1;
		
	}

	void Alg23Peer::performComputation() {

		// ------------------------------ STEP 1: Propose -----------------------------------------
		if (is_byzantine && getRound() == 0 && id() == sender) {
			Alg23Message m0;
			m0.type = "propose";
			m0.source = id();
			m0.value = 0;

			Alg23Message m1;
			m1.type = "propose";
			m1.source = id();
			m1.value = 1;
			
			vector<interfaceId> group_1;
			vector<interfaceId> group_2;
			byzantine_broadcast(m0, m1, percentage, honest_nodes, group_1, group_2);
			if (debug_prints) cout << " sent byzantine propose messages" << endl;
		}

		if (!is_byzantine && getRound() == 0 && id() == sender) {
			Alg23Message m0;
			m0.type = "propose";
			m0.source = id();
			m0.value = 0;
			broadcast(m0);
			if (debug_prints) cout << " sent honest propose message" << endl;
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
			Packet<Alg23Message> newMsg = popInStream();
			Alg23Message m = newMsg.getMessage();
			addMsg(m);
			if (debug_prints) printf("<-- (%s, %ld, %d)\n", m.type.c_str(), m.source, m.value);
			
			// ------------------------------ STEP 2.1: Propose -> Ack ----------------------------
			if (m.type == "propose" && is_first_propose){
				is_first_propose = false;
				Alg23Message ack_msg;
				ack_msg.type = "ack";
				ack_msg.source = id();
				ack_msg.value = m.value;
				broadcast(ack_msg);
				sent_ack_msgs.push_back(m.value);
				if (debug_prints) printf("--> (%s, %ld, %d)\n", ack_msg.type.c_str(), ack_msg.source, ack_msg.value);
			}
			// ------------------------------------------------------------------------------------


			if (m.type == "ack"){
				// ------------------------------ STEP 2.2: Ack -> Ack ----------------------------
				if ((count(ack_msgs, m.value) >= ack_ack_threshold) && (find(sent_ack_msgs.begin(), sent_ack_msgs.end(), m.value) == sent_ack_msgs.end())){
					Alg23Message ack_msg;
					ack_msg.type = "ack";
					ack_msg.source = id();
					ack_msg.value = m.value;
					broadcast(ack_msg);
					sent_ack_msgs.push_back(ack_msg.value);
					if (debug_prints) printf("--> (%s, %ld, %d)\n", ack_msg.type.c_str(), ack_msg.source, ack_msg.value);
				}
				// --------------------------------------------------------------------------------


				// ------------------------------ STEP 3: Commit ----------------------------------
				if ((count(ack_msgs, m.value) >= ack_delivery_threshold) && (delivered == false)){
					final_value = m.value;
					finished_round = getRound();
					delivered = true;
					if (debug_prints) cout << " DELIVERED value " << final_value << endl;
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
			cout << "--------------------------------------------" << endl << endl;
		}
		
	}

	void Alg23Peer::endOfRound(const vector<Peer<Alg23Message>*>& _peers) {
		if (debug_prints) cout << "-------------------------------------------------- End of round " << getRound() << "--------------------------------------------------" << endl << endl;
	}

	Simulation<quantas::Alg23Message, quantas::Alg23Peer>* generateSim() {
        
        Simulation<quantas::Alg23Message, quantas::Alg23Peer>* sim = new Simulation<quantas::Alg23Message, quantas::Alg23Peer>;
        return sim;
    }
}