#include "QuotesSnapshot.h"
#include "../../util/net/socket_util.h"
#include "../../util/string/string_util.h"
#include "../../controller/runtime.h"
#include "../model/convert/Convert.h"
#include <string>

using namespace std;

#pragma pack(1)

// 市场分类信息
struct MarketClassified
{
	int MarketCode;										// 市场代码
	unsigned int nDate;									// 日期
	unsigned int nCrc;									// 请求Crc
	int MemCount;										// 成员数量
	int DataOffset;										// 数据偏移量
	
	MarketClassified(const int Code)
		: MarketCode(Code),
		nDate(0),
		nCrc(0),
		MemCount(0),
		DataOffset(0)
	{
		
	}
	
	MarketClassified& operator = (const MarketClassified &item)
	{
		MarketCode = item.MarketCode;
		nDate = item.nDate;
		nCrc = item.nCrc;
		MemCount = item.MemCount;
		DataOffset = item.DataOffset;
		return *this;
	}
};

enum SECURITIES_EXCHANGE {
    MARKET_SH = 0x4853, 				//上海证券交易所
    MARKET_SZ = 0x5A53, 				//深圳证券交易所
    MARKET_HK = 0x4B48, 				//香港联交所
};

struct markHead
{
	unsigned char	cSparate;
	unsigned short	type;
	unsigned short  attrs;
};

struct MarketValueMap
{
	unsigned int RecordIndex;			// 记录索引
	char strCode[10];					// 股票代码
	
	MarketValueMap& operator = (const MarketValueMap &item)
	{
		RecordIndex = item.RecordIndex;
		if (NULL != item.strCode)
		{
			strncpy(strCode, item.strCode, 10);
		}
		return *this;
	}
};

#pragma pack()

// 静态成员初始化
int CQuotesSnapshot::m_sockfd = -1;
enum QuotesSnapshotStatusEnum CQuotesSnapshot::m_nStatus = QS_NO_DEFINE;
enum NetStatus CQuotesSnapshot::m_nNetStatus = Net_Closed;
bool CQuotesSnapshot::m_bInitialed = false;
struct ACC_STK_STATIC *CQuotesSnapshot::m_pStkStatic = NULL;
struct ACC_STK_DYNA *CQuotesSnapshot::m_pStkDyna = NULL;
struct PriceAlarm* CQuotesSnapshot::m_pPriceAlarmFirst = NULL;
struct PriceAlarm* CQuotesSnapshot::m_pPriceAlarmSecond = NULL;
struct PriceAlarm* CQuotesSnapshot::m_pCurrentSearchDyna = NULL;
struct PriceAlarm* CQuotesSnapshot::m_pCurrentCalcDyna = NULL;
map<string, MarketClassified> CQuotesSnapshot::m_mpMarketClassf;
struct MarketValueMap*	CQuotesSnapshot::m_pMarketValueMap = NULL;
typedef pair<string, MarketClassified> StringMarketPair;
typedef map<string, MarketClassified>::iterator StringMarketIter;

CQuotesSnapshot::CQuotesSnapshot()
{
}


CQuotesSnapshot::~CQuotesSnapshot()
{
}
    

// 获取最大总记录数
const int 
CQuotesSnapshot::GetMaxRecordNum()
{
	const int iDefaultNum = 12288;
	const struct DataServerTag *pServerTag = GetDataServer(false);
	if (NULL == pServerTag || pServerTag->MaxRecordNum < iDefaultNum)
	{
		return iDefaultNum;
	}
	return pServerTag->MaxRecordNum;
}

// 初始化资源
void 
CQuotesSnapshot::InitialRes()
{
	if (NULL == m_pStkStatic)
	{
		m_pStkStatic = new ACC_STK_STATIC[GetMaxRecordNum()];
	}
	bzero(m_pStkStatic, GetMaxRecordNum() * sizeof(ACC_STK_STATIC));
	
	if (NULL == m_pStkDyna)
	{
		m_pStkDyna = new ACC_STK_DYNA[GetMaxRecordNum()];
	}
	bzero(m_pStkDyna, GetMaxRecordNum() * sizeof(ACC_STK_DYNA));
	
	if (NULL == m_pPriceAlarmFirst)
	{
		m_pPriceAlarmFirst = new PriceAlarm[GetMaxRecordNum()];
	}
	bzero(m_pPriceAlarmFirst, GetMaxRecordNum() * sizeof(PriceAlarm));
	
	if (NULL == m_pPriceAlarmSecond)
	{
		m_pPriceAlarmSecond = new PriceAlarm[GetMaxRecordNum()];
	}
	bzero(m_pPriceAlarmSecond, GetMaxRecordNum() * sizeof(PriceAlarm));
	
	if (NULL == m_pMarketValueMap)
	{
		m_pMarketValueMap = new MarketValueMap[GetMaxRecordNum()];
	}
	bzero(m_pMarketValueMap, GetMaxRecordNum() * sizeof(MarketValueMap));
	
	// 连接socket 
	if (Net_Normal != m_nNetStatus)
	{		
		SocketConnect(false);
	}
}

