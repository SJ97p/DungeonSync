#include "Application.h"
#include <Windows.h>

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE,
    PWSTR,
    int showCommand)
{
    DungeonSync::Application application{
        instance,
        showCommand
    };

    return application.Run();
}