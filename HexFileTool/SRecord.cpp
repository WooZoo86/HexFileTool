#include "SRecord.h"

/*
 *Motorola S-record is a file format, created by Motorola,
 *that conveys binary information in ASCII hex text form.
 *This file format may also be known as SRecord, SREC, S19, S28, S37.
 *It is commonly used for programming flash memory in microcontrollers,
 *EPROMs, EEPROMs, and other types of programmable logic devices.
 * see more http://wikipedia.moesalih.com/S19_(file_format)
 */


char hex2bin(char ch)
{
	unsigned char tmp = 0;

	tmp = ch - '0';
	if (tmp > ('F' - '0') || (9 < tmp && tmp < ('A' - '0')))
	{
		return -1;
	}

	if (tmp > 9)
	{
		tmp -= 'A' - '9' - 1;
	}

	return tmp;
}

bool isHexNumber(const char* pBuf, int bufSize)
{
	if ((NULL == pBuf) || (0 == bufSize))
	{
		return false;
	}

	for (int i = 0; i < bufSize; ++i)
	{
		if (('0' > pBuf[i] || '9' < pBuf[i])
			&& ('A' > pBuf[i] || 'F' < pBuf[i])
			&& ('a' > pBuf[i] || 'f' < pBuf[i]))
		{
			return false;
		}
	}

	return true;
}

unsigned int HexToBin(const char* hex, int len)
{
	unsigned int nResult = 0;

	//"34" 2
	for (int i = len - 1; i >= 0; i--)
	{
		nResult += pow(16, len - i - 1) * hex2bin(hex[i]);
	}

	return  nResult;
}


SRecord::SRecord()
{
	
}

SRecord::~SRecord()
{
	Release();
}

bool SRecord::GetErr() const
{
	return err;
}

void SRecord::Reset()
{
	Release();

	err = false;
}

int SRecord::ToBin(const char* in, const char* out)
{
	ifstream iFile(in, ifstream::in | ifstream::binary);
	if (!iFile.is_open())
	{
		return -1;
	}

	string szBin = in;
	out == nullptr ? szBin += ".bin" : szBin = out;
	ofstream oFile(szBin, ofstream::out | ofstream::binary);
	if (!oFile.is_open())
	{
		iFile.close();
		return -2;
	}

	int nRet = 0;
	iFile.setf(ifstream::hex);
	SRecord record;
	while (!iFile.eof())
	{
		
		iFile >> record;
		nRet = record.GetErr();
		if (nRet)
		{
			break;
		}

		oFile << record;
	}

	oFile.flush();
	oFile.close();
	iFile.close();
	return nRet;
}

void SRecord::Release()
{
	if (data)
	{
		delete[] data;
		data = nullptr;
	}
}