// 释放资源
void 
CQuotesSnapshot::ReleaseRes()
{
	if (NULL != m_pStkStatic)
	{
		delete []m_pStkStatic;
		m_pStkStatic = NULL;
	}
	
	if (NULL != m_pStkDyna)
	{
		delete []m_pStkDyna;
	}
	
	if (NULL != m_pPriceAlarmFirst)
	{
		delete []m_pPriceAlarmFirst;
	}
	
	if (NULL != m_pPriceAlarmSecond)
	{
		delete []m_pPriceAlarmSecond;
	}
	
	if (NULL != m_pMarketValueMap)
	{
		delete []m_pMarketValueMap;
		m_pMarketValueMap = NULL;
	}
	
	m_mpMarketClassf.clear();
	if (Net_Normal == m_nNetStatus)
	{
		CloseConnect();
	}
}

// 连接服务器
int 
CQuotesSnapshot::SocketConnect(bool ChangeSer)
{
	char cPort[24] = {0};
	const struct DataServerTag *pServerTag = GetDataServer(ChangeSer);
	sprintf(cPort, "%d", pServerTag->Port);
	if (Net_Closed != m_nNetStatus)
	{
		CloseConnect();
	}
	m_sockfd = Connect(pServerTag->Addr, cPort);
	
	if (m_sockfd > -1)
	{
		SetNoBlockMode(m_sockfd, false);
		m_nNetStatus = Net_Normal;
		NOTE("Connect data server[%s : %d] OK", pServerTag->Addr, pServerTag->Port);
		return m_sockfd;
	}
	else 
	{
		NOTE("Connect data server[%s : %d] Failed", pServerTag->Addr, pServerTag->Port);
		pServerTag = GetDataServer(true);
		sprintf(cPort, "%d", pServerTag->Port);
		m_sockfd = Connect(pServerTag->Addr, cPort);
		if (m_sockfd > -1)
		{
			SetNoBlockMode(m_sockfd, false);
			m_nNetStatus = Net_Normal;
		}
		else
		{
			m_nNetStatus = Net_Abnormal;
		}
		
	}
	
	NOTE("Connect data server[%s : %d] %s", pServerTag->Addr, pServerTag->Port, m_sockfd > -1 ? "OK" : "Failed");
	return m_sockfd;
}

// 关闭当前的连接
void 
CQuotesSnapshot::CloseConnect()
{
	if (m_sockfd >= 0)
	{
		Close(m_sockfd);
		m_sockfd = -1;
	}
	m_nNetStatus = Net_Closed;
}

// 请求静态数据
int 
CQuotesSnapshot::RequestStaicData()
{
	int iRes = 0;
	bzero(m_pStkStatic, GetMaxRecordNum());
	int iCount = 0;
	StringMarketIter iterFind = m_mpMarketClassf.begin();
	while(m_mpMarketClassf.end() != iterFind)
	{
		if (Net_Normal != m_nNetStatus)
		{
			return -1;
		}
		
		if (0 == (iRes = RequestMarketStaticData(&iterFind->second, iCount)))
		{
			iCount += iterFind->second.MemCount;
		}
		else
		{
			ERROR("RequestMarketStaticData failed[res=%d market=%s]", iRes, ParseMarketCode(iterFind->second.MarketCode));
			return -2;
		}
		iterFind++;
	}
	
	// 创建映射
	iCount = 0;
	int iOffset = 0;
	iterFind = m_mpMarketClassf.begin();
	while(m_mpMarketClassf.end() != iterFind)
	{
		for (int j = 0; j < iterFind->second.MemCount; j++)
		{
			MarketValueMap item;
			item.RecordIndex = (unsigned int)iCount;
			strncpy(item.strCode, m_pStkStatic[iCount].m_szLabel, 10);
			AddMarkMapValue((int)item.RecordIndex, &item);
			iCount++;
		}
		SortMarketIndexMapValue(iterFind->second.MemCount, iOffset);
		iOffset += iterFind->second.MemCount;
		
		iterFind++;
	}
	
	return 0;
}

