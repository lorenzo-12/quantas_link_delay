/*
Copyright 2022

This file is part of QUANTAS.
QUANTAS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
QUANTAS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with QUANTAS. If not, see <https://www.gnu.org/licenses/>.
*/

// This class is responsible for setting up connections between peers and execution of a round. 
// The underlaying data structure is a vector of peers(abstract class). It sets channel delays 
// between peers when the network is initialized. These delays are between maximum and one. It 
// is templated with a user defined message and peer class. 


#ifndef Network_hpp
#define Network_hpp

#include <stdio.h>
#include <vector>
#include <string>
#include <random>
#include <limits>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <memory>
#include <thread>
#include <algorithm>
#include <climits>
//#include <bits/stdc++.h>
#include "Peer.hpp"
#include "Distribution.hpp"

namespace quantas{

    using std::string;
    using std::ostream;
    using std::vector;
    using std::cout;
    using std::endl;
    using std::left;
    using std::setw;
    using std::thread;
    using nlohmann::json;

    template<class type_msg, class peer_type>
    class Network{
    protected:

        vector<Peer<type_msg>*>             _peers;
        Distribution                        _distribution;

        void                                addEdges            (Peer<type_msg>*, int, int);
        peer_type*							getPeerById			(string);

    public:
        Network                                                 ();
        Network                                                 (const Network<type_msg,peer_type>&);
        ~Network                                                ();

        // topologies
        void                                initNetwork         (json, int); // initialize network with peers
        void                                initParameters      (json);
        void                                fullyConnect        (int);
        void                                fullyConnect_selfLoops(int);
        void                                star                (int);
        void                                grid                (int, int);
        void                                torus               (int, int);
        void                                chain               (int);
        void                                ring                (int);
        void                                unidirectionalRing  (int);
        void                                userList            (json);
	    void                                dynamic             (int, int);
        
        void                                setDistribution     (json distribution)                             { _distribution.setDistribution(distribution); }

        // getters
        int                                 size                ()const                                         {return (int)_peers.size();};
        int                                 maxDelay            ()const                                         {return _distribution.maxDelay();};
        int                                 avgDelay            ()const                                         {return _distribution.avgDelay();};
        int                                 minDelay            ()const                                         {return _distribution.minDelay();};
        string                              type                ()const                                         {return _distribution.type();};
        vector<Peer<type_msg>*>             peers               ()const                                         {return _peers;};

        //mutators
        void                                receive             (int begin, int end);
        void                                performComputation  (int begin, int end);
        void                                endOfRound          ();
        void                                transmit            (int begin, int end);
        void                                makeRequest         (int i)                                         {_peers[i]->makeRequest();};
        void                                incrementRound();
        void                                initializeRound();
        // void                                shuffleByzantines   (int);

        // operators
        Network&                            operator=           (const Network&);
        peer_type*                          operator[]          (int);
        const peer_type*                    operator[]          (int)const;
        void 								makeRequest	        (Peer<type_msg> * peer)				            { peer->makeRequest(); }

    };

    template<class type_msg, class peer_type>
    Network<type_msg,peer_type>::Network(){
        _peers = vector<Peer<type_msg>*>();
        _distribution = Distribution();
    }

    template<class type_msg, class peer_type>
    Network<type_msg,peer_type>::Network(const Network<type_msg,peer_type> &rhs){
        if(this == &rhs){
            return;
        }

        for(int i = 0; i < _peers.size(); i++){
            delete _peers[i];
        }

        _peers = vector<Peer<type_msg>*>();
        for(int i = 0; i < rhs._peers.size(); i++){
            _peers.push_back(new peer_type(*dynamic_cast<peer_type*>(rhs._peers[i])));
        }
        _distribution = rhs._distribution;
    }

    template<class type_msg, class peer_type>
    Network<type_msg,peer_type>::~Network(){
        for(int i = 0; i < _peers.size(); i++){
            delete _peers[i];
        }
    }

