#pragma once

#include "Foundation/StdHeaders.h"
#include "Foundation/Definitions.h"
#include "Foundation/FundamentalTypes.h"

namespace Horizon
{
    /**
     * @brief Universally Unique Identifier according to RFC4122.
     */
    struct UUID
    {
        /* Nil UUID. */
        static constexpr UUID NilUUID()
        {
            constexpr UUID nil = { 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0, 0 } };
            return nil;
        }

        /** GenerateUUID a new UUID. */
        static UUID GenerateUUID();

        /** Convert UUID to string. */
        static std::string ToString(const UUID& uuid, bool uppercase = false);

        /** Convert string to UUID. */
        static UUID FromString(const char* string);

        /** Convert string to UUID. */
        static UUID FromString(const std::string& string);

        /* The length of a UUID string. */
        static constexpr uint32 StringRepresentationLength = 36;

        /* The low field of the timestamp, 4 octets. */
        uint32 timeLow;

        /* The middle field of the timestamp, 2 octets. */
        uint16 timeMid;

        /* The high field of the timestamp multiplexed with the version number, 2 octets. */
        uint16 timeHighAndVersion;

        /* The high field of the clock sequence multiplexed with the variant, 1 octet. */
        uint8 clockSeqHighAndReserved;

        /* The low field of the clock sequence, 1 octet. */
        uint8 clockSeqLow;

        /* The spatially unique node identifier, 6 octets. */
        uint8 node[6];

        /* Whether this UUID is a nil-valued UUID. */
        bool IsNil() const;
    };

    inline bool operator==(const UUID& lhs, const UUID& rhs)
    {
        return std::memcmp(&lhs, &rhs, sizeof(UUID)) == 0;
    }

    inline bool operator!=(const UUID& lhs, const UUID& rhs)
    {
        return !(lhs == rhs);
    }
}