// 请求市场分类的静态数据
int 
CQuotesSnapshot::RequestMarketStaticData(MarketClassified *pMarketSerial, const int DataOffset)
{
	char cSendBuf[256] = {0};
	
	// main head
	struct ACC_JAVA_HEAD head;
	head.cSparate = '{';
	head.type = HQTYPE_STATIC;
	head.attrs = 0;
	head.length = sizeof(struct ACC_STATICASKHEAD);
	
	// data
	struct ACC_STATICASKHEAD reqHead = {0};
	reqHead.m_nMarket = pMarketSerial->MarketCode;
	pMarketSerial->DataOffset = DataOffset;
	
	// assign
	memcpy(cSendBuf, &head, sizeof(ACC_JAVA_HEAD));
	memcpy(cSendBuf + sizeof(ACC_JAVA_HEAD), &reqHead, sizeof(ACC_STATICASKHEAD));
	
	// send
	int iRes = Send(m_sockfd, cSendBuf, sizeof(ACC_JAVA_HEAD) + sizeof(ACC_STATICASKHEAD), 5*1000);
	if (0 == iRes)
	{
		CloseConnect();
		return -1;
	}
	else if (-1 == iRes)
	{
		return -2;
	}
	
	// 读取头标识
	markHead tmarkHead = {0};
	int iRet = -1;
	iRet = RecvUnknowLen(m_sockfd, &tmarkHead, sizeof(markHead));
	if (iRet != sizeof(markHead))
	{
		CloseConnect();
		return -3;
	}
	
	// 读取长度
	unsigned int iTotalLen = 0;
	if (0 == tmarkHead.attrs)
	{
		unsigned short uLen = 0;
		iRet = RecvUnknowLen(m_sockfd, &uLen, sizeof(short));
		if (iRet != sizeof(short))
		{
			CloseConnect();
			return -4;
		}
		iTotalLen = uLen;
	}
	else if (0x0008 == tmarkHead.attrs)
	{
		iRet = RecvUnknowLen(m_sockfd, &iTotalLen, sizeof(int));
		if (iRet != sizeof(int))
		{
			CloseConnect();
			return -5;
		}
	}
	
	// 取ACC_STATICASKHEAD
	ACC_STATICASKHEAD askHead = {0};
	iRet = RecvUnknowLen(m_sockfd, &askHead, sizeof(ACC_STATICASKHEAD));
	if (iRet != sizeof(ACC_STATICASKHEAD))
	{
		CloseConnect();
		return -6;
	}
	iTotalLen -= sizeof(ACC_STATICASKHEAD);
	// 保留请求的ACC_STATICASKHEAD返回值
	pMarketSerial->nDate = askHead.m_nDate;
	pMarketSerial->nCrc = askHead.m_nCrc;
	
	// 计算个数
	pMarketSerial->MemCount = iTotalLen / sizeof(ACC_STK_STATIC);
	// 检测存储空间是否足够大
	if ((pMarketSerial->MemCount + DataOffset) > GetMaxRecordNum())
	{
		ERROR("DataServer.MaxRecord[%d] maybe too small to store the static data", GetMaxRecordNum());
		return -7;
	}
	
	TRACE("MarketSerial = %s  GetCount=%d", ParseMarketCode(pMarketSerial->MarketCode), pMarketSerial->MemCount);
	
	// 读取数据部分	
	ACC_STK_STATIC *pStart = m_pStkStatic + DataOffset;	
	int iRemainLen = iTotalLen;
	iRet = 0;
	while (iRemainLen > 0)
	{
		iRet = RecvUnknowLen(m_sockfd, (((char*)pStart) + iTotalLen - iRemainLen), iRemainLen);
		if (0 == iRet)
		{
			CloseConnect();
			return -8;
		}
		else if (0 > iRet)
		{
			break;
		}
		
		iRemainLen -= iRet;
	}

	return (iRemainLen == 0 && iTotalLen > 0 ) ? 0 : -9;
}

