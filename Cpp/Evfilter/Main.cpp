#include <Windows.h>

HINSTANCE hDllInstance;

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD reason, LPVOID pVoid)
{
  if (reason == DLL_PROCESS_ATTACH)
  {
    // ���� DLL �̃��W���[���n���h����ۑ����Ă���
    hDllInstance = hInstance;
    DisableThreadLibraryCalls(hDllInstance);
  }
  else if (reason == DLL_PROCESS_DETACH)
  {
    // �������Ȃ�
  }
  return 1;
}
