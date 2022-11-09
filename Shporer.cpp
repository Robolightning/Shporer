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
#include <locale>
#include <windows.h>
#include <vector>
#include <curl/curl.h>

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
int W = GetSystemMetrics(SM_CXSCREEN), H = GetSystemMetrics(SM_CYSCREEN);
int Ww = W, Hh = H, n = 22, dispos = 0;

using namespace std;

struct message_storage
{
	bool request = false;
	string message;
	vector <unsigned int> id;
};

vector <message_storage> my_q_bufer, not_my_q_bufer;

unsigned int last_update_id = 0;
unsigned short not_my_questions_iterator = 0;
string token = "впишите сюда токен бота", m_token = "и сюда токен бота";

size_t convert_codepoint_to_utf8(char(&chr)[4], unsigned int cp)
{
	char* result = chr;

	if(cp < 0x80)         /* one octet */
	{
		*(result++) = (unsigned char)(cp);
	}
	else if(cp < 0x800)   /* two octets */
	{
		*(result++) = (unsigned char)((cp >> 6) | 0xc0);
		*(result++) = (unsigned char)((cp & 0x3f) | 0x80);
	}
	else if(cp < 0x10000) /* three octets */
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
	while(*str)
	{
		if(*str == '\\' && *(str + 1) == 'u')
		{
			str += 2;
			char hex[5] = {};
			size_t c = 0;
			while(is_hex_character(*str) && c < 4) hex[c++] = *str++;
			if(c > 0)
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
	if(!result_u) return "0";
	wchar_t* ures = new wchar_t[result_u];
	if(!MultiByteToWideChar(1251, 0, str, -1, ures, result_u))
	{
		delete[] ures;
		return "0";
	}
	result_c = WideCharToMultiByte(65001, 0, ures, -1, 0, 0, 0, 0);
	if(!result_c)
	{
		delete[] ures;
		return "0";
	}
	char* cres = new char[result_c];
	if(!WideCharToMultiByte(65001, 0, ures, -1, cres, result_c, 0, 0))
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
	if(OpenClipboard(0))
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
	while((pos = s.find(' ')) != string::npos) s.replace(pos, 1, "%20");
}

string Post(string s)
{
	replaceSpaces(s);
	s = cp1251_to_utf8(s.c_str());
	string post_ansver;
	CURL* curl;
	CURLcode response;
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, ("https://api.telegram.org/bot" + s + "&chat_id=-1001731783850").c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getResponsetoString);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &post_ansver);
	response = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	return post_ansver;
}

int TGSendMessage(string s)
{
	int pos = s.find("\"message_id\":");
	s = Post(m_token + "/sendMessage?text=" + s);
	if(pos == -1) return -1;
	s = s.substr(pos + 13);
	return stoi(s.substr(0, s.find(",")));
}

int is_message_in_bufer(string m, vector <message_storage> v)
{
	unsigned int vs = v.size();
	for(int i = 0; i < vs; i++) if(v[i].message == m) return i;
	return -1;
}