	template<class type_msg, class peer_type>
	void Network<type_msg, peer_type>::addEdges(Peer<type_msg>* peer, int maxMsgsRec, int lastRound) {
		for (int i = 0; i < _peers.size() - 1; i++) {

			// determine max total throughput
            int maxThroughputEver = maxMsgsRec*(lastRound+1);
            if (maxThroughputEver / (lastRound+1) != maxMsgsRec) {
                // overflow handling (maxMsgsRec could already be INT_MAX)
                maxThroughputEver = INT_MAX;
            }

            // set the delays for the two directions of the link to 1 because doesn't matter what we put, when sending a message
            // the function "transmit" will set the actual delay based on the distribution
            peer->addChannel(*_peers[i], 1, maxThroughputEver);
			_peers[i]->addChannel(*peer, 1, maxThroughputEver);
		}
	}

	template<class type_msg, class peer_type>
	void Network<type_msg, peer_type>::initNetwork(json topology, int lastRound) {
	    for (int i = 0; i < _peers.size(); i++) {
            delete _peers[i];
        }
        _peers = vector<Peer<type_msg>*>();
        // if there isn't one assume INT_MAX
        int maxMsgsRec = INT_MAX;
        if (topology.contains("maxMsgsRec")) {
            maxMsgsRec = topology["maxMsgsRec"];
        }
		for (int i = 0; i < topology["totalPeers"]; i++) {
			_peers.push_back(new peer_type(i));
            _peers[i]->setMaxMsgsRec(maxMsgsRec);
            _peers[i]->setDistribution(_distribution);
			addEdges(_peers[i], maxMsgsRec, lastRound);
		}
        if (topology["identifiers"] == "random") {
            // randomly shuffle nodes prior to setting up topology
            std::shuffle(_peers.begin(),_peers.end(), RANDOM_GENERATOR);
        }

	    if (topology["type"] == "complete") {
	        fullyConnect(topology["initialPeers"]);
	    }
        else if (topology["type"] == "fullyComplete") {
            fullyConnect_selfLoops(topology["initialPeers"]);
        }
        else if (topology["type"] == "star") {
            star(topology["initialPeers"]);
        }
	    else if (topology["type"] == "grid") {
	        grid(topology["height"], topology["width"]);
	    }
	    else if (topology["type"] == "torus") {
            torus(topology["height"], topology["width"]);
	    }
        else if (topology["type"] == "chain") {
            chain(topology["initialPeers"]);
        }
        else if (topology["type"] == "ring") {
            ring(topology["initialPeers"]);
        }
        else if (topology["type"] == "unidirectionalRing") {
            unidirectionalRing(topology["initialPeers"]);
        }
        else if (topology["type"] == "userList") {
            userList(topology);
        }
	    else if (topology["type"] == "dynamic") {
            dynamic(topology["initialPeers"], topology["sourcePoolSize"]);
        }
        else {
            std::cerr << "Error: need an input for 'type' of topology" << std::endl;
        }
        Peer<type_msg>::initializeRound();
	    Peer<type_msg>::initializeLastRound(lastRound -1);
	}
	
    template<class type_msg, class peer_type>
    void Network<type_msg, peer_type>::initParameters(json parameters) {
        for (int i = 0; i < _peers.size(); i++) {
            _peers[i]->initParameters(_peers, parameters);
        }
    }

    template<class type_msg, class peer_type>
    void Network<type_msg, peer_type>::fullyConnect(int numberOfPeers) {
        for (int i = 0; i < numberOfPeers; i++) {
            // Activate peer
            for (int j = i+1; j < numberOfPeers; j++) {
                _peers[i]->addNeighbor(_peers[j]->id());
                _peers[j]->addNeighbor(_peers[i]->id());
            }
        }
    }

