#define _CRT_SECURE_NO_WARNINGS
#define CURL_STATICLIB
#include <iostream>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <fstream>
#include <conio.h>
#include <objidl.h>
#include <gdiplus.h>
#include <direct.h>
#include <strsafe.h>
#include <locale>
#include <windows.h>
#include <vector>
#pragma comment (lib,"Gdiplus.lib")
#include <Urlmon.h>
#pragma comment(lib, "Urlmon.lib")
#include <mmsystem.h>
#pragma comment (lib, "Winmm.lib")
#include "curlx86/curl.h"
#ifdef _DEBUG
#pragma comment (lib, "curlx86/libcurl_a_debug.lib")
#else
#pragma comment (lib, "curlx86/libcurl_a.lib")
#endif
#pragma comment (lib, "Normaliz.lib")
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Wldap32.lib")
#pragma comment (lib, "Crypt32.lib")
#pragma comment (lib, "advapi32.lib")

using namespace std;

#pragma comment(lib, "GdiPlus.lib")
using namespace Gdiplus;
static const GUID png = { 0x557cf406, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e } };

LRESULT _stdcall HookCallback(int nCode, WPARAM wParam, LPARAM lParam);
HHOOK hook;
KBDLLHOOKSTRUCT kbStruct;
ULONG_PTR gdiplusToken;
wstring s = L"0", bufer, qbufer, sfold = L"0", ifold = L"0";
int screennum = 0, imagenum = 0, questions_iterator = 0;
bool endoff = true, shownow = true, allshow = false, isimg = false;
bool is_feedback_received = false, is_stop_signal = false, is_answer_text = false, is_answer_numbers = false, is_goto_number = false, is_tgimage = false;
int W, H, Ww, Hh, lW = GetSystemMetrics(SM_CXSCREEN), lH = GetSystemMetrics(SM_CYSCREEN);
int n = 22, dispos = 0, screen_counter = 0, mode_of_work = 5, qcounter = 0;
int delay1 = 500, delay2 = 1500;
unsigned int last_update_id = 0;
vector<wstring> image_bufer;
string token = "5620370827:AAHN7m2AgWWA_vkWVPl5UAKzQ-k6Z4bb1Ac", m_token = "5620370827:AAHN7m2AgWWA_vkWVPl5UAKzQ-k6Z4bb1Ac";
//string chat_id = "-1001939277134";
string chat_id = "-1001731783850";
string abufer = "";

size_t StrToWstr(wstring& aDst, const string& aSrc)
{
	size_t length;
	length = mbstowcs(NULL, aSrc.c_str(), 0);
	if (length != static_cast<size_t>(-1)) {
		wchar_t* buffer = new wchar_t[length + 1];
		length = mbstowcs(buffer, aSrc.c_str(), length);
		buffer[length] = L'\0';
		aDst.assign(buffer);
		delete[] buffer;
	}
	return length;
}

string WstrToStr(const wstring& wstr)
{
	string str;
	size_t size;
	str.resize(wstr.length());
	wcstombs_s(&size, &str[0], str.size() + 1, wstr.c_str(), wstr.size());
	return str;
}

size_t convert_codepoint_to_utf8(char(&chr)[4], unsigned int cp)
{
	char* result = chr;

	if (cp < 0x80)         /* one octet */
	{
		*(result++) = (unsigned char)(cp);
	}
	else if (cp < 0x800)   /* two octets */
	{
		*(result++) = (unsigned char)((cp >> 6) | 0xc0);
		*(result++) = (unsigned char)((cp & 0x3f) | 0x80);
	}
	else if (cp < 0x10000) /* three octets */
	{
		*(result++) = (unsigned char)((cp >> 12) | 0xe0);
		*(result++) = (unsigned char)(((cp >> 6) & 0x3f) | 0x80);
		*(result++) = (unsigned char)((cp & 0x3f) | 0x80);
	}
	else                  /* four octets */
	{
		*(result++) = (unsigned char)((cp >> 18) | 0xf0);
		*(result++) = (unsigned char)(((cp >> 12) & 0x3f) | 0x80);
		*(result++) = (unsigned char)(((cp >> 6) & 0x3f) | 0x80);
		*(result++) = (unsigned char)((cp & 0x3f) | 0x80);
	}
	return result - chr;
}

