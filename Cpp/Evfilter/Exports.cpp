#include <Windows.h>
#include <time.h>
#include <comutil.h>
#include "Utils.h"

#pragma comment(lib,"comsuppw.lib")
#pragma comment(lib,"wininet.lib")

extern HINSTANCE hDllInstance;

namespace
{
  const DWORD MyTimeout = 30000;
  const wchar_t *MyMutexName = L"TraderSquareEvfilterMutex";
  const wchar_t *MyAgentName = L"EvfilterAgent";

  const int ReadBufferSize = 1024 * 8;
  char ReadBuffer[ReadBufferSize];

  bool FetchAndReadDataFile(const std::wstring &url, const std::wstring csvPath)
  {
    MyInternetHandle hInternet(InternetOpen(MyAgentName, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0));
    if (hInternet == NULL)
      return false;

    MyInternetHandle hUrl(InternetOpenUrl(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_RAW_DATA | INTERNET_FLAG_PRAGMA_NOCACHE, 0));
    if (hUrl == NULL)
      return false;

    MyHandle hWriteFile(CreateFile(csvPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL), INVALID_HANDLE_VALUE);
    if (hWriteFile == INVALID_HANDLE_VALUE)
      return false;

    DWORD bytesRead, bytesWritten;
    while (InternetReadFile(hUrl, ReadBuffer, ReadBufferSize, &bytesRead))
    {
      if (bytesRead == 0)
        break;

      WriteFile(hWriteFile, ReadBuffer, bytesRead, &bytesWritten, NULL);
    }

    return true;
  }
}

int __stdcall EventReloadW(const wchar_t *url)
{
  wchar_t dllPathBuff[MAX_PATH];
  if (GetModuleFileName(hDllInstance, dllPathBuff, MAX_PATH) > 0)
  {
    std::wstring dllPath(dllPathBuff);
    std::wstring csvPath = ExtractDirectory(ExtractDirectory(dllPathBuff, false), false) + L"\\files\\EvfilterEvents.csv";

    MyHandle hMutex(CreateMutex(NULL, FALSE, MyMutexName), NULL);
    if (hMutex != NULL)
    {
      DWORD waitResult = WaitForSingleObject(hMutex, MyTimeout);
      if (waitResult == WAIT_OBJECT_0 || waitResult == WAIT_ABANDONED)
      {
        MutexReleaser mutexReleaser(hMutex, waitResult == WAIT_ABANDONED);
        
        return FetchAndReadDataFile(std::wstring(url), csvPath);
      }
    }
  }
  return 0;
}

int __stdcall EventReloadA(const char *url)
{
  _bstr_t bStr(url);
  return EventReloadW(bStr);
}

int __stdcall EventInEffectW(__time64_t datetime, const wchar_t *currency, int minutesBefore, int minutesAfter)
{
  return 0;
}

int __stdcall EventInEffectA(__time32_t datetime, const char *currency, int minutesBefore, int minutesAfter)
{
  return 0;
}
