/*
Copyright 2022

This file is part of QUANTAS.
QUANTAS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
QUANTAS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with QUANTAS. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ImbsRaynalPeer.hpp"

namespace quantas {

	//
	// Imbs-Raynal Channel definitions
	//
	ImbsRaynalPeer::~ImbsRaynalPeer() {

	}

	ImbsRaynalPeer::ImbsRaynalPeer(const ImbsRaynalPeer& rhs) : Peer<ImbsRaynalMessage>(rhs) {
		
	}

	ImbsRaynalPeer::ImbsRaynalPeer(long id) : Peer(id) {
		
	}

	int ImbsRaynalPeer::check_witness(interfaceId source, int value) {
		int counter = 0;
		for (const auto& p : received_witness) {
			if (p.first == source && p.second == value) counter++;
			if (counter >= witness_threshold) return value;
		}

		return -1;
	}

	int ImbsRaynalPeer::check_delivery(interfaceId source, int value) {
		int counter = 0;
		for (const auto& p : received_witness) {
			if (p.first == source && p.second == value) counter++;
			if (counter >= delivery_threshold) return value;
		}
		return -1;
	}

	bool ImbsRaynalPeer::contains(const vector<pair<long,int>>& s, long source) {
		for (const auto& p : s){
			if (p.first == source) return true;
		}
		return false;
	}

	bool ImbsRaynalPeer::contains(const vector<pair<long,int>>& s, long source, int value) {
		return find(s.begin(), s.end(), std::make_pair(source, value)) != s.end();
	}

	void ImbsRaynalPeer::initParameters(const vector<Peer<ImbsRaynalMessage>*>& _peers, json parameters) {
		const vector<ImbsRaynalPeer*> peers = reinterpret_cast<vector<ImbsRaynalPeer*> const&>(_peers);
		
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
		
		witness_threshold = n -2*f;
		delivery_threshold = n-f;
		delivered = false;
		received_init.clear();
		received_witness.clear();
		broadcast_witness.clear();
		
		finished_round = -1;
		final_value = -1;
		
	}

	void ImbsRaynalPeer::performComputation() {

		if (delivered) {
			// already delivered, do nothing
			return;
		}

		if (is_byzantine && getRound() == 0 && id() == sender) {
			ImbsRaynalMessage m1;
			m1.type = "init";
			m1.source = id();
			m1.value = 0;

			ImbsRaynalMessage m2;
			m2.type = "init";
			m2.source = id();
			m2.value = 1;
			byzantine_broadcast(m1, m2, percentage, honest_nodes);
			//cout << " sent byzantine init messages" << endl;
		}

		if (is_byzantine) {
			// Byzantine nodes do nothing else
			return;
		}

		//cout << "node_" << id() << " -------------------------------------" << endl;
		while (!inStreamEmpty()) {
			Packet<ImbsRaynalMessage> newMsg = popInStream();
			ImbsRaynalMessage m = newMsg.getMessage();
			//printf("<-- (%s, %ld, %d)\n", m.type.c_str(), m.source, m.value);
		
			if (m.type == "init"){
				if (!contains(received_init, m.source) && !contains(broadcast_witness, m.source)) {
					broadcast_witness.push_back(make_pair(m.source, m.value));
					ImbsRaynalMessage msg;
					msg.type = "witness";
					msg.source = m.source;
					msg.value = m.value;
					broadcast(msg);
					//printf("--> (%s, %ld, %d)\n", msg.type.c_str(), msg.source, msg.value);
				}
				received_init.push_back(make_pair(m.source, m.value));
			}

			if (m.type == "witness"){
				received_witness.push_back(make_pair(m.source, m.value));
				if( (check_witness(m.source, m.value)!=-1) && (!contains(broadcast_witness, m.source, m.value))) {
					broadcast_witness.push_back(make_pair(m.source, m.value));
					broadcast(m);
					//printf("--> (%s, %ld, %d)\n", m.type.c_str(), m.source, m.value);
				}
				if ((check_delivery(m.source, m.value)!=-1) && (delivered==false)) {
					delivered = true;
					finished_round = getRound();
					final_value = m.value;
					//cout << " DELIVERED value " << final_value << " in round " << finished_round << endl;
				}
			}
		}

		/*
		cout << "  received_init: [";
		for (const auto& p : received_init) {
			cout << "(" << p.first << "," << p.second << "), ";
		}
		cout << "]" << endl;
		cout << "  received_witness: [";
		for (const auto& p : received_witness) {
			cout << "(" << p.first << "," << p.second << "), ";
		}
		cout << "]" << endl;
		cout << "  broadcast_witness: [";
		for (const auto& p : broadcast_witness) {
			cout << "(" << p.first << "," << p.second << "), ";
		}
		cout << "]" << endl;
		cout << "--------------------------------------------" << endl << endl;
		*/

	}

	void ImbsRaynalPeer::endOfRound(const vector<Peer<ImbsRaynalMessage>*>& _peers) {
		//cout << "End of round " << getRound() << endl << endl;
	}

	Simulation<quantas::ImbsRaynalMessage, quantas::ImbsRaynalPeer>* generateSim() {
        
        Simulation<quantas::ImbsRaynalMessage, quantas::ImbsRaynalPeer>* sim = new Simulation<quantas::ImbsRaynalMessage, quantas::ImbsRaynalPeer>;
        return sim;
    }
}