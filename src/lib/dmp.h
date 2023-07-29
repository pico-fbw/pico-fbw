#ifndef __DMP_H
#define __DMP_H

/**
 * Writes a memory block to the DMP.
 * @param data The data to write.
 * @param dataSize The size of the data.
 * @param bank The bank to write to.
 * @param address The address to write to.
*/
void dmp_writeMemoryBlock(const uint8_t *data, uint16_t dataSize, uint8_t bank, uint8_t address);

#define DMP_CHUNK_SIZE 16
#define DMP_PACKET_SIZE 28
#define DMP_SIZE 3062

extern const unsigned char dmp[DMP_SIZE];

#endif // __DMP_H