    template<class type_msg, class peer_type>
    void Network<type_msg, peer_type>::fullyConnect_selfLoops(int numberOfPeers) {
        for (int i = 0; i < numberOfPeers; i++) {
            // Activate peer
            for (int j = i; j < numberOfPeers; j++) {
                if (i == j) _peers[i]->addNeighbor(_peers[j]->id());
                else{
                    _peers[i]->addNeighbor(_peers[j]->id());
                    _peers[j]->addNeighbor(_peers[i]->id());
                }
            }
        }
    }

    // Connects all peers to peer 0
    template<class type_msg, class peer_type>
    void Network<type_msg, peer_type>::star(int numberOfPeers) {
        for (int i = 1; i < numberOfPeers; i++) {
            // Activate peer
            _peers[0]->addNeighbor(_peers[i]->id());
            _peers[i]->addNeighbor(_peers[0]->id());
        }
    }

	template<class type_msg, class peer_type>
	void Network<type_msg, peer_type>::grid(int height, int width) {
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				// Activate peer
				int num = i * width + j;
				if (i == 0) {
					// Top row adds only left neighbor
					if (j != 0) {
						_peers[num]->addNeighbor(_peers[num - 1]->id());
						_peers[num - 1]->addNeighbor(_peers[num]->id());
					}
				}
				else {
					// Left column adds only above neighbor
					_peers[num]->addNeighbor(_peers[num - width]->id());
					_peers[num - width]->addNeighbor(_peers[num]->id());
					// All others add both left and above
					if (j != 0) {
						_peers[num]->addNeighbor(_peers[num - 1]->id());
						_peers[num - 1]->addNeighbor(_peers[num]->id());
					}
				}
			}
		}
	}

    template<class type_msg, class peer_type>
    void Network<type_msg, peer_type>::torus(int height, int width) {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                // Activate peer
                int num = i * width + j;
                if (i == 0) {
                    // Top row adds only left neighbor
                    if (j != 0) {
                        _peers[num]->addNeighbor(_peers[num - 1]->id());
                        _peers[num - 1]->addNeighbor(_peers[num]->id());
                    }
                    // Right column creates torus
                    if (j == width - 1) {
                        _peers[num]->addNeighbor(_peers[num - j]->id());
                        _peers[num - j]->addNeighbor(_peers[num]->id());
                    }
                }
                else {
                    // Left column adds only above neighbor
                    _peers[num]->addNeighbor(_peers[num - width]->id());
                    _peers[num - width]->addNeighbor(_peers[num]->id());
                    // All others add both left and above
                    if (j != 0) {
                        _peers[num]->addNeighbor(_peers[num - 1]->id());
                        _peers[num - 1]->addNeighbor(_peers[num]->id());
                    }
                    // Right column creates torus
                    if (j == width - 1) {
                        _peers[num]->addNeighbor(_peers[num - j]->id());
                        _peers[num - j]->addNeighbor(_peers[num]->id());
                    }
                    // Bottom row creates torus
                    if (i == height - 1) {
                        _peers[num]->addNeighbor(_peers[j]->id());
                        _peers[j]->addNeighbor(_peers[num]->id());
                    }
                }
            }
        }
    }

    template<class type_msg, class peer_type>
    void Network<type_msg, peer_type>::chain(int numberOfPeers) {
        for (int i = 1; i < numberOfPeers; i++) {
            // Activate peer
            _peers[i]->addNeighbor(_peers[i-1]->id());
            _peers[i-1]->addNeighbor(_peers[i]->id());
        }
    }

    template<class type_msg, class peer_type>
    void Network<type_msg, peer_type>::ring(int numberOfPeers) {
        for (int i = 1; i < numberOfPeers; i++) {
            // Activate peer
            _peers[i]->addNeighbor(_peers[i - 1]->id());
            _peers[i - 1]->addNeighbor(_peers[i]->id());
        }
        _peers[0]->addNeighbor(_peers[numberOfPeers - 1]->id());
        _peers[numberOfPeers - 1]->addNeighbor(_peers[0]->id());
    }

    template<class type_msg, class peer_type>
    void Network<type_msg, peer_type>::unidirectionalRing(int numberOfPeers) {
        for (int i = 1; i < numberOfPeers; i++) {
            // Activate peer
            _peers[i - 1]->addNeighbor(_peers[i]->id());
        }
        _peers[numberOfPeers - 1]->addNeighbor(_peers[0]->id());
    }

    template<class type_msg, class peer_type>
    void Network<type_msg, peer_type>::userList(json topology) {
        json list = topology["list"];
        for (int i = 0; i < topology["totalPeers"]; i++) {
            if (list.contains(std::to_string(i))) {
                json comLinks = list[std::to_string(i)];
                for (int j = 0; j < comLinks.size(); j++) {
                    _peers[i]->addNeighbor(comLinks[j]);
                }
            }
        }
    }

    // addNeighbor() adds peer to peer's _neighbors vector. I.e., it determines who the peer can broadcast to.
    template<class type_msg, class peer_type>
    void Network<type_msg, peer_type>::dynamic(int numberOfPeers, int sourcePoolSize) {
        Peer<type_msg>::initializeSourcePoolSize(sourcePoolSize);
        for (int i = 0; i < numberOfPeers; ++i) {
            for (int j = i + 1; j < numberOfPeers; ++j) {
                if (j >= sourcePoolSize && !(i >= sourcePoolSize)) { // if peer[j] isn't apart of the source pool but peer[i] is...
                    _peers[i]->addNeighbor(_peers[j]->id());   
	            }
                else {
                    _peers[i]->addNeighbor(_peers[j]->id());
                    _peers[j]->addNeighbor(_peers[i]->id());
                }
            }
        }
    }

    template<class type_msg, class peer_type>
    void Network<type_msg,peer_type>::receive(int begin, int end){
        for (int i = begin; i < end; i++) {
		    _peers[i]->receive();
	    }
    }

    template<class type_msg, class peer_type>
    void Network<type_msg,peer_type>::performComputation(int begin, int end){
        for (int i = begin; i < end; i++) {
            _peers[i]->performComputation();
        }
    }

    template<class type_msg, class peer_type>
    void Network<type_msg, peer_type>::endOfRound() {
        _peers[0]->endOfRound(_peers);
        Peer<type_msg>::incrementRound();
    }

    template<class type_msg, class peer_type>
    void Network<type_msg,peer_type>::transmit(int begin, int end){
        for (int i = begin; i < end; i++) {
            _peers[i]->transmit();
        }
    }

    template<class type_msg, class peer_type>
    Network<type_msg, peer_type>& Network<type_msg,peer_type>::operator=(const Network<type_msg, peer_type> &rhs){
        if(this == &rhs){
            return *this;
        }

        for(int i = 0; i < _peers.size(); i++){
            delete _peers[i];
        }

        _peers = vector<Peer<type_msg>*>();
        for(int i = 0; i < rhs._peers.size(); i++){
            _peers.push_back(new peer_type(*dynamic_cast<peer_type*>(rhs._peers[i])));
        }

        return *this;
    }

    template<class type_msg, class peer_type>
    peer_type* Network<type_msg,peer_type>::operator[](int i){
        return dynamic_cast<peer_type*>(_peers[i]);
    }

    template<class type_msg, class peer_type>
    const peer_type* Network<type_msg,peer_type>::operator[](int i)const{
        return dynamic_cast<peer_type*>(_peers[i]);
    }

    template<class type_msg, class peer_type>
    peer_type* Network<type_msg,peer_type>::getPeerById(string id){
        for(int i = 0; i<_peers.size(); i++){
            if(_peers[i]->id() ==  id)
                return dynamic_cast<peer_type*>(_peers[i]);
        }
        return nullptr;
    }
}
#endif /* Network_hpp */
