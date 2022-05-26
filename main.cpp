/*
 * Author: ixplaayer
 * https://guidedhacking.com/threads/csgo-scoreboard-wallhack.14352/
 */
#include <iostream>
#include <Windows.h>
#include "ProcMan.h"
#include "csgo.hpp"

using namespace hazedumper::netvars;
using namespace hazedumper::signatures;

uintptr_t procID;
uintptr_t moduleBase;
HANDLE hProcess = 0;

RECT m_Rect;

uintptr_t ClocalPlayer;
int myTeam;

float EnemyXY[3];
float entityPosition[3];
float myPosition[3];
float closestEntity[3];

typedef struct
{
    float flMatrix[4][4];
}WorldToScreenMatrix_t;
WorldToScreenMatrix_t WorldToScreenMatrix;

float Get3dDistance(float* myCoords, float* enemyCoords)
{
    return sqrt(
        pow((enemyCoords[0] - myCoords[0]), 2) +
        pow((enemyCoords[1] - myCoords[1]), 2) +
        pow((enemyCoords[2] - myCoords[2]), 2));

}

bool WorldToScreen(float* from, float* to)
{
    float w = 0.0f;

    to[0] = WorldToScreenMatrix.flMatrix[0][0] * from[0] + WorldToScreenMatrix.flMatrix[0][1] * from[1] + WorldToScreenMatrix.flMatrix[0][2] * from[2] + WorldToScreenMatrix.flMatrix[0][3];
    to[1] = WorldToScreenMatrix.flMatrix[1][0] * from[0] + WorldToScreenMatrix.flMatrix[1][1] * from[1] + WorldToScreenMatrix.flMatrix[1][2] * from[2] + WorldToScreenMatrix.flMatrix[1][3];
    w = WorldToScreenMatrix.flMatrix[3][0] * from[0] + WorldToScreenMatrix.flMatrix[3][1] * from[1] + WorldToScreenMatrix.flMatrix[3][2] * from[2] + WorldToScreenMatrix.flMatrix[3][3];

    if (w < 0.01f)
        return false;

    float invw = 1.0f / w;
    to[0] *= invw;
    to[1] *= invw;

    int width = (int)(m_Rect.right - m_Rect.left);
    int height = (int)(m_Rect.bottom - m_Rect.top);

    float x = width / 2;
    float y = height / 2;

    x += 0.5 * to[0] * width + 0.5;
    y -= 0.5 * to[1] * height + 0.5;

    to[0] = x + m_Rect.left;
    to[1] = y + m_Rect.top;

    return true;
}

int main()
{
    procID = getProcID(L"csgo.exe");
    moduleBase = getModuleBaseAddress(procID, L"client.dll");
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procID);
    GetWindowRect(FindWindow(NULL, L"Counter-Strike: Global Offensive"), &m_Rect);


    while (!GetAsyncKeyState(VK_INSERT))
    {
        ClocalPlayer = readMem<uintptr_t>(hProcess, moduleBase + dwLocalPlayer);
        if (ClocalPlayer != NULL)
        {
            myTeam = readMem<int>(hProcess, ClocalPlayer + m_iTeamNum);
            if (GetAsyncKeyState(VK_TAB))
            {
                int closestEntityDistance = 10000000000;
                for (short int i = 0; i < 20; i++)
                {
                    int entity = readMem<int>(hProcess, moduleBase + dwEntityList + i * 0x10);
                    bool dormant = readMem<bool>(hProcess, entity + m_bDormant);
                    if (entity != NULL && dormant == false)
                    {
                        int entityTeam = readMem<int>(hProcess, entity + m_iTeamNum);
                        if (entityTeam != myTeam)
                        {
                            ReadProcessMemory(hProcess, (PBYTE*)(entity + m_vecOrigin), &entityPosition, sizeof(float[3]), 0);

                            ReadProcessMemory(hProcess, (PBYTE*)(ClocalPlayer + m_vecOrigin), &myPosition, sizeof(float[3]), 0);

                            ReadProcessMemory(hProcess, (PBYTE*)(moduleBase + dwViewMatrix), &WorldToScreenMatrix, sizeof(WorldToScreenMatrix), 0);

                            if (WorldToScreen(entityPosition, EnemyXY))
                            {
                                float distance = Get3dDistance(myPosition, entityPosition);
                                if (distance < closestEntityDistance)
                                {
                                    closestEntityDistance = distance;
                                    for (int i = 0; i < 3; i++)
                                    {
                                        closestEntity[i] = entityPosition[i];
                                    }
                                }
                            }
                        }
                    }
                }
                if (WorldToScreen(closestEntity, EnemyXY))
                {
                    SetCursorPos(EnemyXY[0], EnemyXY[1] - 50);
                }
            }
        }
        Sleep(10);
    }
    return 0;
}