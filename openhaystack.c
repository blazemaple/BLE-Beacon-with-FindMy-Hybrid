#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "openhaystack.h"

static uint8_t addr[6] = { 0xFF, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };

/*
 * set_addr_from_key will set the bluetooth address from the first 6 bytes of the key used to be advertised
 */
void set_addr_from_key(char key[28]) {
	/* copy first 6 bytes */
	addr[5] = key[0] | 0b11000000;
	addr[4] = key[1];
	addr[3] = key[2];
	addr[2] = key[3];
	addr[1] = key[4];
	addr[0] = key[5];
}

/*
 * setAdvertisementKey will setup the key to be advertised
 *
 * @param[in] key public key to be advertised
 * @param[out] bleAddr bluetooth address to setup
 * @param[out] data raw data to advertise
 * 
 * @returns raw data size
 */
uint8_t setAdvertisementKey(char *key, uint8_t **bleAddr, uint8_t **data) {
    set_addr_from_key(key);

    *bleAddr = malloc(sizeof(addr));
    memcpy(*bleAddr, addr, sizeof(addr));
}
