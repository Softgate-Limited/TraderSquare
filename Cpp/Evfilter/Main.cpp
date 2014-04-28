#include <Windows.h>

HINSTANCE hDllInstance;

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD reason, LPVOID pVoid)
{
  if (reason == DLL_PROCESS_ATTACH)
  {
    // この DLL のモジュールハンドルを保存しておく
    hDllInstance = hInstance;
    DisableThreadLibraryCalls(hDllInstance);
  }
  else if (reason == DLL_PROCESS_DETACH)
  {
    // 何もしない
  }
  return 1;
}
