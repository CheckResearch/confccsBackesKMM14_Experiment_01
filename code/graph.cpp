#include "graph.h" // class's header file
#include "functions.h"
#include <fstream>
#include<iostream>
#include <sstream>
#include <assert.h>
#include <boost/algorithm/string.hpp>
#include <boost/thread/thread.hpp> 
#include <boost/regex.hpp> 


map<string,int>* Graph::getWeights()
{
	return (this->weights);
}


using namespace std;
// class constructor
Graph::Graph(string consensus_filename, string server_descriptor_db_filename)
{
	if(server_descriptor_db_filename == "")
	{
		boost::regex yyyyMM("\\d\\d\\d\\d-\\d\\d");
		boost::match_results<std::string::iterator> what;
		if(regex_search(consensus_filename.begin(), consensus_filename.end(), what, yyyyMM, boost::match_default))
			server_descriptor_db_filename = "../data/server_descriptors-" + what[0] + ".sqlite";
		else
			server_descriptor_db_filename = "../data/server_descriptors-2014-02.sqlite";
	}
	this->bestSupport=0;
	this->totalSupportedExitBandwidth=0;
	cout << "I try to read the file \"" << consensus_filename << "\"" << std::endl;
	ifstream myfile (consensus_filename.c_str());
	string line;
	bool shouldsave = false;

	string name = "<error> no name given.";
	string IP = "<error> no IP given.";
	string published = "";
	bool guard  = 0;
	bool exit   = 0;
	bool badexit   = 0;
	bool fast   = 0;
	bool valid  = 0;
	bool stable = 0;
	bool running= 0;
	int bandwidth;
	string policy = "";
	this->weights = new map<string,int>();

	sqlite3 *database_handle;
	int rc = sqlite3_open(server_descriptor_db_filename.c_str(), &database_handle);
	//std::cout << "Trying to open " << server_descriptor_db_filename << std::endl;
	/* if opening the database did not succeed output some error message */
	if( rc != 0 )
	{
		string errorString = "Cannot open database:" + string(sqlite3_errmsg(database_handle));
		sqlite3_close(database_handle);
		throw runtime_error("Cannot open database:" + errorString);
	}
	
	if (myfile.is_open())
	{
		while ( getline (myfile,line) )
		{
			vector<string> tokens;
			tokens = tokenize(line,' ');
			
			if(tokens[0] == "r") // Router information.
			{
				if(shouldsave) // Save the previous router first (if there was any)
				{
					this->addNode(name,IP,published,guard,exit,badexit,fast,valid,stable,running,bandwidth, policy, database_handle);
					guard =0;
					exit =0;
					name="<error> no name given.";
					IP="<error> no IP given.";
					policy="";
					bandwidth = 0;
				}
				name = tokens[1];
				published = tokens[4] + " " + tokens[5];
				IP = tokens[6];
				shouldsave=true;
			}else if(tokens[0] == "s") // Tags
			{
				guard = findToken(tokens,"Guard");
				exit = findToken(tokens,"Exit");
				badexit = findToken(tokens,"BadExit");
				valid = findToken(tokens,"Valid");
				stable = findToken(tokens,"Stable");
				running = findToken(tokens,"Running");
				fast = findToken(tokens,"Fast");
			}else if(tokens[0] == "w") // Bandwidth
			{
				vector<string> tempvect;
				tempvect = tokenize(tokens[1],'=');
				istringstream(tempvect[1]) >> bandwidth;
			}else if(tokens[0] == "p") // Policy
			{
				policy = tokens[1] + "," + tokens[2];
			}else if(tokens[0]=="bandwidth-weights") // The overall Bandwidth-Weights
			{
				cout << "Reading weights" << endl;
				// Example:
				//Wbd=1395 Wbe=0 Wbg=3690 Wbm=10000 Wdb=10000 Web=10000 Wed=7209 Wee=10000 Weg=7209 Wem=10000 Wgb=10000 Wgd=1395 Wgg=6310 Wgm=6310 Wmb=10000 Wmd=1395 Wme=0 Wmg=3690 Wmm=10000
				string key;
				int w;
				for(unsigned int i=1; i < tokens.size();i++)
				{
					vector<string> tempvect = tokenize(tokens[i],'=');
					key = tempvect[0];
					istringstream(tempvect[1]) >> w;
					(*this->weights)[key]=w;
				}
			}
		}
		if(shouldsave)
		{
			this->addNode(name,IP,published,guard,exit,badexit,fast,valid,stable,running,bandwidth,policy, database_handle);
		}
		myfile.close();
		
		sqlite3_close(database_handle);
	}
	cout << "Assigning families .." << endl;
	this->family_fingerprint_map = this->constructFamilyFingerprintMapAndReadMoreInfo(server_descriptor_db_filename.c_str());
	if ( this->family_fingerprint_map == NULL )
	{
		cout << "Error: The construction of the family fingerprint map failed.";
	}
	this->assignFamilies();
	if(VERBOSE)cout << ".. done" << endl;

	for(unsigned int i=0; i < this->size(); i++)
	{
		assert(this->nodes[i].myPos==i);
		for(unsigned int j=0; j < this->size(); j++)
		{
			this->myRelatedArray[i].push_back(false);
			if(this->nodes[i].isSubnet(&this->nodes[j]))
			{
				this->nodes[i].addIPFamilyMember(&this->nodes[j]);
				this->myRelatedArray[i][j]=true;
			}else{
				if(this->nodes[i].isFamily(&this->nodes[j]))
					this->myRelatedArray[i][j]=true;
			}
			if(i==j)
				assert(this->myRelatedArray[i][j]);
		}
	}
	for(unsigned int i=0; i < (this->size() / SET_SIZE)+1; i++)
	{
		this->graph_mutex.push_back(new boost::timed_mutex());
	}
}

