#include  <Windows.h>

extern HINSTANCE hDllInstance;

int __stdcall EventReloadA(const char *url)
{
  return 1;
}

int __stdcall EventReloadW(const wchar_t *url)
{
  return 1;
}

int __stdcall EventInEffectA(const char *currency, int minutesBefore, int minutesAfter)
{
  return 0;
}

int __stdcall EventInEffectW(const wchar_t *currency, int minutesBefore, int minutesAfter)
{
  return 0;
}
