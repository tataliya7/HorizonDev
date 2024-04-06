#pragma once

#include <gtest/gtest.h>

#include "Python/PythonModule.h"

namespace HE
{
    TEST(PythonTest, Init)
    {
        HorizonPythonInitialize(__argc, __argv);
        HorizonPythonFinalize();
    }
}