// class destructor
Graph::~Graph()
{

}

const unsigned int Graph::size()
{
	return this->nodes.size();
}

int Graph::getBestSupport()
{
	return this->bestSupport;
}

int Graph::getTotalSupportedExitBandwidth()
{
	//This value is set by Graph::computeExitSupport. 
	//If should never be retrieved before it is calculated.
	assert(this->totalSupportedExitBandwidth>0);
	return this->totalSupportedExitBandwidth;
}


void Graph::addNode(string name, string IP, string published, bool guard, bool exit, bool badexit, bool fast, bool valid, bool stable, bool running, int bandwidth, string policy, sqlite3* database_handle)
{
	Node* n = new Node(name,IP,published,guard,exit,badexit,fast,valid,stable,running, bandwidth, policy, database_handle);
	this->nodes.push_back(*n);
	this->nodes[this->nodes.size()-1].myPos = this->nodes.size()-1;
	this->myRelatedArray.push_back(vector<bool>());
}

Node* Graph::getNode(int i)
{
	return &(this->nodes[i]);
}

void Graph::print()
{
	for(size_t i = 0; i < this->nodes.size(); ++i)
	{
		this->nodes[i].print();
	}
}

FamilyFingerprintMap *Graph::constructFamilyFingerprintMapAndReadMoreInfo(const char* server_descriptor_db_filename)
{
	sqlite3 *database_handle;
	int rc = sqlite3_open(server_descriptor_db_filename, &database_handle);
	std::cout << "Trying to open " << server_descriptor_db_filename << std::endl;
	/* if opening the database did not succeed output some error message */
	if( rc != 0 )
	{
		string errorString = "Cannot open database:" + string(sqlite3_errmsg(database_handle));
		sqlite3_close(database_handle);
		throw runtime_error("Cannot open database:" + errorString);
	} else
	{
		string name_list = "(";
		for (unsigned int i=0; i < this->size(); i++)
		{	
			if (i!=0)
				name_list = name_list + ", ";
			name_list = name_list + "\'" + this->getNode(i)->getID() + "\'";
		}
		name_list = name_list + ")";
		
		string sql_query = "SELECT DISTINCT hash, fingerprint, family FROM familyview WHERE hash in " + name_list + ";";
		
		QueryResultP result_vector = execute_sqlite_query(database_handle, sql_query);
		FamilyFingerprintMap *result_map = new FamilyFingerprintMap();
		Hash hash = "";
		
		Row family_fingerprints;
		string tmp;
		
		// Convert the query result to a map.
		for (QueryResult::iterator it = result_vector->begin(); it != result_vector->end(); ++it)
		{
			// The hash is of the form nickname@address published: e.g., 
			// TorAustralisVII@192.227.247.246 2014-02-17 06:04:23
			hash = it->at(0);
			tmp = it->at(2); // Convert the Python-encoding of the family into a vector
			boost::erase_all(tmp, "set([");
			boost::erase_all(tmp, "])");
			boost::erase_all(tmp, "u'$");
			boost::erase_all(tmp, "u'");
			boost::erase_all(tmp, "'");
			boost::erase_all(tmp, " ");
			boost::split(family_fingerprints, tmp, boost::is_any_of(","));
			result_map->insert(pair<Hash,pair<string, Row> >(hash, pair<string,Row>(it->at(1),family_fingerprints)));
		}
		delete result_vector;
		sqlite3_close(database_handle);
		return result_map;
	}
}

