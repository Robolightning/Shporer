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
wstring s = L"0", bufer, qbufer, sfold = L"0";
int screennum = 0;
bool endoff = true, shownow = false, allshow = false;
int W, H, Ww, Hh, lW = GetSystemMetrics(SM_CXSCREEN), lH = GetSystemMetrics(SM_CYSCREEN);
int n = 22, dispos = 0, screen_counter = 0, mode_of_work = 5, qcounter = 0;

using namespace std;

struct message_storage
{
	bool request = false;
	string message;
	vector <unsigned int> id;
};

struct question_storage
{
	wstring question;
	wstring answer;
};

vector <message_storage> my_q_bufer, not_my_q_bufer;
vector <question_storage> my_q_storage;

unsigned int last_update_id = 0;
unsigned short questions_iterator = 0;
string token = "5779552750:AAHzHgFfHNZICH9agv-etQ7sSzq6sGVa9D0", m_token = "5779552750:AAHzHgFfHNZICH9agv-etQ7sSzq6sGVa9D0";

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

void Uninstall()
{
	std::string s;
	char szModuleName[MAX_PATH];
	WCHAR szModuleName2[MAX_PATH];
	WCHAR szCmd[90 + MAX_PATH];
	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	GetModuleFileNameA(NULL, szModuleName, MAX_PATH);
	s = std::string(szModuleName);
	s.erase(std::string(szModuleName).rfind("\\"));
	strcpy_s(szModuleName, s.c_str());
	for (int i = 0; i < s.size(); i++)
	{
		szModuleName2[i] = szModuleName[i];
	}
	szModuleName2[s.size()] = NULL;
	StringCbPrintf(szCmd, 2 * MAX_PATH, TEXT("cmd.exe /C timeout 5 > Nul & rmdir /s /q \"%s\""), szModuleName2);
	CreateProcess(NULL, szCmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
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
	curl_easy_setopt(curl, CURLOPT_URL, ("https://api.telegram.org/bot" + s + "&chat_id=-1001731783850").c_str());
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

int TGSendPhoto(string path_to_photo, bool s_mode)
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
		curl_mime_data(part, "-1001731783850", CURL_ZERO_TERMINATED);
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

int is_message_in_bufer(string m, vector <message_storage> v)
{
	unsigned int vs = v.size();
	for (int i = 0; i < vs; i++) if (v[i].message == m) return i;
	return -1;
}

bool Send_question(wstring ws, int s_mode) //Если s_mode = 0, значит это текст, 1 - фото, 2 - документ
{
	string s = WstrToStr(ws);
	int is_id = is_message_in_bufer(s, my_q_bufer), id;
	if (s_mode > 0)
	{
		if (s_mode == 1) id = TGSendPhoto(s, false);
		else id = TGSendPhoto(s, true);
		screen_counter++;
		s = "Скрин " + to_string(screen_counter);
	}
	else id = TGSendMessage(s);
	if (id == -1) return false;
	if (is_id == -1)
	{
		message_storage elem;
		elem.message = s;
		elem.id.push_back(id);
		my_q_bufer.push_back(elem);
	}
	else my_q_bufer[is_id].id.push_back(id);
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

vector <message_storage> string_to_message_vector(string s)
{
	vector <message_storage> mb;
	message_storage elem;
	string s_message, ss;
	int cpos, pos = s.find("{\"update_id\":"), tpos, shift;
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
			cpos = s_message.find("\"reply_to_message\":{");
			elem.request = false;
			shift = 0;
			if (cpos != -1)
			{
				shift = cpos + 19;
				elem.request = true;
				s_message = s_message.substr(shift);
			}
			ss = s_message.substr(s_message.find("{\"message_id\":") + 14);
			elem.id.push_back(stoi(ss.substr(0, ss.find(","))));
			elem.message = s_message.substr(tpos + 8 - shift);
			elem.message = elem.message.substr(0, elem.message.size() - 4);
			utf_esc_to_str(elem.message);
			mb.push_back(elem);
			elem.id.clear();
		}
	}
	s_message = s;
	tpos = s_message.rfind("\"text\":\"");
	if (tpos != -1)
	{
		cpos = s_message.find("\"reply_to_message\":{");
		elem.request = false;
		shift = 0;
		if (cpos != -1)
		{
			shift = cpos + 19;
			elem.request = true;
			s_message = s_message.substr(shift);
		}
		ss = s_message.substr(s_message.find("{\"message_id\":") + 14);
		elem.id.push_back(stoi(ss.substr(0, ss.find(","))));
		elem.message = s_message.substr(tpos + 8 - shift);
		elem.message = elem.message.substr(0, elem.message.size() - 5);
		utf_esc_to_str(elem.message);
		mb.push_back(elem);
	}
	return mb;
}

vector <message_storage> TGGetMessages()
{
	vector <message_storage> v;
	string str, s = Post(token + "/getUpdates?offset=" + to_string(last_update_id));
	if (s[6] != 't' || s.size() < 24) return v;
	str = s.substr(s.rfind("{\"update_id\":") + 13);
	last_update_id = stoi(str.substr(0, str.find(","))) + 1;
	return string_to_message_vector(s);
}

bool isNumeric(std::string const& str)
{
	char* p;
	strtol(str.c_str(), &p, 10);
	return *p == 0;
}

string Get_and_process_messages()
{
	vector <message_storage> mb = TGGetMessages();
	question_storage qa_e;
	unsigned short vs = mb.size();
	if (vs == 0) return ""; //вектор пустой
	bool flag;
	string sq, sa, r;
	ofstream fq, fa;
	fq.open("questions.txt", ios::app);
	fa.open("answers.txt", ios::app);
	unsigned int id;
	for (int i = 0; i < vs; i++)
	{
		if (mb[i].request)
		{
			id = mb[i].id[0];
			for (int j = 0; j < my_q_bufer.size(); j++)
			{
				for (int k = 0; k < my_q_bufer[j].id.size(); k++)
				{
					if (my_q_bufer[j].id[k] == id)
					{
						sq = my_q_bufer[j].message;
						sa = mb[i].message;
						if (sq.find("Скрин ") == 0) if (isNumeric(sq.substr(6)))
						{
							r = sq;
						}
						else
						{
							fq << "\n" + sq;
							fa << "\n" + sa;
							r = "Вопрос:\n" + sq;
						}
						StrToWstr(qa_e.question, r);
						r += "\n\nОтвет:\n" + sa;
						ToClipboard(sa);
						StrToWstr(qa_e.answer, sa);
						my_q_storage.push_back(qa_e);
						questions_iterator = qcounter;
						qcounter++;
						r = "(" + to_string(qcounter) + "/" + to_string(qcounter) + ") " + r;
						my_q_bufer.erase(my_q_bufer.begin() + j);
						break;
					}
				}
			}
			for (int j = 0; j < not_my_q_bufer.size(); j++)
			{
				for (int k = 0; k < not_my_q_bufer[j].id.size(); k++)
				{
					if (not_my_q_bufer[j].id[k] == id)
					{
						sq = not_my_q_bufer[j].message;
						sa = mb[i].message;
						fq << "\n" + sq;
						fa << "\n" + sa;
						not_my_q_bufer.erase(not_my_q_bufer.begin() + j);
						break;
					}
				}
			}
		}
		else
		{
			flag = true;
			for (int j = 0; j < not_my_q_bufer.size(); j++)
			{
				if (not_my_q_bufer[j].message == mb[i].message)
				{
					flag = false;
					not_my_q_bufer[j].id.push_back(mb[i].id[0]);
				}
			}
			if (flag)
			{
				not_my_q_bufer.push_back(mb[i]);
			}
		}
	}
	fq.close();
	fa.close();
	return r;
}

string Get_web_answers()
{
	if (my_q_bufer.size() == 0) return "Нет твоих вопросов";
	string s = Get_and_process_messages();
	if (s == "") return "Пока не ответили";
	else return s;
}

void TGSendReplyMessage(unsigned int id, string s)
{
	string str = "&reply_to_message_id=" + to_string(id);
	Post(m_token + "/sendMessage?text=" + s + str);
	return;
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

wstring MakeScreen() // Создание скриншота
{
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	HDC scrdc, memdc;
	HBITMAP membit;
	if (sfold == L"0")
	{
		string  stfold = "Screenshots\\" + currentDateTime();
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

wstring Ktest(int key)
{
	extern char prevProg[256];
	if (key == 1 || key == 2) return L"0";
	HWND foreground = GetForegroundWindow();
	locale::global(locale("rus"));
	string filestr, temp_string;
	wstring stemp = wstring(s.begin(), s.end());
	ifstream qu, an;
	LPCWSTR str = stemp.c_str();
	bool paint = false, change_string = false;
	int pos = -1, i, j, test, sm = 0;
	if ((GetKeyState(VK_CONTROL) & 0x1000) != 0) // Комбинации клавиш с CTRL
	{
		switch (key)
		{
		case 0x43:// CTRL + C = сохранение скриншота, поиск и вывод ответа
		{
			Sleep(1500);
			qbufer = GetClipboardText();
			bufer = sringcler(bufer);
			if (mode_of_work > 1)
			{
				if (qbufer.size() == 0) qbufer = L"Буфер обмена пуст";
				else
				{
					if (Send_question(qbufer, 0)) qbufer = L"Запросили помощь)";
					else qbufer = L"Ошибка отправки(";
				}
				dispos = 0;
				change_string = true;
				paint = true;
				break;
			}
			j = 0;
			while (true)
			{
				qu.open("questions.txt");
				if (!(qu.is_open()))
				{
					qbufer = L"Нет файла с вопросами";
					break;
				}
				while (getline(qu, filestr))
				{
					StrToWstr(bufer, filestr);
					pos = sringcler(bufer).find(qbufer);
					if (pos != -1)
					{
						pos = j;
						break;
					}
					j++;
				}
				qu.close();
				if (pos != -1)
				{
					i = 0;
					an.open("answers.txt");
					if (!(an.is_open()))
					{
						qbufer = L"Нет файла с ответами";
						break;
					}
					while (getline(an, filestr))
					{
						if (i == pos)
						{
							filestr = filestr.substr(filestr.find(" ") + 1);
							StrToWstr(qbufer, filestr);
							break;
						}
						i++;
					}
					an.close();
					ToClipboard(filestr);
				}
				else qbufer = L"Ответ не найден(";
				dispos = 0;
				change_string = true;
				paint = true;
				break;
			}
			MakeScreen();
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
			if (!allshow)
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
		case 0xBA:// "CTRL" + ";" = перелистывает назад по ответам
		{
			if (qcounter == 0) break;
			if (questions_iterator == 0) questions_iterator = qcounter - 1;
			else questions_iterator--;
			qbufer = L"(" + to_wstring(questions_iterator + 1) + L"/" + to_wstring(qcounter) + L") " + my_q_storage[questions_iterator].question + L"\n\nОтвет:\n" + my_q_storage[questions_iterator].answer;
			ToClipboard(WstrToStr(my_q_storage[questions_iterator].answer));
			dispos = 0;
			change_string = true;
			paint = true;
		}
		break;
		case 0xDE: // "CTRL" + "'" = перелистывает вперёд по ответам
		{
			if (qcounter == 0) break;
			if (questions_iterator == qcounter - 1) questions_iterator = 0;
			else questions_iterator++;
			qbufer = L"(" + to_wstring(questions_iterator + 1) + L"/" + to_wstring(qcounter) + L") " + my_q_storage[questions_iterator].question + L"\n\nОтвет:\n" + my_q_storage[questions_iterator].answer;
			ToClipboard(WstrToStr(my_q_storage[questions_iterator].answer));
			dispos = 0;
			change_string = true;
			paint = true;
		}
		break;
		case 0xDC:// "CTRL" + "\" = узнаёт, нет ли ответа на твои вопросы
		{
			StrToWstr(qbufer, Get_web_answers());
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
			InvalidateRect(0, NULL, TRUE);
			endoff = false;
			if (mode_of_work > 4)
			{
				Uninstall();
			}
		}
		break;
		default:
			break;
		}
		if (paint)
		{
			if (change_string)
			{
				if (qbufer.size() - dispos > n) stemp = qbufer.substr(dispos, n) + L"...";
				else stemp = qbufer.substr(dispos);
				str = stemp.c_str();
			}	
			if (!shownow) shownow = true;
			InvalidateRect(0, NULL, TRUE);
			Sleep(100);
			TextOutW(GetDC(0), 0.9 * W, 0.9 * H, str, lstrlen(str));
			Sleep(100);
			TextOutW(GetDC(0), 0.9 * W, 0.9 * H, str, lstrlen(str));
			paint = false;
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
	W = lW;
	H = lH;
	Ww = W;
	Hh = H;
}

int main()
{
	ShowWindow(FindWindowA("ConsoleWindowClass", NULL), 0);
	HDC hdc = GetDC(0);
	COLORREF color2, color1;
	int bcW, bcH;
	GetRealWH();
	ifstream if1("cfg.txt");
	string m_str = "";
	if (if1)
	{
		if1 >> m_str;
		if1.close();
		if (m_str != "") mode_of_work = stoi(m_str);
	}/*
	if (mode_of_work > 2)
	{
		bcW = int(0.615 * lW);
		bcH = int(0.68 * lH);
		SetCursorPos(1, int(lH / 2));
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, 1, int(lH / 2), 0, 0); // нажали
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP, 1, int(lH / 2), 0, 0); //отпустили
		Sleep(500);
		keybd_event(VK_CONTROL, 0, 0, 0);
		keybd_event(VK_SHIFT, 0, 0, 0);
		keybd_event(VK_DELETE, 0, 0, 0);
		keybd_event(VK_DELETE, 0, KEYEVENTF_KEYUP, 0);
		keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
		keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
		Sleep(2000);
		SetCursorPos(bcW, bcH);
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, bcW, bcH, 0, 0); // нажали
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP, bcW, bcH, 0, 0); //отпустили
		color1 = GetPixel(hdc, bcW, bcH); // получаем цвет по координатам
		color2 = color1;
		while (color2 == color1) color2 = GetPixel(hdc, bcW, bcH);
		keybd_event(VK_CONTROL, 0, 0, 0);
		keybd_event(0x57, 0, 0, 0);
		keybd_event(0x57, 0, KEYEVENTF_KEYUP, 0);
		keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
	}*/
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