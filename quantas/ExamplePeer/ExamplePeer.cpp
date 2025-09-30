/*
Copyright 2022

This file is part of QUANTAS.
QUANTAS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
QUANTAS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with QUANTAS. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ExamplePeer.hpp"

namespace quantas {

	//
	// Example Channel definitions
	//
	ExamplePeer::~ExamplePeer() {

	}

	ExamplePeer::ExamplePeer(const ExamplePeer& rhs) : Peer<ExampleMessage>(rhs) {
		
	}

	ExamplePeer::ExamplePeer(long id) : Peer(id) {
		
	}

	void ExamplePeer::initParameters(const vector<Peer<ExampleMessage>*>& _peers, json parameters) {
		const vector<ExamplePeer*> peers = reinterpret_cast<vector<ExamplePeer*> const&>(_peers);
		
		//cout << "Initializing parameters of simulation" << endl;

		if (parameters.contains("parameter1")) {
			//cout << "parameter1: " << parameters["parameter1"] << endl;
		}

		if (parameters.contains("parameter2")) {
			//cout << "parameter2: " << parameters["parameter2"] << endl;
		}

		if (parameters.contains("parameter3")) {
			//cout << "parameter3: " << parameters["parameter3"] << endl;
		}
	}

	void ExamplePeer::performComputation() {

		//cout << "Peer:" << id() << " performing computation" << endl;

		// Read messages from other peers
		while (!inStreamEmpty()) {
			Packet<ExampleMessage> newMsg = popInStream();
			cout << endl << std::to_string(id()) << " has receved a message from " << newMsg.getMessage().aPeerId << endl;
			cout << newMsg.getMessage().message << endl;
		}
		cout << endl;

		if (getRound() == 0){
			cout << "neighbors of node_" << id() << ": [ ";
			for (auto const& n : neighbors()){
				cout << n << ", ";
			}
			cout << "]" << endl;
		}

		// Send hello to everyone else
		if (getRound() == 0 && id() == 0){
			ExampleMessage msg;
			msg.message = "Message: Hello From " + std::to_string(id()) + ". Sent on round: " + std::to_string(getRound());
			msg.aPeerId = std::to_string(id());
			broadcast(msg);
		}
		
	}

	void ExamplePeer::endOfRound(const vector<Peer<ExampleMessage>*>& _peers) {
		cout << "End of round " << getRound() << endl;
	}

	Simulation<quantas::ExampleMessage, quantas::ExamplePeer>* generateSim() {
        
        Simulation<quantas::ExampleMessage, quantas::ExamplePeer>* sim = new Simulation<quantas::ExampleMessage, quantas::ExamplePeer>;
        return sim;
    }
}