int is_hex_character(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

std::string escaped_unicode_to_utf8(char const* str)
{
	std::string result;
	while (*str)
	{
		if (*str == '\\' && *(str + 1) == 'u')
		{
			str += 2;
			char hex[5] = {};
			size_t c = 0;
			while (is_hex_character(*str) && c < 4) hex[c++] = *str++;
			if (c > 0)
			{
				char utf8[4] = {};
				unsigned int code = std::strtoul(hex, nullptr, 16);
				size_t l = convert_codepoint_to_utf8(utf8, code);
				result.append(utf8, l);
			}
		}
		else result += *str++;
	}
	return result;
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT num = 0;
	UINT size = 0;
	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;
	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0)
	{
		return -1;
	}
	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
	{
		return -1;
	}
	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
	for (UINT i = 0; i < num; ++i)
	{
		if (wcscmp(pImageCodecInfo[i].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[i].Clsid;
			free(pImageCodecInfo);
			return i;
		}
	}
	free(pImageCodecInfo);
	return -1;
}

int ConvertPngToBmp(wstring pngFilePath, wstring bmpFilePath)
{
	Bitmap* bitmap = Bitmap::FromFile(pngFilePath.c_str());
	if (!bitmap)
	{
		return -1;
	}
	CLSID bmpClsid;
	GetEncoderClsid(L"image/bmp", &bmpClsid);
	int result = bitmap->Save(bmpFilePath.c_str(), &bmpClsid, NULL);
	delete bitmap;
	return result;
}

static size_t getResponsetoString(void* contents, size_t size, size_t nmemb, void* userp)
{
	((string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

string cp1251_to_utf8(const char* str)
{
	string res;
	int result_u, result_c;
	result_u = MultiByteToWideChar(1251, 0, str, -1, 0, 0);
	if (!result_u) return "0";
	wchar_t* ures = new wchar_t[result_u];
	if (!MultiByteToWideChar(1251, 0, str, -1, ures, result_u))
	{
		delete[] ures;
		return "0";
	}
	result_c = WideCharToMultiByte(65001, 0, ures, -1, 0, 0, 0, 0);
	if (!result_c)
	{
		delete[] ures;
		return "0";
	}
	char* cres = new char[result_c];
	if (!WideCharToMultiByte(65001, 0, ures, -1, cres, result_c, 0, 0))
	{
		delete[] cres;
		return "0";;
	}
	delete[] ures;
	res.append(cres);
	delete[] cres;
	return res;
}

void ToClipboard(string stri)
{
	char* text = new char[stri.length() + 1];
	strcpy(text, stri.c_str());
	if (OpenClipboard(0))
	{
		EmptyClipboard();
		char* clip_data = (char*)(GlobalAlloc(GMEM_FIXED, MAX_PATH));
		lstrcpy(LPWSTR(clip_data), LPCWSTR(text));
		SetClipboardData(CF_TEXT, (HANDLE)(clip_data));
		LCID* lcid = (DWORD*)(GlobalAlloc(GMEM_FIXED, sizeof(DWORD)));
		*lcid = MAKELCID(MAKELANGID(LANG_RUSSIAN, SUBLANG_NEUTRAL), SORT_DEFAULT);
		SetClipboardData(CF_LOCALE, (HANDLE)(lcid));
		CloseClipboard();
	}
}

void replaceSpaces(string& s)
{
	size_t pos;
	while ((pos = s.find(' ')) != string::npos) s.replace(pos, 1, "%20");
}

string Post(string s)
{
	replaceSpaces(s);
	s = cp1251_to_utf8(s.c_str());
	string post_answer;
	CURL* curl;
	CURLcode response;
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, ("https://api.telegram.org/bot" + s + "&chat_id=" + chat_id).c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponsetoString);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &post_answer);
	response = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	return post_answer;
}

int TGSendMessage(string s)
{
	s = Post(m_token + "/sendMessage?text=" + s);
	int pos = s.find("\"message_id\":");
	if (pos == -1) return -1;
	s = s.substr(pos + 13);
	return stoi(s.substr(0, s.find(",")));
}

int TGSendFile(string path_to_photo, bool s_mode)
{
	string post_answer;
	string surl;
	if (s_mode) surl = "https://api.telegram.org/bot" + m_token + "/sendDocument";
	else surl = "https://api.telegram.org/bot" + m_token + "/sendPhoto";
	CURL* curl;
	CURLcode response;
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_URL, surl.c_str());
		curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
		struct curl_slist* headers = NULL;
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponsetoString);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &post_answer);
		curl_slist_append(headers, "Content-Type: multipart/form-data");
		curl_slist_append(headers, "charset=utf-8");
		curl_mime* mime;
		curl_mimepart* part;
		mime = curl_mime_init(curl);
		part = curl_mime_addpart(mime);
		curl_mime_name(part, "chat_id");
		curl_mime_data(part, chat_id.c_str(), CURL_ZERO_TERMINATED);
		part = curl_mime_addpart(mime);
		if (s_mode) curl_mime_name(part, "document");
		else curl_mime_name(part, "photo");
		curl_mime_filedata(part, path_to_photo.c_str());
		curl_mime_type(part, "image/png");
		part = curl_mime_addpart(mime);
		curl_mime_name(part, "caption");
		curl_mime_data(part, "", CURL_ZERO_TERMINATED);
		curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
		response = curl_easy_perform(curl);
		curl_mime_free(mime);
		curl_slist_free_all(headers);
	}
	curl_easy_cleanup(curl);
	int pos = post_answer.find("\"message_id\":");
	if (pos == -1) return -1;
	post_answer = post_answer.substr(pos + 13);
	return stoi(post_answer.substr(0, post_answer.find(",")));
}

bool TGFileDownload(string file_id, wstring folder)
{
	wstring wurl;
	string surl = Post(m_token + "/getFile?file_id=" + file_id);
	string file_download_url = surl.substr(surl.rfind(",\"file_path\":\"") + 14);
	file_download_url = "https://api.telegram.org/file/bot" + m_token + "/" + file_download_url.substr(0, file_download_url.size() - 3);
	StrToWstr(wurl, file_download_url);
	LPCWSTR url = wurl.c_str();
	LPCWSTR filePath = folder.c_str();
	HRESULT hr = URLDownloadToFile(NULL, url, filePath, 0, NULL);
	if (hr == S_OK)
	{
		return true;
	}
	else
	{
		return false;
	}
	return false;
}

