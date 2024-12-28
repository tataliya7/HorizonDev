#include "Foundation/UUID.h"

#include <windows.h>
#include <objbase.h>

namespace Horizon
{
    /** A UUID should be represented as a 128-bit integer. */
    static_assert(sizeof(UUID) == 16, "Unexpected UUID size");

    UUID UUID::GenerateUUID()
    {
        // @todo Port this function across platforms.
        UUID uuid;
        assert(CoCreateGuid(reinterpret_cast<::GUID*>(&uuid)) == S_OK);
        return uuid;
    }

    std::string UUID::ToString(const UUID& uuid, bool uppercase)
    {
        // Standards recommend that the hexadecimal representation used in all human-readable formats be restricted to lower-case letters.
        std::string string("00000000-0000-0000-0000-000000000000");
        const int bufferLength = std::sprintf(
            string.data(),
            uppercase ? "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X" : "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            uuid.timeLow,
            uuid.timeMid,
            uuid.timeHighAndVersion,
            uuid.clockSeqHighAndReserved,
            uuid.clockSeqLow,
            uuid.node[0],
            uuid.node[1],
            uuid.node[2],
            uuid.node[3],
            uuid.node[4],
            uuid.node[5]);
        assert(bufferLength == UUID::StringRepresentationLength);
        return string;
    }

    UUID UUID::FromString(const char* string)
    {
        constexpr UUID nil = NilUUID();

        if (strlen(string) != UUID::StringRepresentationLength)
        {
            return nil;
        }

        UUID uuid;
        const int fieldCount = std::sscanf(
            string,
            "%8x-%4hx-%4hx-%2hhx%2hhx-%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx", // The hexadecimal values "a" through "f" are case insensitive on input.
            &uuid.timeLow,
            &uuid.timeMid,
            &uuid.timeHighAndVersion,
            &uuid.clockSeqHighAndReserved,
            &uuid.clockSeqLow,
            &uuid.node[0],
            &uuid.node[1],
            &uuid.node[2],
            &uuid.node[3],
            &uuid.node[4],
            &uuid.node[5]);

        // Return nil if the format is invalid.
        if (fieldCount != 11)
        {
            return nil;
        }

        return uuid;
    }

    UUID UUID::FromString(const std::string& string)
    {
        return FromString(string.data());
    }

    bool UUID::IsNil() const
    {
        constexpr UUID nil = NilUUID();
        return *this == nil;
    }
}