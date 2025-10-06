/*
Copyright 2022

This file is part of QUANTAS.
QUANTAS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
QUANTAS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with QUANTAS. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef ImbsRaynalPeer_hpp
#define ImbsRaynalPeer_hpp

#include <iostream>
#include <vector>
#include <set> 
#include <utility>    
#include <algorithm>
#include "../Common/Peer.hpp"
#include "../Common/Simulation.hpp"

namespace quantas{

    using std::string; 
    using std::ostream;

    //
    // Example of a message body type
    //
    struct ImbsRaynalMessage{

        string type;
        long source;
        int value;
    };

    //
    // Example Peer used for network testing
    //
    class ImbsRaynalPeer : public Peer<ImbsRaynalMessage>{
    public:

        // ----------- Result parameters -----------
        bool is_byzantine;
        long sender;
        int percentage;
        vector<interfaceId> honest_nodes;

        int finished_round = -1;
        int final_value = -1;
        bool debug_prints;
        // -----------------------------------------

        // ----- Algorithm specific parameters -----
        vector<interfaceId> honest_group_0;
        vector<interfaceId> honest_group_1;
        vector<pair<long,int>> received_init;
        vector<pair<long,int>> received_witness;
        vector<pair<long,int>> broadcast_witness;
        bool delivered = false;
        int witness_threshold;
        int delivery_threshold;

        int check_witness(interfaceId source, int value){
            int counter = 0;
            for (const auto& p : received_witness) {
                if (p.first == source && p.second == value) counter++;
                if (counter >= witness_threshold) return value;
            }

            return -1;
        }

        int check_delivery(interfaceId source, int value){
		    int counter = 0;
            for (const auto& p : received_witness) {
                if (p.first == source && p.second == value) counter++;
                if (counter >= delivery_threshold) return value;
            }
            return -1;
        }

        bool contains(const vector<pair<long,int>>& s, long source){
            for (const auto& p : s){
                if (p.first == source) return true;
            }
            return false;
        }

        bool contains(const vector<pair<long,int>>& s, long source, int value){
            return find(s.begin(), s.end(), std::make_pair(source, value)) != s.end();
        }
        // -----------------------------------------

        // methods that must be defined when deriving from Peer
        ImbsRaynalPeer                             (long);
        ImbsRaynalPeer                             (const ImbsRaynalPeer &rhs);
        ~ImbsRaynalPeer                            ();

        // initialize the configuration of the system
        void                 initParameters(const vector<Peer<ImbsRaynalMessage>*>& _peers, json parameters);
        // perform one step of the Algorithm with the messages in inStream
        void                 performComputation ();
        // perform any calculations needed at the end of a round such as determine throughput (only ran once, not for every peer)
        void                 endOfRound         (const vector<Peer<ImbsRaynalMessage>*>& _peers);
    };

    Simulation<quantas::ImbsRaynalMessage, quantas::ImbsRaynalPeer>* generateSim();
}
#endif /* ImbsRaynalPeer_hpp */
