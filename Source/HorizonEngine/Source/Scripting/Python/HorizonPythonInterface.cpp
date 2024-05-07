#include <Python.h>

#include <filesystem>

#include "HorizonPythonInterface.h"

void HorizonPythonInitialize(int argc, char** argv)
{
   std::filesystem::path path = argv[0];
    std::wstring pythonHome = path.parent_path().append("python310").wstring();

   Py_SetPythonHome(pythonHome.c_str());

    Py_Initialize();
    PyRun_SimpleString("print('hello world')\n");
}

void HorizonPythonFinalize()
{
    Py_Finalize();
}
