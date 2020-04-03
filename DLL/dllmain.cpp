/* MIT License
Copyright (c) 2020 jedi95

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "Windows.h"
#include "detours.h"

bool checked = false;
bool over32 = false;

void check() {
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	int numCPU = sysinfo.dwNumberOfProcessors;
	if (numCPU > 32) {
		over32 = true;
	}
	checked = true;
}

BOOL(WINAPI* pGetProcessAffinityMask) (HANDLE, PDWORD_PTR, PDWORD_PTR) = GetProcessAffinityMask;

BOOL(WINAPI* pSetProcessAffinityMask) (HANDLE, DWORD_PTR) = SetProcessAffinityMask;

DWORD_PTR(WINAPI* pSetThreadAffinityMask) (HANDLE, DWORD_PTR) = SetThreadAffinityMask;

LONG(WINAPI* pChange) (LPCTSTR, DEVMODE*, HWND, DWORD, LPVOID) = ChangeDisplaySettingsEx;

LONG WINAPI myChange(LPCTSTR lpszDeviceName, DEVMODE* lpDevMode, HWND hwnd, DWORD dwflags, LPVOID lParam)
{
	DEVMODE DevMode;
	bool bResult = EnumDisplaySettings(lpszDeviceName, ENUM_CURRENT_SETTINGS, &DevMode);
	if (bResult) {
		if (lpDevMode) {
			lpDevMode->dmDisplayFrequency = DevMode.dmDisplayFrequency;
		}
	}
	return pChange(lpszDeviceName, lpDevMode, hwnd, dwflags, lParam);
}

DWORD_PTR mySetThreadAffinityMask(HANDLE hThread, DWORD_PTR dwThreadAffinityMask) {
	if (!checked)
	{
		check();
	}
	if (over32)
	{
		return 0xffffffff;
	}
	else
	{
		return pSetThreadAffinityMask(hThread, dwThreadAffinityMask);
	}
}

BOOL mySetProcessAffinityMask(HANDLE hProcess, DWORD_PTR dwProcessAffinityMask) {
	if (!checked)
	{
		check();
	}
	if (over32)
	{
		return true;
	}
	else
	{
		return pSetProcessAffinityMask(hProcess, dwProcessAffinityMask);
	}
}

BOOL myGetProcessAffinityMask(HANDLE hProcess, PDWORD_PTR lpProcessAffinityMask, PDWORD_PTR lpSystemAffinityMask) {
	if (!checked)
	{
		check();
	}
	if (over32)
	{
		*lpProcessAffinityMask = 0xffffffff;
		*lpSystemAffinityMask = 0xffffffff;
		return true;
	}
	else
	{
		return pGetProcessAffinityMask(hProcess, lpProcessAffinityMask, lpSystemAffinityMask);
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)pChange, myChange);
		DetourAttach(&(PVOID&)pGetProcessAffinityMask, myGetProcessAffinityMask);
		DetourAttach(&(PVOID&)pSetProcessAffinityMask, mySetProcessAffinityMask);
		DetourAttach(&(PVOID&)pSetThreadAffinityMask, mySetThreadAffinityMask);
		DetourTransactionCommit();
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}