// 请求动态数据
int 
CQuotesSnapshot::RequestDynaData()
{
	if (Net_Normal != m_nNetStatus)
	{
		SocketConnect(false);
	}
	
	int iRes = 0;
	bzero(m_pStkDyna, GetMaxRecordNum());
	int iCount = 0;
	StringMarketIter iterFind = m_mpMarketClassf.begin();
	while(m_mpMarketClassf.end() != iterFind)
	{
		if (Net_Normal != m_nNetStatus)
		{
			return -1;
		}
		
		if (0 == (iRes = RequestMarketDynaData(&iterFind->second, iCount)))
		{
			iCount += iterFind->second.MemCount;
		}
		else
		{
			DEBUG("RequestMarketDynaData failed[%d]", iRes);
			return -2;
		}
		iterFind++;
	}

	return 0;
}

// 请求市场分类的动态数据
int 
CQuotesSnapshot::RequestMarketDynaData(MarketClassified *pMarketSerial, const int DataOffset)
{
	char cSendBuf[256] = {0};
	
	// main head
	struct ACC_JAVA_HEAD head;
	head.cSparate = '{';
	head.type = HQTYPE_DYNA;
	head.attrs = 0;
	head.length = sizeof(short);
	
	// data
	unsigned short uMarket = pMarketSerial->MarketCode;
	
	// assign
	memcpy(cSendBuf, &head, sizeof(ACC_JAVA_HEAD));
	memcpy(cSendBuf + sizeof(ACC_JAVA_HEAD), &uMarket, sizeof(short));
	
	// send
	int iRes = Send(m_sockfd, cSendBuf, sizeof(ACC_JAVA_HEAD) + sizeof(short), 5*1000);
	if (0 == iRes)
	{
		CloseConnect();
		return -1;
	}
	else if (-1 == iRes)
	{
		return -2;
	}
	
	// 读取头标识
	markHead tmarkHead = {0};
	int iRet = -1;
	iRet = RecvUnknowLen(m_sockfd, &tmarkHead, sizeof(markHead));
	if (iRet != sizeof(markHead))
	{
		CloseConnect();
		return -3;
	}
	
	// 读取长度
	unsigned int iTotalLen = 0;
	if (0 == tmarkHead.attrs)
	{
		unsigned short uLen = 0;
		iRet = RecvUnknowLen(m_sockfd, &uLen, sizeof(short));
		if (iRet != sizeof(short))
		{
			CloseConnect();
			return -4;
		}
		iTotalLen = uLen;
	}
	else if (0x0008 == tmarkHead.attrs)
	{
		iRet = RecvUnknowLen(m_sockfd, &iTotalLen, sizeof(int));
		if (iRet != sizeof(int))
		{
			CloseConnect();
			return -5;
		}
	}
	
	// 读取数据部分	
	ACC_STK_DYNA *pStart = m_pStkDyna + DataOffset;	
	int iRemainLen = iTotalLen;
	iRet = 0;
	while (iRemainLen > 0)
	{
		iRet = RecvUnknowLen(m_sockfd, (((char*)pStart) + iTotalLen - iRemainLen), iRemainLen);
		if (0 == iRet)
		{
			CloseConnect();
			return -7;
		}
		else if (0 > iRet)
		{
			break;
		}
		
		iRemainLen -= iRet;
	}
	
	return iRemainLen == 0 ? 0 : -8;
}

// 数据初始化
bool 
CQuotesSnapshot::InitialData()
{
	m_bInitialed = false;
	m_nStatus = QS_INITIALIZING;
	m_pCurrentSearchDyna = NULL;
	m_pCurrentCalcDyna = NULL;
	InitialRes();
	
	// 设置市场
	if (!ParseSupportMarket())
	{
		ERROR("ParseSupportMarket failed, can not support ewarning.");
		m_nStatus = QS_ERROR;
		return false;
	}
	
	if (Net_Normal != m_nNetStatus)
	{
		SocketConnect(false);
	}
	if (Net_Normal != m_nNetStatus)
	{
		return false;
	}
	
	int iRes = RequestStaicData();
	if (0 != iRes)
	{
		SocketConnect(true);
		if (Net_Normal != m_nNetStatus)
		{
			return false;
		}
		iRes = RequestStaicData();
	}
	
	if (0 != iRes)
	{
		ERROR("RequestStaicData error [%d]", iRes);
		return false;
	}
	
	ExchageNameToUtf8();
	m_nStatus = QS_NORAML;
	UpdateDynaData();
	m_bInitialed = true;
	return true;
}