ifstream& operator>>(ifstream& istream, SRecord& record)
{
	record.Reset();
	istream >> record.type;
	while ('S' != record.type && 's' != record.type && !istream.eof())
	{
		istream >> record.type;
	}

	if ('S' != record.type && 's' != record.type)//invalid srecord sign, only 'S'
	{
		record.err = 0;
		return istream;
	}

	istream >> record.type;
	if (!isdigit(record.type))//invalid srecord m_type, only 0~9
	{
		record.err = 2;
		return istream;
	}

	char chs[8] = { 0 };
	istream.read(chs, 2);
	if (!isHexNumber(chs, 2))
	{
		record.err = 3;
		return istream;
	}

	record.count = HexToBin(chs, 2);
	if (record.count > 0x20 || record.count < 0)
	{
		record.err = 4;
		return istream;
	}

	record.data = new unsigned char[record.count];
	if (!record.data)
	{
		record.err = 5;
		return istream;
	}

	unsigned char AddressLen = 0;
	switch (record.type)
	{
	case '0'://header
		AddressLen = 2;
		break;
	case '1'://data
	case '2':
	case '3':
		AddressLen = record.type - '1' + 2;
		break;
	case '4'://Reserved
		record.err = 6;
		return istream;
	case '5'://Count
	case '6'://Count
		AddressLen = record.type - '5' + 2;
		break;
	case '7'://Start Address Termination
	case '8':
	case '9':
		AddressLen = '9' - record.type + 2;
	default:
		break;
	}

	istream.read(chs, AddressLen * 2);
	if (!isHexNumber(chs, AddressLen * 2))
	{
		record.err = 7;
		return istream;
	}
	record.address = HexToBin(chs, AddressLen * 2);
	record.len = 0;
	unsigned int checksum = 0;
	for (int i = 0; i < record.count - AddressLen - 1; ++i)
	{
		istream.read(chs, 2);
		if (!isHexNumber(chs, 2))
		{
			record.err = 8;
			return istream;
		}

		record.data[i] = HexToBin(chs, 2);
		record.len++;
		checksum += record.data[i];
	}

	if (record.len != record.count - AddressLen - 1)
	{
		record.err = 9;
		return istream;
	}

	istream.read(chs, 2);
	if (!isHexNumber(chs, 2))
	{
		record.err = 10;
		return istream;
	}

	checksum += record.count;
	for (int i = 0; i < AddressLen; ++i)
	{
		checksum += (((unsigned char)(record.address >> i * 8)) & 0xFF);
	}

	checksum ^= 0xFF;
	record.checksum = HexToBin(chs, 2);
	if (record.checksum != (unsigned char)checksum)
	{
		record.err = 11;
		return istream;
	}

	return istream;
}

ofstream& operator<<(ofstream& ostream, SRecord& record)
{
	for (unsigned int i = 0; i < record.len; ++i)
	{
		if (record.type == '1' || record.type == '2' || record.type == '3')
		{
			ostream << record.data[i];
		}
	}

	return ostream;
}


BinRecord::BinRecord(unsigned start, bool count) : BinRecord(start, nullptr, nullptr, 24, 32, count)
{
}

BinRecord::BinRecord(unsigned start, unsigned bit) : BinRecord(start, nullptr, nullptr, bit, 32, false)
{
}

BinRecord::BinRecord(unsigned start, unsigned bit, unsigned size) : BinRecord(start, nullptr, nullptr, bit, size, false)
{
}

BinRecord::BinRecord(unsigned start, unsigned bit, bool count): BinRecord(start, nullptr, nullptr, bit, 32, count)
{
}

BinRecord::BinRecord(unsigned start, unsigned bit, unsigned size, bool count) : BinRecord(start, nullptr, nullptr, bit, size, count)
{
}

/*
 * start	起始地址
 * header	头部字符串，要么使用通用的，要么根据自定义  
 * tail		尾部字符串，不是自定义就按规则生成
 * bit		选择的地址位宽，将决定用S1-S3和S7-S9中的一种
 * size		每条记录数据大小
 */
BinRecord::BinRecord(unsigned int start, const char* header, const char* tail, unsigned int bit, unsigned int size, bool count)
{
	m_Start = start;
	m_bCount = count;
	m_Header = common_header;
	if (header)
	{
		m_Header = header;
	}

	if (tail)
	{
		m_Tail = tail;
	}

	SetAddressBit(bit);
	SetDataSize(size);
}

BinRecord::~BinRecord()
{
}

