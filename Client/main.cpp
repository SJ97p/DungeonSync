#include "Application.h"

#include <Windows.h>
#include <objbase.h>
#include <cstdlib>

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE,
    PWSTR,
    int showCommand)
{
    const HRESULT comResult =
        CoInitializeEx(
            nullptr,
            COINIT_MULTITHREADED);

    if (FAILED(comResult))
    {
        MessageBoxW(
            nullptr,
            L"Failed to initialize COM.",
            L"DungeonSync Error",
            MB_OK | MB_ICONERROR);

        return EXIT_FAILURE;
    }

    int exitCode = EXIT_FAILURE;

    {
        DungeonSync::Application application{
            instance,
            showCommand
        };

        exitCode = application.Run();
    }

    CoUninitialize();

    return exitCode;
}