// 更新动态数据
bool 
CQuotesSnapshot::UpdateDynaData()
{
	if (!m_bInitialed)
	{
		return false;
	}
	
	int iRes = RequestDynaData();
	if (0 != iRes)
	{
		DEBUG("RequestMarketDynaData error [%d]", iRes);
		m_nStatus = QS_ERROR;
		return false;
	}
	
	CalcRequireData();
	
	return true;
}

// 计算需要的参数
int 
CQuotesSnapshot::CalcRequireData()
{
	if (NULL == m_pCurrentCalcDyna)
	{
		m_pCurrentCalcDyna = m_pPriceAlarmFirst;
	}
	else if (m_pCurrentCalcDyna == m_pPriceAlarmFirst)
	{
		m_pCurrentCalcDyna = m_pPriceAlarmSecond;
	}
	else
	{
		m_pCurrentCalcDyna = m_pPriceAlarmFirst;
	}
	
	// 计算
	int iOffSet = 0;
	BYTE PriceDigit = 2;
	StringMarketIter iterFind = m_mpMarketClassf.begin();
	while(m_mpMarketClassf.end() != iterFind)
	{
		// 上海证券交易所 及 深圳证券交易所特殊处理
		bool bExclude = false;
		if("SH" == iterFind->first || "SZ" == iterFind->first)
		{
			bExclude = true;
		}
		for (int j = 0; j < iterFind->second.MemCount; j++)
		{
			// 代码
			memcpy(m_pCurrentCalcDyna[iOffSet].m_strSymbol, m_pStkStatic[iOffSet].m_szLabel, MAX_STKCODE_LEN);
			
			if (bExclude)
			{
				// 名称
				memcpy(m_pCurrentCalcDyna[iOffSet].m_strName, m_pStkStatic[iOffSet].m_szName, 29);
				// 价格最小分辨率(名称的第29位)
				memcpy(&PriceDigit, &m_pStkStatic[iOffSet].m_szName[29], sizeof(BYTE));
			}
			else
			{
				memcpy(m_pCurrentCalcDyna[iOffSet].m_strName, m_pStkStatic[iOffSet].m_szName, MAX_STKNAME);
				PriceDigit = m_pStkStatic[iOffSet].m_nPriceDigit;
			}
			unsigned char cLen = strlen(m_pCurrentCalcDyna[iOffSet].m_strName);
			if (cLen < MAX_STKNAME)
			{
				m_pCurrentCalcDyna[iOffSet].m_strName[cLen] = '\0';
			}
							
			// 最新价(存在量才定义为有效的价格)
			if (0 >= m_pStkDyna[iOffSet].m_mVolume.GetValue()
				|| 0 >= m_pStkDyna[iOffSet].m_dwHigh
				|| 0 >= m_pStkDyna[iOffSet].m_dwLow
				|| m_pStkDyna[iOffSet].m_dwNew < m_pStkDyna[iOffSet].m_dwLow
				|| m_pStkDyna[iOffSet].m_dwNew > m_pStkDyna[iOffSet].m_dwHigh)
			{
				m_pCurrentCalcDyna[iOffSet].m_Price = 0;
			}
			else
			{
				m_pCurrentCalcDyna[iOffSet].m_Price = m_pStkDyna[iOffSet].m_dwNew;
			}
			
			// 涨跌幅
			if (0 == m_pStkDyna[iOffSet].m_dwNew || 0 == m_pStkStatic[iOffSet].m_dwLastClose)
			{
				m_pCurrentCalcDyna[iOffSet].m_Change = 0;
			}
			else
			{
				float fClose = m_pStkStatic[iOffSet].m_dwLastClose / powf(10, m_pStkStatic[iOffSet].m_nPriceDigit);
				float fLatest = m_pStkDyna[iOffSet].m_dwNew / powf(10, PriceDigit);
				m_pCurrentCalcDyna[iOffSet].m_Change = 
					ChangeToINT((fLatest - fClose) / fClose * 100.0f, PriceDigit);
			}
			
			// 换手率
			if (0 == m_pStkStatic[iOffSet].m_mFloatIssued.GetValue() || 0 == m_pStkDyna[iOffSet].m_mVolume.GetValue())
			{
	            m_pCurrentCalcDyna[iOffSet].m_TurnoverRate = 0;
	        }
	        else
	        {
	            m_pCurrentCalcDyna[iOffSet].m_TurnoverRate = 
	            	ChangeToUINT(m_pStkDyna[iOffSet].m_mVolume.GetValue() * 1.0f / m_pStkStatic[iOffSet].m_mFloatIssued.GetValue() 
	            	* 100.0f, PriceDigit);
			}

			// 成交时间
			m_pCurrentCalcDyna[iOffSet].m_time = m_pStkDyna[iOffSet].m_time;
			
			// 小数位精度
			m_pCurrentCalcDyna[iOffSet].m_cPrecision = PriceDigit;
						
			iOffSet++;
		}
		
		iterFind++;
	}
	
	m_pCurrentSearchDyna = m_pCurrentCalcDyna;
	return 0;
}

