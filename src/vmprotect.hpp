#pragma once
#include "includes/vmprotect/VMProtectSDK.h"
#include <filesystem>
#include <Windows.h>
#include <time.h>

#undef GetUserName

#define SERIAL_NUMBER_MAX_LENGTH 8'192

namespace ncore {
	class vmprotect
	{
	private:
		static vmprotect* ClassInstance;

	private:
		__forceinline vmprotect()
		{
			SerialNumberBuffer = new char[SERIAL_NUMBER_MAX_LENGTH] { 0 };

			SerialData = new VMProtectSerialNumberData;

			LastMessage = "No key";
			/*
			для доп защиты, если это не делает сам вмп, можно выделять в каком то куске память (ключ * ид),
			и писать туда что нибдуь, за тем, менять протект страницы на readonly,
			таким образом, 2 раз написать туда будет невозможно,
			по скольку последует завершение программы с кодом STATUS_ACCES_VIOLATION,
			чтобы нельзя было выделить один и тот же вмп 2 раза, например, если его захотят выделить ИЗВНЕ
			*/
		}

		__forceinline ~vmprotect()
		{
			delete SerialData;

			delete[] SerialNumberBuffer;
		}

		__forceinline int SilentExit()
		{
			((void(NTAPI*)(ULONG))GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlExitUserProcess"))(-1);

			return *(int*)(0x0);
		}

		enum VMProtectExtendedActivationFlags
		{
			ACTIVATION_SET_SERIAL_NUMBER = ACTIVATION_NOT_AVAILABLE + 1,
			ACTIVATION_INVALID_SERIAL_NUMBER,
			ACTIVATION_INVALID_SERIAL_DATA,
			ACTIVATION_TIMEOUT,
		};

	public:
		static __forceinline vmprotect* Instance(bool initIfNot = true)
		{
			if (!ClassInstance && initIfNot)
				ClassInstance = new vmprotect();

			return ClassInstance;
		}

		static __forceinline void Terminate()
		{
			if (!ClassInstance)
				return;

			delete ClassInstance;
			ClassInstance = NULL;
		}


		__forceinline bool Activate(char* serialNumber = NULL, bool recursion = true, bool getNewKey = false)
		{
#if defined (DEBUG) || defined (_DEBUG)
			SetActivationMessage(ACTIVATION_OK);

			return true;
#endif
			LastMessage = "---";

			static DWORD InputTry = NULL;

		NextTry:
			if (InputTry++ >= 5)
			{
				SetActivationMessage(ACTIVATION_TIMEOUT);

				Sleep(10000);

				InputTry = NULL;
			}


			int VMSetNumber;
			if (serialNumber)
			{
				for (int c = 0; c < SERIAL_NUMBER_MAX_LENGTH; c++)
					SerialNumberBuffer[c] = serialNumber[c];
			}
			else
			{
				int VMActivate = ACTIVATION_BAD_CODE;
				char* Key = GetKey(getNewKey);
				if (Key)
				{
					VMActivate = VMProtectActivateLicense(Key, SerialNumberBuffer, SERIAL_NUMBER_MAX_LENGTH);
					delete[] Key;
				}

				SetActivationMessage(VMActivate);

				if (VMActivate != ACTIVATION_OK) goto Exit;
			}


			VMSetNumber = VMProtectSetSerialNumber(SerialNumberBuffer);
			if (VMSetNumber != ACTIVATION_OK)
			{
				SetActivationMessage(ACTIVATION_SET_SERIAL_NUMBER);

			Exit:
				if (recursion)
					goto NextTry;

				return false;
			}


			int IsProtected = VMProtectGetSerialNumberState();
			if (IsProtected != ACTIVATION_OK)
			{
				SetActivationMessage(ACTIVATION_INVALID_SERIAL_NUMBER);

				goto Exit;
			}


			bool SrialData = VMProtectGetSerialNumberData(SerialData, sizeof(VMProtectSerialNumberData));
			if (SrialData != true)
			{
				SetActivationMessage(ACTIVATION_INVALID_SERIAL_DATA);

				goto Exit;
			}

			return true;
		}

		__forceinline char* GetUserName()
		{
			char* UserName = new char[sizeof(VMProtectSerialNumberData::wUserName) / sizeof(*VMProtectSerialNumberData::wUserName)] { 0 };

			sprintf(UserName, "%S", SerialData->wUserName);

			return UserName;
		}

		__forceinline char* GetUserData()
		{
			char* UserData = new char[sizeof(VMProtectSerialNumberData::bUserData) / sizeof(*VMProtectSerialNumberData::bUserData)] { 0 };

			sprintf(UserData, "%s", SerialData->bUserData);

			return UserData;
		}

		__forceinline int GetRemainingTime()
		{
			time_t CurrentTime = time(0);
			tm* NowDate = localtime(&CurrentTime);

			tm ExpireDate = {};
			ExpireDate.tm_mday = SerialData->dtExpire.bDay;
			ExpireDate.tm_mon = SerialData->dtExpire.bMonth - 1;
			ExpireDate.tm_year = SerialData->dtExpire.wYear - 1900;
			mktime(&ExpireDate);

			return ((365 * (ExpireDate.tm_year - NowDate->tm_year)) + ExpireDate.tm_yday) - NowDate->tm_yday;
		}

		__forceinline char* GetExpirationTime()
		{
			auto Result = new char[64] {0};

			sscanf(Result, "%d.%d.%d", SerialData->dtExpire.bDay, SerialData->dtExpire.bMonth, SerialData->dtExpire.wYear);

			return Result;
		}

		__forceinline char* GetHardwareID()
		{
			int HWIDLength = VMProtectGetCurrentHWID(NULL, NULL);
			char* HWIDStr = new char[HWIDLength];
			VMProtectGetCurrentHWID(HWIDStr, HWIDLength);
			return HWIDStr;
		}

		__forceinline char* GetHardwareCode()
		{
			auto ID = GetHardwareID();
			if (strlen(ID) < 15)
			{
				return ID; //myhwid for example
			}

			auto Code = new char[15] { 0 };
			for (char i = 0; i < 15 - 1; i++)
			{
				if (i == 4 || i == 9)
				{
				SetBreak:
					Code[i] = '-';
				}
				else if (ID[i] < 48 || ID[i] > 122)
				{
					for (SIZE_T j = i; j < strlen(ID); j++)
					{
						if (ID[j] >= 48 && ID[j] <= 122)
						{
							Code[i] = ID[j];
							break;
						}

					}
					goto SetBreak;
				}
				else
				{
					Code[i] = ID[i];
				}

			}
			delete[] ID;

			return Code;
		}

		__forceinline char* GetSerialNumber(char buffer[SERIAL_NUMBER_MAX_LENGTH] = NULL)
		{
			if (!SerialNumberBuffer)
				return NULL;

			if (!buffer)
				buffer = new char[SERIAL_NUMBER_MAX_LENGTH] { 0 };

			for (int c = 0; c < SERIAL_NUMBER_MAX_LENGTH; c++)
				buffer[c] = SerialNumberBuffer[c];

			return buffer;
		}

		__forceinline const char* GetActivationMessage()
		{
			return LastMessage;
		}

		char* GetLastKey()
#ifndef VMP_GETLASTKEY_OVERRIDE
		{
			char* CurrentHWID = GetHardwareID();

			for (SIZE_T i = 0; i < strlen(CurrentHWID); i++)
			{
				if (CurrentHWID[i] == '\\' || CurrentHWID[i] == '/')
					CurrentHWID[i] = '_';
			}
			std::string KeyFilePath = std::filesystem::temp_directory_path().string() + CurrentHWID;

			delete CurrentHWID;



			HANDLE KeyFileHandle = CreateFileA(KeyFilePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN, NULL);
			if (KeyFileHandle == INVALID_HANDLE_VALUE)
			{
				return NULL;
			}

			char* Key = new char[15] { 0 };

			ReadFile(KeyFileHandle, Key, 15, NULL, NULL);

			CloseHandle(KeyFileHandle);

			return Key;
		}
#endif
		;

		void SetLastKey(char* key)
#ifndef VMP_SETLASTKEY_OVERRIDE
		{
			char* CurrentHWID = GetHardwareID();

			for (SIZE_T i = 0; i < strlen(CurrentHWID); i++)
				if (CurrentHWID[i] == '\\' || CurrentHWID[i] == '/')
					CurrentHWID[i] = '_';

			std::string KeyFilePath = std::filesystem::temp_directory_path().string() + CurrentHWID;

			delete CurrentHWID;



			HANDLE KeyFileHandle = CreateFileA(KeyFilePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);

			if (KeyFileHandle == INVALID_HANDLE_VALUE)
				SilentExit();

			WriteFile(KeyFileHandle, key, 15, NULL, NULL);

			CloseHandle(KeyFileHandle);
		}
#endif
		;
	private:
		const char* LastMessage;

		char* SerialNumberBuffer;

		VMProtectSerialNumberData* SerialData;


		__forceinline void SetActivationMessage(int state)
		{
			static const char* Messages[]
			{
				/*
			ACTIVATION_OK,
			ACTIVATION_SMALL_BUFFER,
			ACTIVATION_NO_CONNECTION,
			ACTIVATION_BAD_REPLY,
			ACTIVATION_BANNED,
			ACTIVATION_CORRUPTED,
			ACTIVATION_BAD_CODE,
			ACTIVATION_ALREADY_USED,
			ACTIVATION_SERIAL_UNKNOWN,
			ACTIVATION_EXPIRED,
			ACTIVATION_NOT_AVAILABLE

			ACTIVATION_SET_SERIAL_NUMBER,
			ACTIVATION_GET_SERIAL_NUMBER_STATE,
			ACTIVATION_GET_SERIAL_NUMBER_DATA
				*/

				"Ok",
				"Small buffer",
				"No connection",
				"Bad reply",
				"Not allowed",
				"Corrupted",
				"Incorrect key",
				"Already used",
				"Serial unknown",
				"License expired",
				"Not available",

				"Can't set serial number",
				"Invalid serial number",
				"Invalid serial data",
				"Timeout 10 secound",

			};

			LastMessage = Messages[state];

#ifndef VMP_NO_MESSAGES_OUTPUT
			printf(std::string(LastMessage + std::string("\n")).c_str());
#endif
		}

		__forceinline char* GetKey(bool getNewKey)
		{
			return getNewKey ? GetNewKey() : GetLastKey();
		}

		//u need make it on ur project manually
		char* GetNewKey();

	}; //vmprotect
}
