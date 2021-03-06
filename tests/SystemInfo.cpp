#include <string>

#include "SystemInfo.h"

#if defined(LINUX) || defined(__APPLE__) || defined(ANDROID)

#include <string>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

std::string TrimLeft(std::string str)
{
	return str.substr(str.find_first_not_of(" \n\r\t"));
}

std::string TrimRight(std::string str)
{
	str.erase(str.find_last_not_of(" \n\r\t")+1);
	return str;
}

std::string Trim(std::string str)
{
	return TrimLeft(TrimRight(str));
}

// http://stackoverflow.com/questions/646241/c-run-a-system-command-and-get-output
std::string RunProcess(const char *cmd)
{
	FILE *fp = popen(cmd, "r");
	if (!fp)
		return std::string();

	std::stringstream ss;
	char str[1035];
	while(fgets(str, sizeof(str)-1, fp))
		ss << str;

	pclose(fp);

	return TrimRight(ss.str()); // Trim the end of the result to remove \n.
}

std::string FindLine(const std::string &inStr, const char *lineStart)
{
	int lineStartLen = strlen(lineStart);
	size_t idx = inStr.find(lineStart);
	if (idx == std::string::npos)
		return std::string();
	idx += lineStartLen;
	size_t lineEnd = inStr.find("\n", idx);
	if (lineEnd == std::string::npos)
		return inStr.substr(idx);
	else
		return inStr.substr(idx, lineEnd-idx);
}

#endif

#if defined(WIN32) && !defined(WIN8RT)

#include <windows.h>
#include <iphlpapi.h>

#include <tchar.h>
#include <stdio.h>

#include <sstream>
#include <iostream>

#ifdef _MSC_VER
#include <intrin.h>

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "IPHLPAPI.lib")
#elif defined (__GNUC__)
#include <cpuid.h>
#endif

typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