// 查找证券在相应市场中的索引
const unsigned short
CQuotesSnapshot::findSecurityIndex(const char *strSymbol)
{
	if (!m_bInitialed || NULL == strSymbol || strlen(strSymbol) < 2)
	{
		return 0xFFFF;
	}
	
	// 市场代码
	char cMarkertTem[MarketCodeLen + 1] = {0};
	strncpy(cMarkertTem, strSymbol, MarketCodeLen);
	char cMarket[MarketCodeLen + 1] = {0};
	ToUppers(cMarket, cMarkertTem);
	
	// 证券代码
	MarketValueMap temItem;
	bzero(&temItem, sizeof(MarketValueMap));
	strncpy(temItem.strCode, strSymbol + 2, 10);
	
	// 市场索引
	MarketClassified *pMarketClassified = FindMarketClassified(cMarket);
	if (NULL == pMarketClassified)
	{
		return 0xFFFF;
	}
	
	int iIndex = FindMarketIndexMapValue(pMarketClassified->MemCount, pMarketClassified->DataOffset,
					&temItem);
	if (iIndex < 0)
	{
		return 0xFFFF;
	}

	return (iIndex - pMarketClassified->DataOffset);
}

// 获取指定的动态信息
PriceAlarm* 
CQuotesSnapshot::GetIndexSecurity(const int StkIndex, const char *strSymbol)
{
	if (!m_bInitialed || NULL == strSymbol || strlen(strSymbol) < 2
		|| NULL == m_pCurrentSearchDyna)
	{
		return NULL;
	}
	
	// 市场代码
	char cMarkertTem[MarketCodeLen + 1] = {0};
	strncpy(cMarkertTem, strSymbol, MarketCodeLen);
	char cMarket[MarketCodeLen + 1] = {0};
	ToUppers(cMarket, cMarkertTem);
	
	MarketClassified *pMarketClassified = FindMarketClassified(cMarket);
	if (NULL == pMarketClassified)
	{
		return NULL;
	}
	
	int iIndex = StkIndex + pMarketClassified->DataOffset;
	return &m_pCurrentSearchDyna[iIndex];
}

// 获取指定的静态信息
ACC_STK_STATIC*
CQuotesSnapshot::GetIndexStatic(const int StkIndex, const char *strSymbol)
{
	if (!m_bInitialed || NULL == strSymbol || strlen(strSymbol) < 2
		|| NULL == m_pStkStatic)
	{
		return NULL;
	}
	
	// 市场代码
	char cMarkertTem[MarketCodeLen + 1] = {0};
	strncpy(cMarkertTem, strSymbol, MarketCodeLen);
	char cMarket[MarketCodeLen + 1] = {0};
	ToUppers(cMarket, cMarkertTem);
	
	MarketClassified *pMarketClassified = FindMarketClassified(cMarket);
	if (NULL == pMarketClassified)
	{
		return NULL;
	}
	
	int iIndex = StkIndex + pMarketClassified->DataOffset;
	return &m_pStkStatic[iIndex];
}

