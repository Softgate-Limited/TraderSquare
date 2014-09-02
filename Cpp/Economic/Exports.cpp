#include <Windows.h>
#include <time.h>
#include <comutil.h>
#include <string>
#include <vector>
#include "Utils.h"

#pragma comment(lib,"comsuppw.lib")
#pragma comment(lib,"wininet.lib")

extern HINSTANCE hDllInstance;

using namespace std;

namespace
{
  const DWORD MyTimeout = 30000;
  const wchar_t *MyMutexName = L"TraderSquareEconomicMutex";
  const wchar_t *MyAgentName = L"EconomicAgent";

  const int ReadBufferSize = 1024 * 8;
  char ReadBuffer[ReadBufferSize];

  bool FetchDataFile(const std::wstring &url, const std::wstring csvPath)
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

    DWORD bytesRead, bytesWritten, bytesReadTotal = 0;
    while (InternetReadFile(hUrl, ReadBuffer, ReadBufferSize, &bytesRead))
    {
      bytesReadTotal += bytesRead;
      if (bytesRead == 0)
        break;

      WriteFile(hWriteFile, ReadBuffer, bytesRead, &bytesWritten, NULL);
      if (bytesRead != bytesWritten)
        return false;
    }

    return bytesReadTotal > 0;
  }

  struct EconomicEvent
  {
    time_t datetime;
    int impact;
    wchar_t country[3];
    wchar_t currency[4];
    wstring title;

    EconomicEvent(time_t dt, int imp, const wchar_t *cnt, const wchar_t *cur, const wstring &tit)
    {
      datetime = dt;
      impact = imp;

      //_bstr_t cntBstr(cnt);
      //_bstr_t curBstr(cur);

      memcpy(country, cnt, 3 * sizeof(wchar_t));
      memcpy(currency, cur, 4 * sizeof(wchar_t));

      title.assign(tit);

      OutputDebugStringW(title.c_str());
    }
  };

  typedef vector<EconomicEvent> EconomicEvents;

  EconomicEvents economicEvents;

  bool ReadDataFile(const std::wstring csvPath)
  {
    // 実際の CSV ファイルは UTF-8 だが、指標名は読み捨てるので、ASCII のつもりで読み込んで問題ない。
    FILE *file;    
    errno_t err = _wfopen_s(&file, csvPath.c_str(), L"r,ccs=UTF-8");
    bool returnValue = (err == 0);
    if (returnValue)
    {
      try
      {
        economicEvents.clear();

        wchar_t lineBuff[256];
        while (fgetws(lineBuff, 256, file) != NULL)
        {
          if (wcslen(lineBuff) > 28)
          {
            wstring year(lineBuff, 4);
            wstring month(lineBuff + 5, 2);
            wstring day(lineBuff + 8, 2);
            wstring hour(lineBuff + 11, 2);
            wstring minute(lineBuff + 14, 2);
            //string second(lineBuff + 17, 2);
            wstring country(lineBuff + 17, 2);
            wstring currency(lineBuff + 20, 3);
            wstring impact(lineBuff + 24, 1);

            tm tm;
            tm.tm_year = _wtoi(year.c_str()) - 1900;
            tm.tm_mon = _wtoi(month.c_str()) - 1;
            tm.tm_mday = _wtoi(day.c_str());
            tm.tm_hour = _wtoi(hour.c_str());
            tm.tm_min = _wtoi(minute.c_str());
            tm.tm_sec = 0;
            time_t datetime = _mkgmtime(&tm);
            int imp = _wtoi(impact.c_str());

            wstring line(lineBuff + 26);
            int pos = line.find_first_of(L',');

            wstring title(line.begin(), pos < 0 ? line.end() : (line.begin() + pos));

            economicEvents.push_back(EconomicEvent(datetime, imp, country.c_str(), currency.c_str(), title));
          }
        }
      }
      catch (...)
      {
        returnValue = false;
      }
      fclose(file);
    }
    return returnValue && economicEvents.size() > 0;
  }
}

int __stdcall EconomicReload(const wchar_t *url)
{
  wchar_t dllPathBuff[MAX_PATH];
  if (GetModuleFileName(hDllInstance, dllPathBuff, MAX_PATH) > 0)
  {
    std::wstring dllPath(dllPathBuff);
    std::wstring csvPath = ExtractDirectory(ExtractDirectory(dllPathBuff, false), false) + L"\\files\\EconomicEvents.csv";

    MyHandle hMutex(CreateMutex(NULL, FALSE, MyMutexName), NULL);
    if (hMutex != NULL)
    {
      DWORD waitResult = WaitForSingleObject(hMutex, MyTimeout);
      if (waitResult == WAIT_OBJECT_0 || waitResult == WAIT_ABANDONED)
      {
        MutexReleaser mutexReleaser(hMutex, waitResult == WAIT_ABANDONED);
        
        return FetchDataFile(std::wstring(url), csvPath) && ReadDataFile(csvPath);
      }
    }
  }
  return 0;
}

// 現在の日時から次のイベントを探して、そのインデックスに offset を加えたものを返す
int __stdcall EconomicGetIndex(int offset)
{
  try
  {
    time_t jstNow = time(0) + 9 * 60 * 60;
    int index = -1;

    // vector をサーチ
    for (int i = 0; i < economicEvents.size(); ++i)
    {
      if (economicEvents[i].impact > 0)
      {
        if (economicEvents[i].datetime > jstNow)
        {
          index = i;
          break;
        }
      }
    }

    if (index >= 0 && index + offset < economicEvents.size())
      return index + offset;
  }
  catch (...)
  {
  }
  return -1;
}

__int64 __stdcall EconomicGetDateTime(int index)
{
  if (index >= 0)
    return economicEvents.at(index).datetime;
  return 0;
}

int __stdcall EconomicGetImpact(int index)
{
  if (index >= 0)
    return economicEvents.at(index).impact;
  return 0;
}

const wchar_t * __stdcall EconomicGetCurrency(int index)
{
  if (index >= 0)
    return economicEvents.at(index).currency;
  return 0;
}

const wchar_t * __stdcall EconomicGetTitle(int index)
{
  if (index >= 0)
    return economicEvents.at(index).title.c_str();
  return 0;
}