std::string GetOSDisplayString()
{
	OSVERSIONINFOEXA osvi;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
	BOOL bOsVersionInfoEx = GetVersionExA((OSVERSIONINFOA *)&osvi);
	if (!bOsVersionInfoEx)
		return "Unknown OS";

	// Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.
	SYSTEM_INFO si;
	ZeroMemory(&si, sizeof(SYSTEM_INFO));

	PGNSI pGNSI = (PGNSI)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");
	if (pGNSI)
		pGNSI(&si);
	else
		GetSystemInfo(&si);

	std::stringstream ss;

	if (osvi.dwPlatformId != VER_PLATFORM_WIN32_NT)
	{
		ss << "Unknown OS (PlatformID: " << osvi.dwPlatformId << ", MajorVersion: " << osvi.dwMajorVersion << ", MinorVersion: " << osvi.dwMinorVersion << ")";
		return ss.str();
	}

	ss << "Microsoft ";

	// Test for the specific product.
	if (osvi.dwMajorVersion == 6)
	{
		if (osvi.dwMinorVersion == 0)
		{
			if (osvi.wProductType == VER_NT_WORKSTATION)
				ss << "Windows Vista ";
			else
				ss << "Windows Server 2008 ";
		}
		else if (osvi.dwMinorVersion == 1)
		{
			if (osvi.wProductType == VER_NT_WORKSTATION)
				ss << "Windows 7 ";
			else
				ss << "Windows Server 2008 R2 ";
		}
		else if (osvi.dwMinorVersion == 2)
		{
			if (osvi.wProductType == VER_NT_WORKSTATION)
				ss << "Windows 8 ";
		}
		PGPI pGPI = (PGPI)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetProductInfo");
		DWORD dwType = 0;
		pGPI(osvi.dwMajorVersion, osvi.dwMinorVersion, 0, 0, &dwType);

		switch(dwType)
		{
		case 0x1 /*PRODUCT_ULTIMATE*/: ss << "Ultimate Edition"; break;
		case 0x2 /*PRODUCT_HOME_BASIC*/: ss << "Home Basic Edition"; break;
		case 0x3 /*PRODUCT_HOME_PREMIUM*/: ss << "Home Premium Edition"; break;
		case 0x4 /*PRODUCT_ENTERPRISE*/: ss << "Enterprise Edition"; break;
		case 0x6 /*PRODUCT_BUSINESS*/: ss << "Business Edition"; break;
		case 0xB /*PRODUCT_STARTER*/: ss << "Starter Edition"; break;
		case 0x12 /*PRODUCT_CLUSTER_SERVER*/: ss << "Cluster Server Edition"; break;
		case 0x8 /*PRODUCT_DATACENTER_SERVER*/: ss << "Datacenter Edition"; break;
		case 0xC /*PRODUCT_DATACENTER_SERVER_CORE*/: ss << "Datacenter Edition (core installation)"; break;
		case 0xA /*PRODUCT_ENTERPRISE_SERVER*/: ss << "Enterprise Edition"; break;
		case 0xE /*PRODUCT_ENTERPRISE_SERVER_CORE*/: ss << "Enterprise Edition (core installation)"; break;
		case 0xF /*PRODUCT_ENTERPRISE_SERVER_IA64*/: ss << "Enterprise Edition for Itanium-based Systems"; break;
		case 0x9 /*PRODUCT_SMALLBUSINESS_SERVER*/: ss << "Small Business Server"; break;
		case 0x19 /*PRODUCT_SMALLBUSINESS_SERVER_PREMIUM*/: ss << "Small Business Server Premium Edition"; break;
		case 0x7 /*PRODUCT_STANDARD_SERVER*/: ss << "Standard Edition"; break;
		case 0xD /*PRODUCT_STANDARD_SERVER_CORE*/: ss << "Standard Edition (core installation)"; break;
		case 0x11 /*PRODUCT_WEB_SERVER*/: ss << "Web Server Edition"; break;
		}
	}
	else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
	{
		if (GetSystemMetrics(SM_SERVERR2))
			ss << "Windows Server 2003 R2, ";
		else if (osvi.wSuiteMask == VER_SUITE_STORAGE_SERVER)
			ss << "Windows Storage Server 2003";
		else if (osvi.wSuiteMask == 0x00008000)
			ss << "Windows Home Server";
		else if (osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
			ss << "Windows XP Professional x64 Edition";
		else
			ss << "Windows Server 2003, ";

		// Test for the server type.
		if (osvi.wProductType != VER_NT_WORKSTATION)
		{
			if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
			{
				if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
					ss << "Datacenter Edition for Itanium-based Systems";
				else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
					ss << "Enterprise Edition for Itanium-based Systems";
			}
			else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
			{
				if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
					ss << "Datacenter x64 Edition";
				else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
					ss << "Enterprise x64 Edition";
				else
					ss << "Standard x64 Edition";
			}
			else
			{
				if (osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER)
					ss << "Compute Cluster Edition";
				else if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
					ss << "Datacenter Edition";
				else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
					ss << "Enterprise Edition";
				else if (osvi.wSuiteMask & VER_SUITE_BLADE)
					ss << "Web Edition";
				else
					ss << "Standard Edition";
			}
		}
	}
	else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
	{
		ss << "Windows XP ";
		if (osvi.wSuiteMask & VER_SUITE_PERSONAL)
			ss << "Home Edition";
		else
			ss << "Professional";
	}
	else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
	{
		ss << "Windows 2000 ";

		if (osvi.wProductType == VER_NT_WORKSTATION)
			ss << "Professional";
		else
		{
			if (osvi.wSuiteMask & VER_SUITE_DATACENTER )
				ss << "Datacenter Server";
			else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
				ss << "Advanced Server";
			else
				ss << "Server";
		}
	}
	else
	{
		ss << "Unknown OS (PlatformID: " << osvi.dwPlatformId << ", MajorVersion: " << osvi.dwMajorVersion << ", MinorVersion: " << osvi.dwMinorVersion << ")";
		return ss.str();
	}

	// Include service pack (if any) and build number.
	if (strlen(osvi.szCSDVersion) > 0)
		ss << " " << osvi.szCSDVersion;

	ss << " (build " << (int)osvi.dwBuildNumber << ")";

	if (osvi.dwMajorVersion >= 6)
	{
		if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
			ss << ", 64-bit";
		else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
			ss << ", 32-bit";
	}
	return ss.str();
}

unsigned long long GetTotalSystemPhysicalMemory()
{
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	int ret = GlobalMemoryStatusEx(&statex);
	if (ret == 0)
		return 0;

	return (unsigned long long)statex.ullTotalPhys;
}

void CpuId(int *outInfo, int infoType)
{
#ifdef _MSC_VER
	__cpuid(outInfo, infoType);
#elif defined(__GNUC__)
	__get_cpuid((unsigned int)infoType, (unsigned int*)outInfo, (unsigned int*)outInfo+1, (unsigned int*)outInfo+2, (unsigned int*)outInfo+3);
#else
#warning CpuId not implemented for this compiler!
#endif
}

std::string GetProcessorBrandName()
{
	int CPUInfo[4] = {-1};

	// Calling __cpuid with 0x80000000 as the InfoType argument
	// gets the number of valid extended IDs.
	CpuId(CPUInfo, 0x80000000);
	unsigned int nExIds = CPUInfo[0];

	if (nExIds < 0x80000004)
		 return "Unknown";

	char CPUBrandString[0x40];
	memset(CPUBrandString, 0, sizeof(CPUBrandString));

	// Get the information associated with each extended ID.
	for (unsigned int i = 0x80000002; i <= nExIds && i <= 0x80000004; ++i)
	{
		CpuId(CPUInfo, i);

		// Interpret CPU brand string and cache information.
		if  (i == 0x80000002)
			memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
		else if  (i == 0x80000003)
			memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
		else if  (i == 0x80000004)
			memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
	}

	return CPUBrandString;
}

std::string GetProcessorCPUIDString()
{
	int CPUInfo[4] = {-1};

	// __cpuid with an InfoType argument of 0 returns the number of
	// valid Ids in CPUInfo[0] and the CPU identification string in
	// the other three array elements. The CPU identification string is
	// not in linear order. The code below arranges the information 
	// in a human readable form.
	CpuId(CPUInfo, 0);
//	unsigned nIds = CPUInfo[0];
	char CPUString[0x20] = {};
	*((int*)CPUString) = CPUInfo[1];
	*((int*)(CPUString+4)) = CPUInfo[3];
	*((int*)(CPUString+8)) = CPUInfo[2];

	return CPUString;
}

std::string GetProcessorExtendedCPUIDInfo()
{
	int CPUInfo[4] = {-1};

	// __cpuid with an InfoType argument of 0 returns the number of
	// valid Ids in CPUInfo[0] and the CPU identification string in
	// the other three array elements. The CPU identification string is
	// not in linear order. The code below arranges the information 
	// in a human readable form.
	CpuId(CPUInfo, 0);
	unsigned nIds = CPUInfo[0];
	char CPUString[0x20];
	memset(CPUString, 0, sizeof(CPUString));
	*((int*)CPUString) = CPUInfo[1];
	*((int*)(CPUString+4)) = CPUInfo[3];
	*((int*)(CPUString+8)) = CPUInfo[2];

	if (nIds == 0)
		return CPUString;

	CpuId(CPUInfo, 1);

	int nSteppingID = CPUInfo[0] & 0xf;
	int nModel = (CPUInfo[0] >> 4) & 0xf;
	int nFamily = (CPUInfo[0] >> 8) & 0xf;
	//	int nProcessorType = (CPUInfo[0] >> 12) & 0x3;
	int nExtendedmodel = (CPUInfo[0] >> 16) & 0xf;
	int nExtendedfamily = (CPUInfo[0] >> 20) & 0xff;
	//	int nBrandIndex = CPUInfo[1] & 0xff;

	std::stringstream ss;
	ss << CPUString << ", " << "Stepping: " << nSteppingID << ", Model: " << nModel <<
		", Family: " << nFamily << ", Ext.model: " << nExtendedmodel << ", Ext.family: " << nExtendedfamily << ".";

	return ss.str();
}

int GetMaxSimultaneousThreads()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
}

