#include "parallel_utils.hpp"

namespace parallel_utils {

/*
 * Inicialization Global ID
 */
unsigned Parallel_UniqueID::nextID = 0;

/*
 * Functions
 */
void print_message(const char msg[], Parallel_MessageType type) {
	switch(type) {
		case PARALLEL_MESSAGE_ERROR:
			printf("HPSM API Error: %s", msg);
			break;
			
		case PARALLEL_MESSAGE_WARNING:
			printf("HPSM API Warning: %s", msg);
			break;
	}
	printf("\n");
}

/*
 * Functions Parallel_Backend_Key
 */
void Parallel_Backend_Key::delete_keys() {
	delete[] backend_key;
}

bool Parallel_Backend_Key::has_key(std::string key) {
	for(int i=0; i<backend_key_size; i++) {
		if(backend_key[i].key == key)
			return true;
	}
	
	return false;
}

void Parallel_Backend_Key::insert_key(std::string key) {
	backend_key[backend_key_size].key = key;
	backend_key_size++;
}

} //namespace parallel_utils
