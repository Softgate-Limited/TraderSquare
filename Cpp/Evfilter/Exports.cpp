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
  const wchar_t *MyMutexName = L"TraderSquareEvfilterMutex";
  const wchar_t *MyAgentName = L"EvfilterAgent";

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

    DWORD bytesRead, bytesWritten;
    while (InternetReadFile(hUrl, ReadBuffer, ReadBufferSize, &bytesRead))
    {
      if (bytesRead == 0)
        break;

      WriteFile(hWriteFile, ReadBuffer, bytesRead, &bytesWritten, NULL);
    }

    return true;
  }

  struct EconomicEvent
  {
    time_t datetime;
    int impact;
    wchar_t country[3];
    wchar_t currency[4];

    EconomicEvent(time_t dt, int imp, const char *cnt, const char *cur)
    {
      datetime = dt;
      impact = imp;

      _bstr_t cntBstr(cnt);
      _bstr_t curBstr(cur);

      memcpy(country, (wchar_t *)cntBstr, 3 * sizeof(wchar_t));
      memcpy(currency, (wchar_t *)curBstr, 4 * sizeof(wchar_t));
    }
  };

  typedef vector<EconomicEvent> EconomicEvents;

  EconomicEvents economicEvents;

  bool ReadDataFile(const std::wstring csvPath)
  {
    // 実際の CSV ファイルは UTF-8 だが、指標名は読み捨てるので、ASCII のつもりで読み込んで問題ない。
    FILE *file;    
    errno_t err = _wfopen_s(&file, csvPath.c_str(), L"r");
    bool returnValue = (err == 0);
    if (returnValue)
    {
      try
      {
        economicEvents.clear();

        char lineBuff[256];
        while (fgets(lineBuff, sizeof(lineBuff), file) != NULL)
        {
          if (strlen(lineBuff) > 28)
          {
            string year(lineBuff, 4);
            string month(lineBuff + 5, 2);
            string day(lineBuff + 8, 2);
            string hour(lineBuff + 11, 2);
            string minute(lineBuff + 14, 2);
            //string second(lineBuff + 17, 2);
            string country(lineBuff + 17, 2);
            string currency(lineBuff + 20, 3);
            string impact(lineBuff + 24, 1);

            tm tm;
            tm.tm_year = atoi(year.c_str()) - 1900;
            tm.tm_mon = atoi(month.c_str()) - 1;
            tm.tm_mday = atoi(day.c_str());
            tm.tm_hour = atoi(hour.c_str());
            tm.tm_min = atoi(minute.c_str());
            tm.tm_sec = 0;
            time_t datetime = _mkgmtime(&tm) - 9 * 60 * 60;
            int imp = atoi(impact.c_str());

            economicEvents.push_back(EconomicEvent(datetime, imp, country.c_str(), currency.c_str()));
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
        
        return FetchDataFile(std::wstring(url), csvPath) && ReadDataFile(csvPath);
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

int __stdcall EventInEffectW(__time64_t datetime, const wchar_t *currency, int impact, int minutesBefore, int minutesAfter)
{
  bool anySymbol = true;
  wchar_t symbolBuf[256];
  int retval = 0;

  try
  {
    if (currency != 0 && currency[0] != L'\0')
    {
      wcscpy_s(symbolBuf, 256, currency);
      _wcsupr_s(symbolBuf, 256);
      anySymbol = false;
    }

    // vector をサーチ

    for (EconomicEvents::const_iterator it = economicEvents.begin(); it != economicEvents.end(); ++it)
    {
      if (it->impact >= impact)
      {
        if ((minutesBefore < 0 || it->datetime - minutesBefore * 60 <= datetime) && (minutesAfter < 0 || it->datetime + minutesAfter * 60 >= datetime))
        {
          if (anySymbol || wcsstr(symbolBuf, it->currency) != 0)
          {
            retval = 1;
            break;
          }
        }
      }

      if (it->datetime - minutesBefore > datetime)
        break;
    }
  }
  catch (...)
  {
  }
  return retval;
}

int __stdcall EventInEffectA(__time32_t datetime, const char *currency, int impact, int minutesBefore, int minutesAfter)
{
  _bstr_t bStr(currency);
  return EventInEffectW((__time64_t)datetime, bStr, impact, minutesBefore, minutesAfter);
}