unsigned long GetCPUSpeedFromRegistry(unsigned long dwCPU)
{
	HKEY hKey;
	DWORD dwSpeed;

	// Get the key name
	TCHAR szKey[256] = {};
	_sntprintf(szKey, sizeof(szKey)/sizeof(TCHAR)-1, TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\%d\\"), (int)dwCPU);

	// Open the key
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,szKey, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
		return 0;

	// Read the value
	DWORD dwLen = 4;
	if (RegQueryValueEx(hKey, TEXT("~MHz"), NULL, NULL, (LPBYTE)&dwSpeed, &dwLen) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return 0;
	}

	// Cleanup and return
	RegCloseKey(hKey);
	return dwSpeed;
}
#elif defined(LINUX)

std::string GetOSDisplayString()
{
	return RunProcess("lsb_release -ds") + " " + RunProcess("uname -mrs");
}

unsigned long long GetTotalSystemPhysicalMemory()
{
	std::string r = RunProcess("cat /proc/meminfo");
	std::string memTotal = FindLine(r, "MemTotal:");
	int mem = 0;
	int n = sscanf(memTotal.c_str(), "%d", &mem);
	if (n == 1)
		return (unsigned long long)mem * 1024;
	else
		return 0;
}

std::string GetProcessorBrandName()
{
	std::string r = RunProcess("cat /proc/cpuinfo");
	return Trim(FindLine(FindLine(r, "vendor_id"),":"));
}

std::string GetProcessorCPUIDString()
{
	std::string r = RunProcess("cat /proc/cpuinfo");
	return Trim(FindLine(FindLine(r, "model name"),":"));
}

std::string GetProcessorExtendedCPUIDInfo()
{
	std::string r = RunProcess("cat /proc/cpuinfo");
	std::string stepping = Trim(FindLine(FindLine(r, "stepping"),":"));
	std::string model = Trim(FindLine(FindLine(r, "model"),":"));
	std::string family = Trim(FindLine(FindLine(r, "cpu family"),":"));

	std::stringstream ss;
	ss << GetProcessorBrandName() << ", " << "Stepping: " << stepping << ", Model: " << model <<
		", Family: " << family;
	return ss.str();
}

int GetMaxSimultaneousThreads()
{
	std::string r = RunProcess("lscpu");
	r = TrimRight(FindLine(r, "CPU(s):"));
	int numCPUs = 0;
	int n = sscanf(r.c_str(), "%d", &numCPUs);
	return (n == 1) ? numCPUs : 0;
}

unsigned long GetCPUSpeedFromRegistry(unsigned long /*dwCPU*/)
{
	std::string r = RunProcess("lscpu");
	r = TrimRight(FindLine(r, "CPU MHz:"));
	int mhz = 0;
	int n = sscanf(r.c_str(), "%d", &mhz);
	return (n == 1) ? (unsigned long)mhz : 0;
}

#elif defined(EMSCRIPTEN)

#include <emscripten.h>

unsigned long long GetTotalSystemPhysicalMemory() { return (unsigned long long)emscripten_run_script_int("TOTAL_MEMORY"); }
std::string GetProcessorBrandName() { return "n/a"; } 
std::string GetProcessorCPUIDString() { return "n/a"; }
std::string GetProcessorExtendedCPUIDInfo() { return "n/a"; }
unsigned long GetCPUSpeedFromRegistry(unsigned long /*dwCPU*/) { return 1; }
int GetMaxSimultaneousThreads() { return 1; }

bool IsChromeBrowser()
{
	return GetChromeVersion().v[0] > 0;
}

bool IsChromeBrowserOnWin32()
{
	std::string os = GetOSDisplayString();

	return GetChromeVersion().v[0] > 0 && os.find("Win32") != std::string::npos;
}

bool IsOperaBrowser()
{
	std::string os = GetOSDisplayString();
	return GetOperaVersion().v[0] > 0;
}

bool IsSafariBrowser()
{
	std::string os = GetOSDisplayString();
	return GetSafariVersion().v[0] > 0;
}

BrowserVersion GetChromeVersion()
{
	std::string os = GetOSDisplayString();
	size_t idx = os.find("Chrome/");
	if (idx == std::string::npos)
		return BrowserVersion();
	return BrowserVersion(os.substr(idx+strlen("Chrome/")).c_str());
}

BrowserVersion GetOperaVersion()
{
	std::string os = GetOSDisplayString();
	size_t idx = os.find("Opera");
	if (idx == std::string::npos)
		return BrowserVersion();

	idx = os.find("Version/");
	if (idx == std::string::npos)
		return BrowserVersion();
	return BrowserVersion(os.substr(idx+strlen("Version/")).c_str());
}

BrowserVersion GetSafariVersion()
{
	std::string os = GetOSDisplayString();
	size_t idx = os.find("Safari");
	if (idx == std::string::npos)
		return BrowserVersion();

	idx = os.find("Version/");
	if (idx == std::string::npos)
		return BrowserVersion();
	return BrowserVersion(os.substr(idx+strlen("Version/")).c_str());
}

#elif defined(__APPLE__)

std::string GetOSDisplayString()
{
	std::string uname = RunProcess("uname -mrs");

	// http://stackoverflow.com/questions/11072804/mac-os-x-10-8-replacement-for-gestalt-for-testing-os-version-at-runtime/11697362#11697362
	std::string systemVer = RunProcess("cat /System/Library/CoreServices/SystemVersion.plist");
	size_t idx = systemVer.find("<key>ProductVersion</key>");
	if (idx == std::string::npos)
		return uname;
	idx = systemVer.find("<string>", idx);
	if (idx == std::string::npos)
		return uname;
	idx += strlen("<string>");
	size_t endIdx = systemVer.find("</string>", idx);
	if (endIdx == std::string::npos)
		return uname;
	std::string marketingVersion = Trim(systemVer.substr(idx, endIdx-idx));

	uname += " Mac OS X " + marketingVersion;
	int majorVer = 0, minorVer = 0;
	int n = sscanf(marketingVersion.c_str(), "%d.%d", &majorVer, &minorVer);
	if (n != 2)
		return uname;
	switch (majorVer * 100 + minorVer)
	{
		case 1001: return uname + " Puma";
		case 1002: return uname + " Jaguar";
		case 1003: return uname + " Panther";
		case 1004: return uname + " Tiger";
		case 1005: return uname + " Leopard";
		case 1006: return uname + " Snow Leopard";
		case 1007: return uname + " Lion";
		case 1008: return uname + " Mountain Lion";
		default: return uname;
	}
}

#include <sys/types.h>
#include <sys/sysctl.h>

std::string sysctl_string(const char *sysctl_name)
{
	char str[128] = {};
	size_t size = sizeof(str)-1;
	sysctlbyname(sysctl_name, str, &size, NULL, 0);
	return str;
}

int sysctl_int32(const char *sysctl_name)
{
	int32_t val = 0;
	size_t size = sizeof(val);
	sysctlbyname(sysctl_name, &val, &size, NULL, 0);
	return (int)val;
}

int64_t sysctl_int64(const char *sysctl_name)
{
	int64_t val = 0;
	size_t size = sizeof(val);
	sysctlbyname(sysctl_name, &val, &size, NULL, 0);
	return val;
}

unsigned long long GetTotalSystemPhysicalMemory()
{
	return (unsigned long long)sysctl_int64("hw.memsize");
}

std::string GetProcessorBrandName()
{
	return sysctl_string("machdep.cpu.vendor");
}

std::string GetProcessorCPUIDString()
{
	return sysctl_string("machdep.cpu.brand_string");
}

std::string GetProcessorExtendedCPUIDInfo()
{
	char str[1024];
	sprintf(str, "%s, Stepping: %d, Model: %d, Family: %d, Ext.model: %d, Ext.family: %d.", GetProcessorCPUIDString().c_str(), sysctl_int32("machdep.cpu.stepping"), sysctl_int32("machdep.cpu.model"), sysctl_int32("machdep.cpu.family"), sysctl_int32("machdep.cpu.extmodel"), sysctl_int32("machdep.cpu.extfamily"));
	return str;
}

unsigned long GetCPUSpeedFromRegistry(unsigned long /*dwCPU*/)
{
	int64_t freq = sysctl_int64("hw.cpufrequency");
	return (unsigned long)(freq / 1000 / 1000);
}

// Returns the maximum number of threads the CPU can simultaneously accommodate.
// E.g. for dualcore hyperthreaded Intel CPUs, this returns 4.
int GetMaxSimultaneousThreads()
{
	return (int)sysctl_int32("machdep.cpu.thread_count");
}

#elif defined(ANDROID)

unsigned long long GetTotalSystemPhysicalMemory()
{
	std::string r = RunProcess("cat /proc/meminfo");
	std::string memTotal = FindLine(r, "MemTotal:");
	int mem = 0;
	int n = sscanf(memTotal.c_str(), "%d", &mem);
	if (n == 1)
		return (unsigned long long)mem * 1024;
	else
		return 0;
}

std::string GetOSDisplayString()
{
	std::string ver = Trim(RunProcess("getprop ro.build.version.release"));
	std::string apiLevel = Trim(RunProcess("getprop ro.build.version.sdk"));
	if (apiLevel.empty())
		apiLevel = Trim(RunProcess("getprop ro.build.version.sdk_int"));

	std::string os;
	if (!ver.empty())
		os = "Android " + ver + " ";
	if (!apiLevel.empty())
		os += "SDK API Level " + apiLevel + " ";
	os += Trim(RunProcess("getprop ro.build.description"));
	return os;
}

std::string GetProcessorBrandName()
{
	std::string r = RunProcess("cat /proc/cpuinfo");
	return Trim(FindLine(FindLine(r, "Processor"),":"));
}

std::string GetProcessorCPUIDString()
{
	// Note: This is actually HW identifier, not CPU ID.
	std::string manufacturer = RunProcess("getprop ro.product.manufacturer");
	std::string brand = RunProcess("getprop ro.product.brand");
	std::string model = RunProcess("getprop ro.product.model");
	std::string board = RunProcess("getprop ro.product.board");
	std::string device = RunProcess("getprop ro.product.device");
	std::string name = RunProcess("getprop ro.product.name");
	return manufacturer + " " + brand + " " + model + " " + board + " " + device + " " + name;
}

std::string GetProcessorExtendedCPUIDInfo()
{
	std::string r = RunProcess("cat /proc/cpuinfo");
	std::string implementer = Trim(FindLine(FindLine(r, "CPU implementer"),":"));
	std::string arch = Trim(FindLine(FindLine(r, "CPU architecture"),":"));
	std::string variant = Trim(FindLine(FindLine(r, "CPU variant"),":"));
	std::string part = Trim(FindLine(FindLine(r, "CPU part"),":"));
	std::string rev = Trim(FindLine(FindLine(r, "CPU revision"),":"));
	std::string hw = Trim(FindLine(FindLine(r, "Hardware"),":"));

	std::stringstream ss;
	ss << "Hardware: " << hw << ", CPU implementer: " << implementer << ", arch: " << arch << ", variant: " << variant 
		<< ", part: " << part << ", revision: " << rev;
	return ss.str();
}

unsigned long GetCPUSpeedFromRegistry(unsigned long dwCPU)
{
	std::stringstream ss;
	ss << "cat /sys/devices/system/cpu/cpu" << dwCPU << "/cpufreq/cpuinfo_max_freq";
	std::string r = RunProcess(ss.str().c_str());
	int freq = 0;
	int n = sscanf(r.c_str(), "%d", &freq);
	if (n == 1)
		return freq / 1000; // cpuinfo_max_freq seems to be in kHz. Output MHz.
	else
		return 0;
}

int GetMaxSimultaneousThreads()
{
	std::string r = RunProcess("cat /sys/devices/system/cpu/present");
	int nCoresMin = 0, nCoresMax = 0;
	int n = sscanf(r.c_str(), "%d-%d", &nCoresMin, &nCoresMax);
	if (n == 2)
		return nCoresMax - nCoresMin + 1; // The min-max numbers are indices to cpu cores, so +1 for the count.
	else
		return 1;
}

#else

#ifdef _MSC_VER
#pragma WARNING("SystemInfo.cpp not implemented for the current platform!")
#else
#warning SystemInfo.cpp not implemented for the current platform!
#endif

std::string GetOSDisplayString() { return ""; }
unsigned long long GetTotalSystemPhysicalMemory() { return 0; }
std::string GetProcessorBrandName() { return ""; }
std::string GetProcessorCPUIDString() { return ""; }
std::string GetProcessorExtendedCPUIDInfo() { return ""; }
unsigned long GetCPUSpeedFromRegistry(unsigned long /*dwCPU*/) { return 0; }
int GetMaxSimultaneousThreads() { return 0; }

#endif