void update_update_id(unsigned int& lui)
{
	string str = Post(token + "/getUpdates?offset=" + to_string(lui));
	if (str[6] != 't' || str.size() < 24) return;
	str = str.substr(str.rfind("{\"update_id\":") + 13);
	lui = stoi(str.substr(0, str.find(","))) + 1;
	return;
}

bool Send_question(wstring ws, int s_mode) //Если s_mode = 0, значит это текст, 1 - фото, 2 - документ
{
	string s = WstrToStr(ws);
	int id = -1;
	if (s_mode > 0)
	{
		if (s_mode == 1) id = TGSendFile(s, false);
		else id = TGSendFile(s, true);
		s = "Скрин " + to_string(screen_counter);
	}
	update_update_id(last_update_id);
	if (id == -1) return false;
	return true;
}

void utf_esc_to_str(string& s)
{
	int size, codepage = 1251;
	s = escaped_unicode_to_utf8(s.c_str());
	char* buf = new char[s.length() + 1];
	strcpy_s(buf, s.length() + 1, s.c_str());
	size = MultiByteToWideChar(CP_UTF8, 0, buf, -1, 0, 0);
	wstring wstr(size, 0);
	MultiByteToWideChar(CP_UTF8, 0, buf, -1, &wstr[0], size);
	size = WideCharToMultiByte(codepage, 0, &wstr[0], -1, 0, 0, 0, 0);
	string str(size, 0);
	WideCharToMultiByte(codepage, 0, &wstr[0], -1, &str[0], size, 0, 0);
	SetConsoleOutputCP(codepage);
	s = str;
	return;
}

void string_to_message_vector(string s, vector <string>& mv)
{
	string s_message, ss, file_type = "";
	bool is_get_img = false;
	int pos = s.find("{\"update_id\":"), tpos;
	s = s.substr(pos + 13);
	pos = s.find("{\"update_id\":");
	while (pos != -1)
	{
		s_message = s.substr(0, pos);
		tpos = s_message.rfind("\"text\":\"");
		s = s.substr(pos + 13);
		pos = s.find("{\"update_id\":");
		if (tpos != -1)
		{
			s_message = s_message.substr(tpos + 8);
			s_message = s_message.substr(0, s_message.size() - 4) + file_type;
			utf_esc_to_str(s_message);
			mv.push_back(s_message);
		}
		else
		{
			tpos = s_message.rfind("\"caption\":\"$\"");
			if (tpos == -1)
			{
				tpos = s_message.rfind("\"caption\":\"*\"");
				is_get_img = true;
			}
			if (tpos != -1)
			{
				tpos = s_message.rfind("\"file_id\":\"");
				if (tpos != -1)
				{
					s_message = s_message.substr(tpos + 11);
					s_message = s_message.substr(0, s_message.find("\""));
					if (is_get_img)
					{
						file_type = s.substr(s.find("\"mime_type\":\"image/") + 19, s.size());
						file_type = file_type.substr(0, file_type.find("\""));
						mv.push_back("*" + s_message + " " + file_type);
						is_get_img = false;
					}
					else
					{
						mv.push_back("$" + s_message);
					}
				}
			}
		}
	}
	s_message = s;
	tpos = s_message.rfind("\"text\":\"");
	if (tpos != -1)
	{
		s_message = s_message.substr(tpos + 8);
		s_message = s_message.substr(0, s_message.size() - 5);
		utf_esc_to_str(s_message);
		mv.push_back(s_message);
	}
	else
	{
		tpos = s_message.rfind("\"caption\":\"$\"");
		if (tpos == -1)
		{
			tpos = s_message.rfind("\"caption\":\"*\"");
			is_get_img = true;
		}
		if (tpos != -1)
		{
			tpos = s_message.rfind("\"file_id\":\"");
			if (tpos != -1)
			{
				s_message = s_message.substr(tpos + 11);
				s_message = s_message.substr(0, s_message.find("\""));
				if (is_get_img)
				{
					file_type = s.substr(s.find("\"mime_type\":\"image/") + 19, s.size());
					file_type = file_type.substr(0, file_type.find("\""));
					mv.push_back("*" + s_message + " " + file_type);
					is_get_img = false;
				}
				else
				{
					mv.push_back("$" + s_message);
				}
			}
		}
	}
	return;
}

bool TGGetMessages(vector <string>& mv)
{
	string str, s = Post(token + "/getUpdates?offset=" + to_string(last_update_id));
	if (s[6] != 't' || s.size() < 24) return false;
	str = s.substr(s.rfind("{\"update_id\":") + 13);
	last_update_id = stoi(str.substr(0, str.find(","))) + 1;
	string_to_message_vector(s, mv);
	return true;
}

bool isNumeric(std::string const& str)
{
	char* p;
	strtol(str.c_str(), &p, 10);
	return *p == 0;
}

const string currentDateTime()
{
	string s;
	time_t now = time(0);
	int pos;
	struct tm tstruct;
	char buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%d.%m.%Y %X", &tstruct);
	s = buf;
	pos = s.find(":");
	while (pos > -1)
	{
		s.replace(pos, 1, ".");
		pos = s.find(":");
	}
	return s;
}