typedef map<Hash,Node*> FamilyMap;

void Graph::assignFamilies()
{
	FamilyMap fingerprint_node_map;
	FamilyMap nickname_node_map;
	FamilyFingerprintMap::iterator res;
	
	Node *current_node = NULL;

	// First run: Create a map from fingerprints to node pointer.
	for (unsigned int i=0; i < this->size(); i++)
	{
		current_node = this->getNode(i);

		res = this->family_fingerprint_map->find(current_node->getID());
		if (res != this->family_fingerprint_map->end())
		{
			// cout << "Fingerprint: " << res->second.first << endl;
			current_node->setBelievedFamily(res->second.second);
			current_node->setFingerprint(res->second.first);
			fingerprint_node_map.insert(pair<Hash,Node*>(current_node->getFingerprint(),current_node));
			nickname_node_map.insert(pair<Hash,Node*>(current_node->getName(),current_node));
		}
	}

	// Second run: Add those nodes as family that also consider you as family.
	// For every node: current_node
	for (unsigned int i=0; i < this->size(); i++)
	{
		current_node = this->getNode(i);
		Row vector_family_members_fingerprint = current_node->getBelievedFamily();

		// For every fingerprint of a member of node i
		for (Row::iterator family_member_fingerprint = vector_family_members_fingerprint.begin(); 
		     family_member_fingerprint != vector_family_members_fingerprint.end(); ++family_member_fingerprint)
		{
			FamilyMap::iterator iterator_family_member = fingerprint_node_map.find(*family_member_fingerprint);

			Node* family_member = NULL;

			// If this entry exists.
			if (iterator_family_member != fingerprint_node_map.end())
			{
				family_member = iterator_family_member->second;
				
				/* If the current node is also in the family of this family_member, enter the node pointers.
				   For other containers, the following lines would just be replaced by the "find" operation. */

			} else
			{
				// Check whether there is a node pointer for this string as a nickname.
				FamilyMap::iterator iterator_family_member_nickname = nickname_node_map.find(*family_member_fingerprint);
			
				// If this entry exists.
				if (iterator_family_member_nickname != nickname_node_map.end())
				{
					family_member = iterator_family_member_nickname->second;
				}
			}
			
			if (family_member != NULL)
			{
				Row memberlist = family_member->getBelievedFamily();
				
				bool found = false;
				for (Row::iterator fingerprint_entry = memberlist.begin(); fingerprint_entry != memberlist.end(); ++fingerprint_entry)
				{
					if (current_node->getFingerprint() == *fingerprint_entry || current_node->getName() == *fingerprint_entry)
					{
						found = true;
					}
				}
				
				if (found)
				{
					if (current_node != family_member && !current_node->isFamily(family_member))
       				{
						current_node->addFamilyMember(family_member);
						current_node->addFamilyOfMember(family_member);
					}
				}
			}
		}
		
	}
}

unsigned int Graph::getNumberEntryNodes(Ports* ports, Preferences* prefs)
{
	unsigned int j = 0;
	for (unsigned int i=0; i < this->size(); i++)
	{
		if (this->getNode(i)->possibleEntry(ports, prefs))
			j++;
	}
	return j;
}

unsigned int Graph::getNumberExitNodes(Ports* ports, Preferences* prefs)
{
	unsigned int j = 0;
	for (unsigned int i=0; i < this->size(); i++)
	{
		if (this->getNode(i)->possibleExit(ports, prefs))
			j++;
	}
	return j;
}

unsigned int Graph::getFractionSizeEntry(double fraction, Ports* ports, Preferences* prefs)
{
	return ceil(fraction * this->getNumberEntryNodes(ports, prefs));
}

unsigned int Graph::getFractionSizeExit(double fraction, Ports* ports, Preferences* prefs)
{
	return ceil(fraction * this->getNumberExitNodes(ports, prefs));
}

unsigned int Graph::getFractionSize(double fraction, Ports* ports, Preferences* prefs)
{
	return ceil(fraction * this->size());
}
