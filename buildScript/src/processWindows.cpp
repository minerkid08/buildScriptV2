#ifdef _WIN64
#include <string>
#include <windows.h>

int runCmd(const char* cmd, const char* args2, std::string* out)
{
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	HANDLE hr;
	HANDLE hw;
	CreatePipe(&hr, &hw, &sa, 0);
	SetHandleInformation(hr, HANDLE_FLAG_INHERIT, 0);
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags |= STARTF_USESTDHANDLES;
	si.hStdInput = NULL;
	si.hStdOutput = hw;
	si.hStdError = hw;
	std::string command = cmd;
	command += ' ';
	command += args2;
	CreateProcess(nullptr, (LPSTR)(char*)command.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	unsigned long read = 1;
	char buf[2048];
	CloseHandle(hw);
	while (read > 0)
	{
		if (!ReadFile(hr, buf, 2048, &read, nullptr))
		{
			break;
		}
		*out += std::string(buf, read);
	}
	CloseHandle(hr);
  return 0;
}

#endif