void SwitchMicro(bool mode)
{
	HMIXER hmx;
	mixerOpen(&hmx, 0, 0, 0, MIXER_OBJECTF_WAVEIN);

	// Get the line info for the wave in destination line 
	MIXERLINE mxl;

	memset(&mxl, 0, sizeof(mxl));

	mxl.cbStruct = sizeof(mxl);
	mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
	mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_COMPONENTTYPE | MIXER_OBJECTF_HMIXER);

	// find the microphone source line connected to this wave in destination 
	DWORD cConnections = mxl.cConnections;
	for (DWORD j = 0; j < cConnections; j++)
	{
		mxl.dwSource = j;
		mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE);

		if (MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE == mxl.dwComponentType)
		{
			// Find a volume control, if any, of the microphone line 
			LPMIXERCONTROL pmxctrl = (LPMIXERCONTROL)malloc(sizeof MIXERCONTROL);
			MIXERLINECONTROLS mxlctrl =
			{
				sizeof mxlctrl,
				mxl.dwLineID,
				MIXERCONTROL_CONTROLTYPE_VOLUME,
				1,
				sizeof MIXERCONTROL,
				pmxctrl
			};
			if (!mixerGetLineControls((HMIXEROBJ)hmx, &mxlctrl, MIXER_GETLINECONTROLSF_ONEBYTYPE))
			{
				DWORD cChannels = mxl.cChannels;
				if (MIXERCONTROL_CONTROLF_UNIFORM & pmxctrl->fdwControl)
					cChannels = 1;

				LPMIXERCONTROLDETAILS_UNSIGNED pUnsigned = (LPMIXERCONTROLDETAILS_UNSIGNED)
					malloc(cChannels * sizeof MIXERCONTROLDETAILS_UNSIGNED);
				MIXERCONTROLDETAILS mxcd =
				{
					sizeof(mxcd),
					pmxctrl->dwControlID,
					cChannels,
					(HWND)0,
					sizeof MIXERCONTROLDETAILS_UNSIGNED,
					(LPVOID)pUnsigned
				};
				mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE);
				if (mode)
				{
					pUnsigned[0].dwValue = pUnsigned[cChannels - 1].dwValue = pmxctrl->Bounds.dwMinimum + pmxctrl->Bounds.dwMaximum;
				}
				else
				{
					pUnsigned[0].dwValue = pUnsigned[cChannels - 1].dwValue = 0;
				}
				mixerSetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE);

				free(pmxctrl);
				free(pUnsigned);
			}
			else
			{
				free(pmxctrl);
			}
		}
	}
	mixerClose(hmx);
}

// функция для получения названия принтера по умолчанию
BOOL GetDefPrinterName(LPWSTR* printerName)
{
	DWORD n; // количество символов (включая нулевой символ), требуемое под
	// массив-буфер для размещения названия принтера по умолчанию

// 1-й вызов: получим количество символов n
	BOOL OK = GetDefaultPrinterW(NULL, &n);

	// выделим память под массив-буфер для размещения названия принтера
	*printerName = NULL;
	if (n > 0)
		*printerName = new WCHAR[n];

	// 2-й вызов: получим название принтера по умолчанию
	if (*printerName != NULL)
		OK = GetDefaultPrinterW(*printerName, &n);

	return OK;
}

bool IsDefaultPrinterPrinting(LPWSTR defaultPrinterName)
{
	PRINTER_DEFAULTS defaults = { NULL, NULL, PRINTER_ACCESS_USE };
	HANDLE printerHandle = NULL;
	DWORD needed = 0;
	DWORD status = 0;
	// Открываем принтер по умолчанию
	if (!OpenPrinter(defaultPrinterName, &printerHandle, &defaults))
	{
		return false;
	}

	// Получаем статус принтера
	if (!GetPrinter(printerHandle, 2, NULL, 0, &needed) && needed == 0)
	{
		ClosePrinter(printerHandle);
		return false;
	}

	PRINTER_INFO_2* printerInfo = (PRINTER_INFO_2*)malloc(needed);
	if (!GetPrinter(printerHandle, 2, (LPBYTE)printerInfo, needed, &needed))
	{
		free(printerInfo);
		ClosePrinter(printerHandle);
		return false;
	}

	// Проверяем статус принтера
	status = printerInfo->Status;
	free(printerInfo);
	ClosePrinter(printerHandle);

	// Если статус принтера равен PRINTER_STATUS_PRINTING, то он печатает
	if (status != 0)
	{
		return true;
	}
	return false;
}

// функция для посылки задания на печать на принтер
bool PrintFile(string filePath)
{
	string printCommand = ".\\PDFtoPrinter.exe " + filePath;
	system(printCommand.c_str());
	LPWSTR defaultPrinterName; // для названия принтера по умолчанию
	// вызываем функцию для получения названия принтера по умолчанию
	if (!GetDefPrinterName(&defaultPrinterName))
	{
		return FALSE; // завершаем программу ошибкой
	}
	Sleep(2000);
	while (IsDefaultPrinterPrinting(defaultPrinterName))
	{
		Sleep(1000);
	}
	Sleep(3000);
	return true;
}