int BinRecord::ToSRecord(const char* in, const char* out)
{
	fstream iFile(in, fstream::in | fstream::binary);
	if (!iFile.is_open())
	{
		return -1;
	}
	
	string szBin = in;
	out == nullptr ? szBin += ".s19" : szBin = out;
	ofstream oFile(szBin, ofstream::out | ofstream::binary);
	if (!oFile.is_open())
	{
		iFile.close();
		return -2;
	}
	
	//写入头部 请确保数据的合法性
	oFile << m_Header;

	//写入数据
	iFile.seekg(0, ofstream::_Seekend);
	size_t fsize = iFile.tellg();
	iFile.seekg(0, ofstream::_Seekbeg);

	const unsigned int len = m_DataSize - m_AddressSize - 1;//实际数据大小需要减去地址大小和校验和
	unsigned int address = m_Start;
	unsigned int loop = fsize / len;
	unsigned int remain = fsize % len;
	static char buf[256] = { 0 };
	static char data[256] = { 0 };
	unsigned int i = 0;
	if (loop)
	{
		do
		{
			memset(buf, 0, 256);
			memset(data, 0, 256);

			iFile.read(data, len);
			GetLine(m_type, m_DataSize, address, m_AddressSize, buf, 256, data, len);
			oFile << buf << "\r\n";
			
			address += len;
			++i;
		} while (i < loop);
	}

	if (remain)//通常应该是按32对齐的，不会进入下面分支
	{
		memset(buf, 0, 256);
		memset(data, 0, 256);

		iFile.read(data, remain);
		GetLine(m_type, m_DataSize, address, m_AddressSize, buf, 256, data, len);
		oFile << buf << "\r\n";
		i++;
	}

	//如果需要写入S5或S6
	if (m_bCount)
	{
		memset(buf, 0, 256);
		if (i > 0xFFFF)//需要S6
		{
			GetLine("S6", 4, i, 3, buf, 256, nullptr, 0);
		}
		else
		{
			GetLine("S5", 3, i, 2, buf, 256, nullptr, 0);
		}
		
		oFile << buf << "\r\n";
	}
	
	//写入尾部 请确保数据的合法性
	if (m_Tail)
	{
		oFile << m_Tail;
	}
	else
	{
		memset(buf, 0, 256);
		
		//S7-8-9
		m_type[1] = '9' - m_AddressSize + 2;
		GetLine(m_type, m_DataSize, address, m_AddressSize, buf, 256, nullptr, 0);
		oFile << buf << "\r\n";
		m_type[1] = '0' + m_AddressSize - 1;
	}

	oFile.flush();
	oFile.close();
	iFile.close();
	return 0;
}

unsigned int BinRecord::SetAddressBit(unsigned int bit)
{
	const unsigned int old = m_AddressSize;
	if (bit)
	{
		m_AddressSize = bit > 32 ? 4 : (bit / 8);
		SetType(m_AddressSize);
	}
	
	return old;
}

unsigned int  BinRecord::SetDataSize(unsigned int size)
{
	const unsigned int old = m_DataSize;
	if (size)
	{
		m_DataSize = size > 32 ? 32 : size;
	}
	
	return old;
}

unsigned int  BinRecord::SetStart(unsigned int start)
{
	const unsigned int old = m_Start;
	if (start)
	{
		m_Start = start >= 0xFFFFFFFF ? 0 : start;
	}

	return old;
}

bool BinRecord::SetKeepCount(bool yes)
{
	bool old = m_bCount;
	m_bCount = yes;

	return old;
}

void BinRecord::SetType(unsigned int AddrSize)
{
	switch (AddrSize)
	{
	case 16:
		m_type[1] = '1';
		break;
	case 24:
		m_type[1] = '2';
		break;
	case 32:
		m_type[1] = '3';
		break;
	default:
		break;
	}
}


void BinRecord::GetLine(const char* type, unsigned int count, unsigned int address, unsigned int addressSize, char* buf, unsigned int len, char* data, unsigned int size)
{
	unsigned char sum;
	unsigned int i;

	sum = 0xFF - count;
	//类型 + count + 地址
	snprintf(buf, len, "%2s%02X%*X", type, count, addressSize, address);
	for (i = 0; i < addressSize; ++i)
	{
		sum -= (address >> (i * 8)) & 0xFF;
	}

	//数据
	char* pData = buf + 4 + 2 * addressSize;
	for (i = 0; i < size; ++i)
	{
		sum -= data[i];
		snprintf(pData, 3, "%02X", (unsigned char)data[i]);
		pData += 2;
	}

	//校验和
	snprintf(pData, len, "%02X", sum);
}