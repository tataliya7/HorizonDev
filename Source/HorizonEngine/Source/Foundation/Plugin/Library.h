#pragma once

namespace Horizon
{
    class Library
    {
    public:

        ~Library();

        void Load();
        void Free();
    };
}