string ImageSave(string dpath)
{
	int dpos = dpath.rfind(" ");
	string dtype = dpath.substr(dpos + 1, dpath.size());
	dpath = dpath.substr(0, dpos);
	if (ifold == L"0")
	{
		string stfold = "Images\\" + currentDateTime();
		const char* cfold = stfold.c_str();
		_mkdir("Images");
		StrToWstr(ifold, stfold);
		_mkdir(cfold);
	}
	wstring wfolder = ifold + L"\\" + to_wstring(imagenum) + L".";
	wstring dfolder;
	StrToWstr(dfolder, dtype);
	dfolder = wfolder + dfolder;
	wfolder += L"bmp";
	image_bufer.push_back(wfolder);
	TGFileDownload(dpath, dfolder);
	if (dtype != "bmp")
	{
		ConvertPngToBmp(dfolder, wfolder);
	}
	questions_iterator = imagenum;
	imagenum += 1;
	qcounter = imagenum;
	isimg = true;
	return "(" + to_string(questions_iterator + 1) + "/" + to_string(qcounter) + ")";
}

string Get_and_process_messages()
{
	vector <string> mb;
	TGGetMessages(mb);
	unsigned short vs = mb.size();
	if (vs == 0) return ""; //вектор пустой
	string answer = "";
	for (int i = 0; i < vs; i++)
	{
		if (string(".!#$*").find(mb[i][0]) != -1)
		{
			is_feedback_received = true;
			switch (mb[i][0])
			{
			case '.':
				if (mb[i].size() == 2)
				{
					is_stop_signal = true;
				}
				else
				{
					answer = " " + mb[i].substr(1);
					is_answer_text = true;
				}
				break;
			case '#':
				answer = mb[i].substr(1);
				is_goto_number = true;
				break;
			case '!':
				answer = mb[i].substr(1);
				is_answer_numbers = true;
				break;
			case '$':
				answer = mb[i].substr(1);
				TGFileDownload(answer, L"Сканы\\Scan.pdf");
				SwitchMicro(false);
				PrintFile("Сканы\\Scan.pdf");
				//Ждать конца печати
				SwitchMicro(true);
				break;
			case '*':
				answer = mb[i].substr(1);
				answer = ImageSave(answer);
				is_tgimage = true;
			}
			if (is_goto_number || is_stop_signal)
			{
				keybd_event(VK_SCROLL, 0, 0, 0);
				keybd_event(VK_SCROLL, 0, KEYEVENTF_KEYUP, 0);
			}
			else
			{
				if (!is_tgimage)
				{
					keybd_event(VK_NUMLOCK, 0, 0, 0);
					keybd_event(VK_NUMLOCK, 0, KEYEVENTF_KEYUP, 0);
				}	
				else
				{
					is_tgimage = false;
				}
			}
		}
	}
	return answer;
}

string Get_web_answers()
{
	return Get_and_process_messages();
}

void TGSendReplyMessage(unsigned int id, string s)
{
	string str = "&reply_to_message_id=" + to_string(id);
	Post(m_token + "/sendMessage?text=" + s + str);
	return;
}

unsigned int GetClipboardFiles(vector<string>& fnv)
{
	if (IsClipboardFormatAvailable(CF_HDROP))
	{
		/* Clipboard contains file names; print them */
		HDROP cb_hnd;		/* Clipboard memory handle */
		unsigned int textfmt = CF_OEMTEXT;
		OpenClipboard(NULL);
		cb_hnd = (HDROP)GetClipboardData(CF_HDROP);
		if (cb_hnd != NULL) 
		{
			int nfiles, i;
			char fname[MAX_PATH];

			nfiles = DragQueryFile(cb_hnd, 0xFFFFFFFF, NULL, 0);
			for (i = 0; i < nfiles; i++)
			{
				switch (textfmt)
				{
				case CF_TEXT:
					DragQueryFileA(cb_hnd, i, fname, sizeof(fname));
					break;
				case CF_UNICODETEXT:
					DragQueryFileW(cb_hnd, i, (LPWSTR)fname, sizeof(fname));
					break;
				case CF_OEMTEXT:
					DragQueryFileA(cb_hnd, i, fname, sizeof(fname));
					CharToOemBuff((LPCWSTR)fname, fname, strlen(fname));
					break;
				}
				fnv.push_back(fname);
			}
		}
		CloseClipboard();
	}
	return fnv.size();
}

wstring GetClipboardText()
{
	if (!OpenClipboard(nullptr))
	{
		CloseClipboard();
		return L"";
	}
	HANDLE hData = GetClipboardData(CF_UNICODETEXT);
	if (hData == nullptr)
	{
		CloseClipboard();
		return L"";
	}
	wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
	if (pszText == nullptr)
	{
		CloseClipboard();
		return L"";
	}
	wstring text(pszText);
	GlobalUnlock(hData);
	CloseClipboard();
	return text;
}

wstring sringcler(wstring s) // Удаление из строки небуквенных символов, перевод в нижний регистр
{
	wstring result;
	for (auto& c : s)
	{
		if (c > 1071 && c < 1104) result += c;
		else if (c > 1039 && c < 1072)
		{
			char nc = (char)((int)c - 1072);
			string ms;
			ms = nc;
			wstring wst;
			StrToWstr(wst, ms);
			result += wst;
		}
		else if (c == 1105 || c == 1025) result += L"?";
		else if (isalnum(c)) result += tolower(c);
	}
	return result;
}

