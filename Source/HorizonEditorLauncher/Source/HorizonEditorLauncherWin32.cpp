#include "Editor/HorizonEditorModule.h"

#include <Windows.h>
#include <shellapi.h>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(pCmdLine, &argc);

    // TODO: parse arguments here

    LocalFree(argv);

    //int exit = HorizonEditorMain();
    //return exit;

    return 0;
}

int main(int argc, char** argv)
{
    int exit = HorizonEditorMain(argc, argv);
    return exit;
}