// 将证券名称转换为UTF8
void 
CQuotesSnapshot::ExchageNameToUtf8()
{
	int iOffSet = 0;
	CCodeConverter ConvertUtf8("GBK", "UTF-8");
	StringMarketIter iterFind = m_mpMarketClassf.begin();
	while(m_mpMarketClassf.end() != iterFind)
	{
		// 上海证券交易所 及 深圳证券交易所特殊处理
		bool bExclude = false;
		if("SH" == iterFind->first || "SZ" == iterFind->first)
		{
			bExclude = true;
		}
		
		for (int j = 0; j < iterFind->second.MemCount; j++)
		{
			char cName[128] = {0};
			char strName[56] = {0};
			if (bExclude)
			{
				memcpy(strName, m_pStkStatic[iOffSet].m_szName, 29);
				ConvertUtf8.Convert(strName, 29, cName, 128);
				memcpy(m_pStkStatic[iOffSet].m_szName, cName, 29);
			}
			else
			{
				memcpy(strName, m_pStkStatic[iOffSet].m_szName, MAX_STKNAME);
				ConvertUtf8.Convert(strName, MAX_STKNAME, cName, 128);
				strncpy(m_pStkStatic[iOffSet].m_szName, cName, MAX_STKNAME);
			}
			TRACE("iOffSet=%d name=%s mak=%s", iOffSet, m_pStkStatic[iOffSet].m_szName, iterFind->first.c_str());
			iOffSet++;
		}
		
		iterFind++;
	}
}

// 是否已经初始化
bool
CQuotesSnapshot::HasInitialed()
{
	return m_bInitialed;
}

// 判断静态数据是否有更新
bool
CQuotesSnapshot::HasStaticDateUpdate()
{
	if (Net_Normal != m_nNetStatus)
	{
		SocketConnect(false);
	}
	
	StringMarketIter iterFind = m_mpMarketClassf.begin();
	while(m_mpMarketClassf.end() != iterFind)
	{
		if (Net_Normal != m_nNetStatus)
		{
			return false;
		}
		
		if (HasMarketStaticDataUpdate(&iterFind->second))
		{
			return true;
		}
		iterFind++;
	}
	
	return false;
}

// 判断指定市场的静态数据是否有更新
bool
CQuotesSnapshot::HasMarketStaticDataUpdate(MarketClassified *pMarketSerial)
{
	char cSendBuf[256] = {0};
		
	// main head
	struct ACC_JAVA_HEAD head;
	head.cSparate = '{';
	head.type = HQTYPE_STATIC;
	head.attrs = 0;
	head.length = sizeof(struct ACC_STATICASKHEAD);
	
	// data
	struct ACC_STATICASKHEAD reqHead = {0};
	reqHead.m_nMarket = pMarketSerial->MarketCode;
	reqHead.m_nDate = pMarketSerial->nDate;
	reqHead.m_nCrc = pMarketSerial->nCrc;
	
	// assign
	memcpy(cSendBuf, &head, sizeof(ACC_JAVA_HEAD));
	memcpy(cSendBuf + sizeof(ACC_JAVA_HEAD), &reqHead, sizeof(ACC_STATICASKHEAD));
	
	// send
	int iRes = Send(m_sockfd, cSendBuf, sizeof(ACC_JAVA_HEAD) + sizeof(ACC_STATICASKHEAD), 5*1000);
	if (0 == iRes)
	{
		CloseConnect();
		return false;
	}
	else if (-1 == iRes)
	{
		return false;
	}
	
	// 读取头标识
	markHead tmarkHead = {0};
	int iRet = -1;
	iRet = RecvUnknowLen(m_sockfd, &tmarkHead, sizeof(markHead));
	if (iRet != sizeof(markHead))
	{
		CloseConnect();
		return false;
	}
	
	// 读取长度
	unsigned int iTotalLen = 0;
	if (0 == tmarkHead.attrs)
	{
		unsigned short uLen = 0;
		iRet = RecvUnknowLen(m_sockfd, &uLen, sizeof(short));
		if (iRet != sizeof(short))
		{
			CloseConnect();
			return false;
		}
		iTotalLen = uLen;
	}
	else if (0x0008 == tmarkHead.attrs)
	{
		iRet = RecvUnknowLen(m_sockfd, &iTotalLen, sizeof(int));
		if (iRet != sizeof(int))
		{
			CloseConnect();
			return false;
		}
	}
	
	// 取ACC_STATICASKHEAD
	ACC_STATICASKHEAD askHead = {0};
	iRet = RecvUnknowLen(m_sockfd, &askHead, sizeof(ACC_STATICASKHEAD));
	if (iRet != sizeof(ACC_STATICASKHEAD))
	{
		CloseConnect();
		return false;
	}
	iTotalLen -= sizeof(ACC_STATICASKHEAD);
	
	if (pMarketSerial->nDate == askHead.m_nDate
		&& pMarketSerial->nCrc == askHead.m_nCrc)
	{
		return false;
	}
	
	if (iTotalLen <= 0)
	{
		return false;
	}
	
	// 读取数据部分	
	char pStart[512] = {0};	
	int iRemainLen = iTotalLen;
	iRet = 0;
	while (iRemainLen > 0)
	{
		iRet = RecvUnknowLen(m_sockfd, pStart, 512);
		if (0 == iRet)
		{
			CloseConnect();
			break;
		}
		else if (0 > iRet)
		{
			break;
		}
		
		iRemainLen -= iRet;
	}

	return true;
}