wstring MakeScreen() // Создание скриншота
{
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	HDC scrdc, memdc;
	HBITMAP membit;
	if (sfold == L"0")
	{
		string stfold = "Screenshots\\" + currentDateTime();
		const char* cfold = stfold.c_str();
		_mkdir("Screenshots");
		StrToWstr(sfold, stfold);
		_mkdir(cfold);
	}
	scrdc = GetDC(0);
	memdc = CreateCompatibleDC(scrdc);
	membit = CreateCompatibleBitmap(scrdc, Ww, Hh);
	SelectObject(memdc, membit);
	BitBlt(memdc, 0, 0, Ww, Hh, scrdc, 0, 0, SRCCOPY);
	HBITMAP hBitmap;
	hBitmap = (HBITMAP)SelectObject(memdc, membit);
	Gdiplus::Bitmap bitmap(hBitmap, NULL);
	wstring www = sfold + L"\\" + to_wstring(screennum) + L".png";
	const wchar_t* l = www.c_str();
	bitmap.Save(l, &png);
	screennum++;
	DeleteObject(hBitmap);
	return l;
}

void flasher(string in, bool fmode) //мигает столько раз, сколько указано в цифрах
{
	int k_code;
	if (fmode)
	{
		k_code = VK_NUMLOCK;
		is_answer_numbers = false;
	}
	else
	{
		k_code = VK_SCROLL;
		is_goto_number = false;
	}
	vector <int> num;
	int pos = in.find(" "), cn;
	while (pos != -1)
	{
		cn = stoi(in.substr(0, pos));
		if (cn < 30)
		{
			num.push_back(cn);
		}
		in = in.substr(pos + 1);
		pos = in.find(" ");
	}
	cn = stoi(in);
	if (cn < 30)
	{
		num.push_back(cn);
	}
	Sleep(delay1);
	keybd_event(k_code, 0, 0, 0);
	keybd_event(k_code, 0, KEYEVENTF_KEYUP, 0);
	for (int i = 0; i < num.size(); i++)
	{
		for (int j = 0; j < num[i]; j++)
		{
			Sleep(delay1);
			keybd_event(k_code, 0, 0, 0);
			keybd_event(k_code, 0, KEYEVENTF_KEYUP, 0);
			Sleep(delay1);
			keybd_event(k_code, 0, 0, 0);
			keybd_event(k_code, 0, KEYEVENTF_KEYUP, 0);
		}
		if (i != num.size() - 1) Sleep(delay2);
	}
	return;
}

