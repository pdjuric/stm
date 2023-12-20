#ifndef STM_CONFIG_HPP
#define STM_CONFIG_HPP


// word size in bytes
#define ALIGNMENT 8

// log_2 ALIGNMENT - number of LSB zeros
#define ALIGNMENT_BITS 3

// memory block contains 2^VIRTUAL_ADDR_OFFSET_BITS words
#define VIRTUAL_ADDR_OFFSET_BITS 10

// memory region contains 2^VIRTUAL_ADDR_BLOCK_BITS blocks
#define VIRTUAL_ADDR_BLOCK_BITS 9

#endif