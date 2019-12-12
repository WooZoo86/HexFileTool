#pragma once

#include <fstream>

using namespace std;

char hex2bin(char ch);
bool isHexNumber(const char* pBuf, int bufSize);

class SRecord
{
	friend ifstream& operator>>(ifstream& istream, SRecord& record);
	friend ofstream& operator<<(ofstream& ostream, SRecord& record);
	
public:
	SRecord();
	virtual ~SRecord();

	bool GetErr() const;
	void Reset();
	static int ToBin(const char* in, const char* out = nullptr);

private:
	void Release();
	
private:
	unsigned char type{};
	unsigned char checksum{};
	unsigned char count{};
	unsigned char err{};
	unsigned int address{};
	unsigned int len{};
	unsigned char* data{};
};


class BinRecord
{
public:
	BinRecord() = delete;
	BinRecord(unsigned int start, bool count);
	BinRecord(unsigned int start, unsigned int bit);
	BinRecord(unsigned int start, unsigned int bit, unsigned int size);
	BinRecord(unsigned int start, unsigned int bit, bool count);
	BinRecord(unsigned int start, unsigned int bit, unsigned int size, bool count);
	BinRecord(unsigned int start, const char* header = nullptr, const char* tail = nullptr, unsigned int bit = 24, unsigned int size = 32, bool count = false);
	virtual ~BinRecord();

	int ToSRecord(const char* in, const char* out = nullptr);
	unsigned int SetAddressBit(unsigned int bit = 0);
	unsigned int SetDataSize(unsigned int size = 0);
	unsigned int SetStart(unsigned int start = 0);
	bool SetKeepCount(bool yes = false);
	
private:
	void GetLine(const char* type, unsigned int count, unsigned int address, unsigned int addressSize, char* buf, unsigned int len, char* data, unsigned int size);
	void SetType(unsigned int AddrLen);
	
	unsigned int m_Start = 0x218000;
	const char* m_Header = nullptr;
	const char* m_Tail = nullptr;
	unsigned int m_AddressSize = 3;
	unsigned int m_DataSize = 0;
	bool m_bCount = false;		//Êä³öS5»òS6
	char m_type[3] = { 'S','2',0 };

	constexpr static const char* common_header = "S00600004844521B\r\n";
};