bool Send_question(string s)
{
	int is_id = is_message_in_bufer(s, my_q_bufer), id = TGSendMessage(s);
	if(id == -1) return false;
	if(is_id == -1)
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
	while(pos != -1)
	{
		s_message = s.substr(0, pos);
		tpos = s_message.rfind("\"text\":\"");
		s = s.substr(pos + 13);
		pos = s.find("{\"update_id\":");
		if(tpos != -1)
		{
			cpos = s_message.find("\"reply_to_message\":{");
			elem.request = false;
			shift = 0;
			if(cpos != -1)
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
	if(tpos != -1)
	{
		cpos = s_message.find("\"reply_to_message\":{");
		elem.request = false;
		shift = 0;
		if(cpos != -1)
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
	if(s[6] != 't' || s.size() < 24) return v;
	str = s.substr(s.rfind("{\"update_id\":") + 13);
	last_update_id = stoi(str.substr(0, str.find(","))) + 1;
	return string_to_message_vector(s);
}

string Get_and_process_messages(bool answers)
{
	vector <message_storage> mb = TGGetMessages();
	unsigned short vs = mb.size(), qcounter = 1;
	if(vs == 0) return ""; //вектор пустой
	bool flag;
	string sq, sa, r;
	ofstream fq, fa;
	fq.open("questions.txt", ios::app);
	fa.open("answers.txt", ios::app);
	unsigned int id;
	for(int i = 0; i < vs; i++)
	{
		if(mb[i].request)
		{
			id = mb[i].id[0];
			for(int j = 0; j < my_q_bufer.size(); j++)
			{
				for(int k = 0; k < my_q_bufer[j].id.size(); k++)
				{
					if(my_q_bufer[j].id[k] == id)
					{
						sq = my_q_bufer[j].message;
						sa = mb[i].message;
						fq << "\n" + sq;
						fa << "\n" + sa;
						if(answers)
						{
							r += to_string(qcounter) + "\nВопрос:\n" + sq + "\n\nОтвет:\n" + sa;
							qcounter++;
						}
						my_q_bufer.erase(my_q_bufer.begin() + j);
						break;
					}
				}
			}
			for(int j = 0; j < not_my_q_bufer.size(); j++)
			{
				for(int k = 0; k < not_my_q_bufer[j].id.size(); k++)
				{
					if(not_my_q_bufer[j].id[k] == id)
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
			for(int j = 0; j < not_my_q_bufer.size(); j++)
			{
				if(not_my_q_bufer[j].message == mb[i].message)
				{
					flag = false;
					not_my_q_bufer[j].id.push_back(mb[i].id[0]);
				}
			}
			if(flag)
			{
				not_my_q_bufer.push_back(mb[i]);
			}
		}
	}
	fq.close();
	fa.close();
	return r;
}

string Get_web_ansvers()
{
	if(my_q_bufer.size() == 0) return "Нет твоих вопросов";
	string s = Get_and_process_messages(true);
	if(s == "") return "Пока не ответили";
	else
	{
		ToClipboard(s);
		return s;
	}
}

string Get_web_questions()
{
	unsigned short vs;
	Get_and_process_messages(false);
	vs = not_my_q_bufer.size();
	if(vs == 0) return "Пока нет вопросов";
	else
	{
		string s;
		not_my_questions_iterator++;
		if(not_my_questions_iterator > vs) not_my_questions_iterator = 1;
		s = "(" + to_string(not_my_questions_iterator) + "/" + to_string(vs) + ") " + not_my_q_bufer[not_my_questions_iterator - 1].message;
		ToClipboard(s);
		return s;
	}
}

void TGSendReplyMessage(unsigned int id, string s)
{
	string str = "&reply_to_message_id=" + to_string(id);
	Post(m_token + "/sendMessage?text=" + s + str);
	return;
}

void Send_answer_to_web_question(string s)
{
	if(not_my_q_bufer.size() != 0)
	{
		for(int i = 0; i < not_my_q_bufer[not_my_questions_iterator - 1].id.size(); i++)
		{
			TGSendReplyMessage(not_my_q_bufer[not_my_questions_iterator - 1].id[i], s);
		}
		not_my_q_bufer.erase(not_my_q_bufer.begin() + not_my_questions_iterator - 1);
	}
}

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

wstring GetClipboardText()
{
	if(!OpenClipboard(nullptr))
	{
		CloseClipboard();
		return L"";
	}
	HANDLE hData = GetClipboardData(CF_UNICODETEXT);
	if(hData == nullptr)
	{
		CloseClipboard();
		return L"";
	}
	wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
	if(pszText == nullptr)
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

void MakeScreen() // Создание скриншота
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
}

wstring Ktest(int key)
{
	extern char prevProg[256];
	if (key == 1 || key == 2) return L"0";
	HWND foreground = GetForegroundWindow();
	locale::global(locale("rus"));
	string filestr;
	wstring stemp = wstring(s.begin(), s.end());
	ifstream qu, an;
	LPCWSTR str = stemp.c_str();
	bool paint = false;
	int pos = -1, i, j;
	int test;
	if ((GetKeyState(VK_CONTROL) & 0x1000) != 0) // Комбинации клавиш с CTRL
	{
		switch (key)
		{
		case 0x43:// "CTRL" + "C" = сохранение скриншота, поиск и вывод ответа
		{
			Sleep(1500);
			qbufer = GetClipboardText();
			bufer = sringcler(bufer);
			str = qbufer.c_str();
			stemp = qbufer;
			j = 0;
			while(true)
			{
				qu.open("questions.txt");
				if(!(qu.is_open()))
				{
					stemp = L"Нет файла с вопросами";
					str = stemp.c_str();
					break;
				}
				while(getline(qu, filestr))
				{
					StrToWstr(bufer, filestr);
					pos = sringcler(bufer).find(stemp);
					if(pos != -1)
					{
						pos = j;
						break;
					}
					j++;
				}
				qu.close();
				if(pos != -1)
				{
					i = 0;
					an.open("ansvers.txt");
					if(!(an.is_open()))
					{
						stemp = L"Нет файла с ответами";
						str = stemp.c_str();
						break;
					}
					while(getline(an, filestr))
					{
						if(i == pos)
						{
							filestr = filestr.substr(filestr.find(" ") + 1);
							StrToWstr(qbufer, filestr);
							break;
						}
						i++;
					}
					an.close();
					ToClipboard(filestr);
					if(qbufer.size() > n) stemp = qbufer.substr(0, n) + L"...";
					else stemp = qbufer;
				}
				else stemp = L"Ответ не найден(";
				str = stemp.c_str();
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
			if(n != 2) n -= 2;
			if(stemp.size() > n) stemp = stemp.substr(0, n) + L"...";
			else stemp = qbufer;
			str = stemp.c_str();
			paint = true;
		}
		break;
		case 0xDD:// "CTRL" + "]" = Увеличивает количество выводимых символов на 2
		{
			n += 2;
			test = (qbufer.substr(dispos)).size();
			if((qbufer.substr(dispos)).size() >= n - 1) stemp = qbufer.substr(dispos, n) + L"...";
			else stemp = qbufer;
			str = stemp.c_str();
			paint = true;
		}
		break;
		case 0xBC:// "CTRL" + "<" = Сдвигает стоку на 10 символов влево
		{
			if(qbufer.size() > n && dispos != 0)
			{
				if(dispos < n / 2)
				{
					dispos = 0;
					stemp = qbufer.substr(0, n);
				}
				else
				{
					dispos -= n / 2;
					stemp = qbufer.substr(dispos, n);
				}
				stemp += L"...";
				str = stemp.c_str();
				paint = true;
			}
		}
		break;
		case 0xBE://"CTRL" + ">" = Сдвигает стоку на 10 символов вправо
		{
			if(qbufer.size() > n && dispos < qbufer.size() - n)
			{
				if(dispos > qbufer.size() - 1.5 * n)
				{
					dispos = qbufer.size() - n;
					stemp = qbufer.substr(dispos);
				}
				else
				{
					dispos += n / 2;
					stemp = qbufer.substr(dispos, n) + L"...";
				}
				str = stemp.c_str();
				paint = true;
			}
		}
		break;
		case 0x42:// "CTRL" + "B" = показать весь текст разом
		{
			if(!allshow)
			{
				InvalidateRect(0, NULL, TRUE);
				pos = 1;
				stemp = qbufer;
				if(size(stemp) > n)
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
				while(size(stemp) > n)
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
				if(size(qbufer) > n) s = qbufer.substr(0, n) + L"...";
				else s = qbufer;
				allshow = false;
				paint = true;
			}
		}
		break;
		case 0xC0:// "CTRL" + "~" = показывает текст/ скрывает текст
		{
			if(shownow) InvalidateRect(0, NULL, TRUE);
			else TextOutW(GetDC(0), 0.9 * W, 0.9 * H, str, lstrlen(str));
			shownow = !shownow;
		}
		break;
		case 0xBF:// "CTRL" + "/" = запрашивает помощь по вопросу, скопированному в буфер обмена
		{
			qbufer = GetClipboardText();
			if(qbufer.size() == 0) stemp = L"Буфер обмена пуст";
			else
			{
				if(Send_question(WstrToStr(qbufer))) stemp = L"Запросили помощь)";
				else stemp = L"Ошибка отправки(";
			}
			str = stemp.c_str();
			paint = true;
		}
		break;
		case 0xDC:// "CTRL" + "\" = узнаёт, нет ли ответа на твои вопросы
		{
			StrToWstr(stemp, Get_web_ansvers());
			str = stemp.c_str();
			paint = true;
		}
		break;
		case 0xBA:// "CTRL" + ";" = предоставляет содержимое буфера обмена, как ответ на текущий, взятый у кого-то, вопрос
		{
			qbufer = GetClipboardText();
			if(qbufer.size() == 0) stemp = L"Буфер обмена пуст";
			else
			{
				Send_answer_to_web_question(WstrToStr(qbufer));
				stemp = L"Ответ отправлен)";
			}
			str = stemp.c_str();
			paint = true;
		}
		break;
		case 0xDE:// "CTRL" + "'" = запрашивает текущие открытые вопросы других людей
		{
			StrToWstr(stemp, Get_web_questions());
			str = stemp.c_str();
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
		}
		break;
		default:
			break;
		}
		if (paint)
		{
			if(!shownow) shownow = true;
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

int main()
{
	ifstream iff("token.txt");
	string f_token;
	if(iff)
	{
		iff >> f_token;
		iff.close();
		if(f_token != "") m_token = f_token;
	}
	ShowWindow(FindWindowA("ConsoleWindowClass", NULL), 0);
	if(!(hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallback, NULL, 0))) MessageBox(NULL, LPWSTR("Что-то пошло не так!"), LPWSTR("Ошибка"), MB_ICONERROR);
	MSG message;
	while(true) GetMessage(&message, NULL, 0, 0);
}
