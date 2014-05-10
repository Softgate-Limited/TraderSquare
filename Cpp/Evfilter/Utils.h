#pragma once

#include <Windows.h>
#include <wininet.h>
#include <string>

template <class T> class MyAutoArray
{
public:
  MyAutoArray(size_t size) : m_ptr(new T[size]) {}
  ~MyAutoArray() { delete [] m_ptr; }
  operator T* () { return m_ptr; }

private:
  T *m_ptr;
};

class MyHandle
{
public:
  MyHandle(HANDLE handle, HANDLE invalidValue) { hHandle = handle; hInvalidValue = invalidValue; }
  ~MyHandle() { if (hHandle != hInvalidValue) CloseHandle(hHandle); }
  operator HANDLE () { return hHandle; }

private:
  MyHandle(MyHandle &);
  MyHandle& operator=(MyHandle &);

  HANDLE hHandle;
  HANDLE hInvalidValue;
};

class MyInternetHandle
{
public:
  MyInternetHandle(HINTERNET handle) { hHandle = handle; }
  ~MyInternetHandle() { if (hHandle != NULL) InternetCloseHandle(hHandle); }
  operator HINTERNET () { return hHandle; }

private:
  MyInternetHandle(MyInternetHandle &);
  MyInternetHandle& operator=(MyInternetHandle &);

  HINTERNET hHandle;
};

class MutexReleaser
{
public:
  MutexReleaser(HANDLE hMutex, bool abandoned) { m_hMutex = hMutex; m_abandoned = abandoned; }
  ~MutexReleaser() { if (!m_abandoned) ReleaseMutex(m_hMutex); }

private:
  MutexReleaser(MutexReleaser &);
  MutexReleaser& operator=(MutexReleaser &);

  bool m_abandoned;
  HANDLE m_hMutex;
};

std::wstring ExtractDirectory(const std::wstring &path, bool inclusive)
{
  const std::wstring::size_type pos = path.find_last_of('\\');
  return (std::wstring::npos != pos) ? path.substr(0, pos + (inclusive ? 1 : 0)) : L"";
}