void Paint(wstring path, int x, int y, int is)
{
	HDC hdc;
	PAINTSTRUCT ps;
	BITMAP bmp;
	hdc = GetDC(0);
	HBITMAP hBitmap = (HBITMAP)LoadImage(NULL, path.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	GetObject(hBitmap, sizeof(BITMAP), &bmp);
	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
	int nih = bmp.bmHeight * is / bmp.bmWidth;
	StretchBlt(hdc, x, y, is, nih, hdcMem, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
	SelectObject(hdcMem, hOldBitmap);
	DeleteDC(hdcMem);
	DeleteObject(hBitmap);
	EndPaint(NULL, &ps);
}

wstring Ktest(int key)
{
	extern char prevProg[256];
	if (key == 1 || key == 2) return L"0";
	locale::global(locale("rus"));
	string filestr, temp_string, tcbc = "";
	wstring stemp = wstring(s.begin(), s.end());
	ifstream qu, an;
	LPCWSTR str = stemp.c_str();
	vector <string> fnv;
	short gksc;
	bool paint = false, change_string = false;
	int pos = -1, j, sm = 0;
	gksc = GetKeyState(VK_CONTROL);
	if (gksc == 0)
	{
		if (is_feedback_received)
		{
			if (is_stop_signal)
			{
				keybd_event(VK_SCROLL, 0, 0, 0);
				keybd_event(VK_SCROLL, 0, KEYEVENTF_KEYUP, 0);
				is_goto_number = false;
			}
			if (is_answer_numbers)
			{
				flasher(WstrToStr(qbufer), true);
				is_goto_number = false;
			}
			if (is_goto_number)
			{
				flasher(WstrToStr(qbufer), false);
				is_goto_number = false;
			}
			if (is_answer_text)
			{
				if (abufer.size() > 1)
				{
					tcbc = abufer[0];
					abufer = abufer.substr(1);
				}
				else
				{
					tcbc = "";
					is_answer_text = false;
					keybd_event(VK_NUMLOCK, 0, 0, 0);
					keybd_event(VK_NUMLOCK, 0, KEYEVENTF_KEYUP, 0);

				}
				ToClipboard(tcbc);
			}
			is_feedback_received = false;
		}
	}
	else
	{
		if ((gksc & 0x1000) != 0) // Комбинации клавиш с CTRL
		{
			switch (key)
			{
			case 0x43:// CTRL + C = отправка скопированных файлов в ТГ
			{
				Sleep(1500);
				GetClipboardFiles(fnv);
				for (int i = 0; i < fnv.size(); i++)
				{
					TGSendFile(fnv[i], true);
				}
				break;
			}
			case 0x49:// "CTRL" + "I" = отменить ввод из буфера обмена
				TGSendMessage("Я сбросил ввод текущего, отправленного мне сообщения");
				tcbc = "";
				is_answer_text = false;
				keybd_event(VK_NUMLOCK, 0, 0, 0);
				keybd_event(VK_NUMLOCK, 0, KEYEVENTF_KEYUP, 0);
				break;
			case 0x56:// "CTRL" + "V" = вставить одну букву ответа в поле для ответов
				if (is_answer_text)
				{
					if (abufer.size() > 1)
					{
						tcbc = abufer[0];
						abufer = abufer.substr(1);
					}
					else
					{
						tcbc = "";
						is_answer_text = false;
						keybd_event(VK_NUMLOCK, 0, 0, 0);
						keybd_event(VK_NUMLOCK, 0, KEYEVENTF_KEYUP, 0);
					}
					if (tcbc == "\\" && abufer.size() > 1 && abufer[0] == 'n')
					{
						abufer = abufer.substr(1);
						keybd_event(VK_RETURN, 0, 0, 0);
						keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
						tcbc = "";
					}
					ToClipboard(tcbc);
				}
				break;
			case 0x52:// "CTRL" + "R" = создание скриншота и сохранение его в папку
				MakeScreen();
				break;
			case 0xDB:// "CTRL" + "[" = Уменьшает количество выводимых символов на 2
			{
				if (n != 2) n -= 2;
				change_string = true;
				paint = true;
			}
			break;
			case 0xDD:// "CTRL" + "]" = Увеличивает количество выводимых символов на 2
			{
				n += 2;
				change_string = true;
				paint = true;
			}
			break;
			case 0xBC:// "CTRL" + "<" = Сдвигает стоку на 10 символов влево
			{
				if (qbufer.size() > n && dispos != 0)
				{
					if (dispos < n / 2) dispos = 0;
					else dispos -= n / 2;
					change_string = true;
					paint = true;
				}
			}
			break;
			case 0xBE:// "CTRL" + ">" = Сдвигает стоку на 10 символов вправо
			{
				if (qbufer.size() > n && dispos < qbufer.size() - n)
				{
					if (dispos > qbufer.size() - 1.5 * n) dispos = qbufer.size() - n;
					else dispos += n / 2;
					change_string = true;
					paint = true;
				}
			}
			break;

			case 0x42:// "CTRL" + "B" = показать весь текст разом
			{
				if (!allshow && shownow)
				{
					InvalidateRect(0, NULL, TRUE);
					pos = 1;
					stemp = qbufer;
					if (size(stemp) > n)
					{
						s = stemp.substr(0, n);
						str = s.c_str();
						TextOutW(GetDC(0), 0.9 * W, 0.9 * H, str, lstrlen(str));
						Sleep(100);
						stemp = stemp.substr(n, size(stemp) - n);
					}
					else
					{
						str = stemp.c_str();
						TextOutW(GetDC(0), 0.9 * W, 0.9 * H, str, lstrlen(str));
						Sleep(100);
					}
					while (size(stemp) > n)
					{
						s = stemp.substr(0, n);
						str = s.c_str();
						TextOutW(GetDC(0), 0.9 * W, 0.9 * H + pos * 0.02 * H, str, lstrlen(str));
						Sleep(100);
						pos += 1;
						stemp = stemp.substr(n, size(stemp) - n);
					}
					str = stemp.c_str();
					TextOutW(GetDC(0), 0.9 * W, 0.9 * H + pos * 0.02 * H, str, lstrlen(str));
					Sleep(100);
					allshow = true;
				}
				else
				{
					if (size(qbufer) > n) s = qbufer.substr(0, n) + L"...";
					else s = qbufer;
					allshow = false;
					paint = true;
				}
			}
			break;
			case 0xC0:// "CTRL" + "~" = показывает текст/ скрывает текст
			{
				if (shownow) InvalidateRect(0, NULL, TRUE);
				else TextOutW(GetDC(0), 0.9 * W, 0.9 * H, str, lstrlen(str));
				shownow = !shownow;
				if (!shownow)
				{
					isimg = false;
				}
			}
			break;
			case 0xBA:// "CTRL" + ";" = Перемещается назад по ответам
			{
				if (imagenum == 0) break;
				if (questions_iterator == 0) questions_iterator = qcounter - 1;
				else questions_iterator--;
				qbufer = L"(" + to_wstring(questions_iterator + 1) + L"/" + to_wstring(qcounter) + L")";
				dispos = 0;
				isimg = true;
				change_string = true;
				paint = true;
			}
			break;
			case 0xDE: // "CTRL" + "'" = Перемещается вперёд по ответам
			{
				if (qcounter == 0) break;
				if (questions_iterator == qcounter - 1) questions_iterator = 0;
				else questions_iterator++;
				qbufer = L"(" + to_wstring(questions_iterator + 1) + L"/" + to_wstring(qcounter) + L")";
				dispos = 0;
				isimg = true;
				change_string = true;
				paint = true;
			}
			break;
			case 0xBF: // "CTRL" + "/" = отправляет скриншот экрана, как вопрос
			{
				if (shownow) sm = 1;
				if (Send_question(MakeScreen(), 1 + sm)) qbufer = L"Скрин " + to_wstring(screen_counter) + L" отправлен";
				else qbufer = L"Ошибка отправки(";
				dispos = 0;
				change_string = true;
				paint = true;
			}
			break;
			case 0xDC:// "CTRL" + "\" = узнаёт, нет ли ответа на твои вопросы
			{
				abufer = Get_web_answers();
				if (abufer == "")
				{
					StrToWstr(stemp, "Нет ответов");
				}
				else
				{
					StrToWstr(stemp, abufer);
				}
				qbufer = stemp;
				str = stemp.c_str();
				dispos = 0;
				change_string = true;
				paint = true;
			}
			break;
			case 0x25:// "CTRL" + "стрелка влево" = сдвигает окно влево
			{
				W -= Ww / 10;
				paint = true;
			}
			break;
			case 0x26: // "CTRL" + "стрелка вниз" = сдвигает окно вниз
			{
				H -= Hh / 10;
				paint = true;
			}
			break;
			case 0x27: // "CTRL" + "стрелка вправо" = сдвигает окно вправо
			{
				W += Ww / 10;
				paint = true;
			}
			break;
			case 0x28: // "CTRL" + "стрелка вверх" = сдвигает окно вверх
			{
				H += Hh / 10;
				paint = true;
			}
			break;
			case 0x51: // "CTRL" + "Q" = закрывает программу
			{
				GdiplusShutdown(gdiplusToken);
				InvalidateRect(0, NULL, TRUE);
				endoff = false;
			}
			break;
			default:
				break;
			}
			if (paint && shownow)
			{
				if (change_string)
				{
					if (qbufer.size() - dispos > n) stemp = qbufer.substr(dispos, n) + L"...";
					else stemp = qbufer.substr(dispos);
					str = stemp.c_str();
				}
				InvalidateRect(0, NULL, TRUE);
				Sleep(100);
				TextOutW(GetDC(0), 0.9 * W, 0.9 * H, str, lstrlen(str));
				Sleep(100);
				TextOutW(GetDC(0), 0.9 * W, 0.9 * H, str, lstrlen(str));	
				paint = false;
				if (isimg)
				{
					int img_size = 50 * n;
					Paint(image_bufer[questions_iterator], 0.9 * W, 0.9 * H - img_size, img_size);
				}
			}
		}
	}
	wstring ws(str);
	return ws;
}

LRESULT _stdcall HookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (endoff)
	{
		if (nCode >= 0)
		{
			if (wParam == WM_KEYDOWN)
			{
				kbStruct = *((KBDLLHOOKSTRUCT*)lParam);
				s = Ktest(kbStruct.vkCode);
			}
		}
	}
	else exit(0);
	return CallNextHookEx(hook, nCode, wParam, lParam);
}

void GetRealWH()
{
	keybd_event(VK_SNAPSHOT, 0, 0, 0);
	keybd_event(VK_SNAPSHOT, 0, KEYEVENTF_KEYUP, 0);
	int lc = 0;
	HBITMAP hBitmap;
	OpenClipboard(NULL);
	hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
	CloseClipboard();
	while (hBitmap == NULL)
	{
		Sleep(1000);
		OpenClipboard(NULL);
		hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
		CloseClipboard();
		lc == 0;
		if (lc == 5)
		{
			keybd_event(VK_SNAPSHOT, 0, 0, 0);
			keybd_event(VK_SNAPSHOT, 0, KEYEVENTF_KEYUP, 0);
		}
	}
	BITMAP bmp;
	GetObject(hBitmap, sizeof(bmp), &bmp);
	Ww = bmp.bmWidth;
	Hh = bmp.bmHeight;
	W = Ww - 500;
	H = Hh - 300;
}

UINT GetClipboardFormat()
{
	UINT datatype[26] = { CF_BITMAP, CF_DIB, CF_DIBV5, CF_DIF, CF_DSPBITMAP, CF_DSPENHMETAFILE, CF_DSPMETAFILEPICT, CF_DSPTEXT, CF_ENHMETAFILE, CF_GDIOBJFIRST, CF_GDIOBJLAST, CF_HDROP, CF_LOCALE, CF_METAFILEPICT, CF_OEMTEXT, CF_OWNERDISPLAY, CF_PALETTE, CF_PENDATA, CF_PRIVATEFIRST, CF_PRIVATELAST, CF_RIFF, CF_SYLK, CF_TEXT, CF_TIFF, CF_UNICODETEXT, CF_WAVE };
	for (int i = 0; i < 26; i++)
	{
		if (IsClipboardFormatAvailable(datatype[i]))
		{
			return datatype[i];
		}
	}
	return -1;
}

int main()
{
	ShowWindow(FindWindowA("ConsoleWindowClass", NULL), 0);
	HDC hdc = GetDC(0);
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	COLORREF color2, color1;
	int bcW, bcH;
	GetRealWH();
	update_update_id(last_update_id);
	string m_str = "";
	ifstream if2("token.txt");
	string f_token;
	if (if2)
	{
		if2 >> f_token;
		if2.close();
		if (f_token != "") m_token = f_token;
	}
	if (!(hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallback, NULL, 0))) MessageBox(NULL, LPWSTR("Что-то пошло не так!"), LPWSTR("Ошибка"), MB_ICONERROR);
	MSG message;
	while (true) GetMessage(&message, NULL, 0, 0);
}