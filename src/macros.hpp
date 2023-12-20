#ifndef STM_MACROS_HPP
#define STM_MACROS_HPP


//region GNUC defines
#undef likely
#ifdef __GNUC__
#define likely(prop) \
        __builtin_expect((prop) ? true : false, true /* likely */)
#else
#define likely(prop) \
        (prop)
#endif


#undef unlikely
#ifdef __GNUC__
#define unlikely(prop) \
        __builtin_expect((prop) ? true : false, false /* unlikely */)
#else
#define unlikely(prop) \
        (prop)
#endif


#undef unused
#ifdef __GNUC__
#define unused(variable) \
        variable __attribute__((unused))
#else
#define unused(variable)
    #warning This compiler has no support for GCC attributes
#endif


#undef always_inline
#ifdef __GNUC__
#define always_inline  __attribute__((always_inline))
#else
#define always_inline
    #warning This compiler has no support for GCC attributes
#endif

//endregion

/**
 * Prepares a 64-bit bitmask with a single set bit, at position `position`.
 */
#define BITMASK(position) ((uint64_t) 1 << (position))

/**
 * Sets a bit of `value` at position `position`.
 */
#define SET_BIT(value, position) ((value) | BITMASK(position))

/**
 * Resets a bit of `value` at position `position`.
 */
#define RESET_BIT(value, position) ((value) & ~BITMASK(position))

/**
 * Check whether a bit at position `position` of `value` is set.
 */
#define IS_SET_BIT(value, position) ((value) & BITMASK(position) ? true : false)

/**
 * Check whether a bit at position `position` of `value` is reset.
 */
#define IS_RESET_BIT(value, position) ((value) & BITMASK(position) ? false : true)

#endif