// 添加市场值映射
void
CQuotesSnapshot::AddMarkMapValue(const int index, const struct MarketValueMap *Item)
{
	if (NULL == Item || index > GetMaxRecordNum()
		|| NULL == m_pMarketValueMap)
	{
		return;
	}
	
	m_pMarketValueMap[index] = *Item;
}

int
NodeCmp(const void *node1, const void *node2)
{
	if (NULL == node1 || NULL == node2)
	{
		return -1;
	}
	
	return strcoll(((const struct MarketValueMap *)node1)->strCode,
      ((const struct MarketValueMap *)node2)->strCode);
}

// 排序市场值映射
void
CQuotesSnapshot::SortMarketIndexMapValue(const int ItemCount, const int OffSet)
{
	if (NULL == m_pMarketValueMap)
	{
		return;
	}
	qsort(m_pMarketValueMap + OffSet, ItemCount, sizeof(MarketValueMap), NodeCmp);
}

// 查找市场值映射
int
CQuotesSnapshot::FindMarketIndexMapValue(const int ItemCount,				
			const int OffSet, const struct MarketValueMap *itemFind)
{
	if (NULL == m_pMarketValueMap || NULL == itemFind)
	{
		return -1;
	}
	
	MarketValueMap *pFind = (MarketValueMap*)bsearch(itemFind, m_pMarketValueMap + OffSet, ItemCount, sizeof(MarketValueMap), NodeCmp);
	if (NULL == pFind)
	{
		return -2;
	}
	return (int)pFind->RecordIndex;
}

// 转换市场代码
int
CQuotesSnapshot::ExchangeMarketCode(const char *MarketSimCode)
{
	if (NULL == MarketSimCode || (size_t)MarketCodeLen != strlen(MarketSimCode))
	{
		return 0x4853;
	}
	
	char cTemp[16] = {0};
	snprintf(cTemp, 16, "%X%X", MarketSimCode[1], MarketSimCode[0]);

	int code = 0;
	sscanf(cTemp, "%X", &code);
	return code;
}

// 解析市场代码
char*
CQuotesSnapshot::ParseMarketCode(const int MarketCode)
{
	static char cCode[8] = {0};
	memset(cCode, 0, 8);
	snprintf(cCode, 8, "%c%c", MarketCode&0xFF, (MarketCode&0xFF00)>>8);
	return cCode;
}

// 解析支持的市场
bool
CQuotesSnapshot::ParseSupportMarket()
{
	m_mpMarketClassf.clear();
	const struct DataServerTag *pServerTag = GetDataServer(false);
	if (NULL == pServerTag)
	{
		return false;
	}
	
	char *pCodeInclude = (char*)pServerTag->MarketInclude;
	int iMarketIncludeLen = strlen(pServerTag->MarketInclude);
	if (iMarketIncludeLen <= 0)
	{
		return false;
	}
	
	char cMarketCode[MarketCodeLen + 1] = {0};
	while (iMarketIncludeLen > 0)
	{
		memset(cMarketCode, 0, MarketCodeLen);
		strncpy(cMarketCode, pCodeInclude, MarketCodeLen);
		m_mpMarketClassf.insert(StringMarketPair(cMarketCode, MarketClassified(ExchangeMarketCode(cMarketCode))));
		pCodeInclude += MarketCodeLen;
		iMarketIncludeLen -= MarketCodeLen;
	}
	
	return true;
}

// 查找市场分类
MarketClassified*
CQuotesSnapshot::FindMarketClassified(const char *MarketCode)
{
	StringMarketIter iterFind = m_mpMarketClassf.find(MarketCode);
	if (m_mpMarketClassf.end() == iterFind)
	{
		return NULL;
	}
	return &iterFind->second;
}

