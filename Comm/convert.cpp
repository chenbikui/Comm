static void convert_string_to_upper_case(char *ptr)
{
	for (; *ptr; ptr++)
	{
		if (*ptr >= 'a' && *ptr <= 'z')
		{
			*ptr -= 'a' - 'A';
		}
	}
}

static void convert_upper_case(stParam *pParam)
{
	int i;
	
	for (i = 0; i < pParam->argc; i++)
	{
		convert_string_to_upper_case(pParam->argv[i]);
	}
}


#define giz_getbit(value,n)    ((value>>n) & 1)

void CAbcDlg::OnButton8() 
{
/*	test_t *p = new test_t;

	p->a = 100000;
	memcpy(p->buf,"123", 3);
	p->b=101;
	p->c=sizeof(test_t);

	char *pp = (char *)p;

	test_t *pc = (test_t *)pp;

	delete p;
	p = NULL;*/

	BYTE value = 0x2;
	BYTE a = giz_getbit(value, 0);
}

typedef struct _LOCAL_TIMER_T
{
	uint8_t index;//定时器索引
	//actions
	uint8_t powerstate; //定时器要执行的动作根据后台的设置来定义
	uint8_t fog;
	uint8_t repeat;//是否重复
	//time
	uint32_t utc;
	uint8_t tm_wday;//bit0为1时周一有效，bit6代表周日.......
	
	//char timer[32];
	//uint8_t enable;
	uint8_t valid;//0:无效  1:有效
}local_timer_t;


typedef struct _localtimer_info_t
{
	local_timer_t localtimer[5]; //定时器指针
	int nCount;//有多少个定时器
}localtimer_info;


#define LEAPOCH (946684800L + 86400*(31+29))

#define DAYS_PER_400Y (365*400 + 97)
#define DAYS_PER_100Y (365*100 + 24)
#define DAYS_PER_4Y   (365*4 + 1)

#define TIME_ZONE	8	/*时区,默认为中国东8区*/

int ntp_secs_to_tm(LONG64 t, struct tm *tm)
{
	LONG64 days, secs;
	int remdays, remsecs, remyears;
	int qc_cycles, c_cycles, q_cycles;
	int years, months;
	int wday, yday, leap;
	static const char days_in_month[] = {31,30,31,30,31,31,30,31,30,31,31,29};
	
	/* Reject time_t values whose year would overflow int */
	//if ((t < INT_MIN * 31622400) || (t > INT_MAX * 31622400L))
//		return -1;
	
	secs = t - LEAPOCH;
	days = secs / 86400;
	remsecs = secs % 86400;
	if (remsecs < 0) {
		remsecs += 86400;
		days--;
	}
	
	wday = (3+days)%7;
	if (wday < 0) wday += 7;
	
	qc_cycles = days / DAYS_PER_400Y;
	remdays = days % DAYS_PER_400Y;
	if (remdays < 0) {
		remdays += DAYS_PER_400Y;
		qc_cycles--;
	}
	
	c_cycles = remdays / DAYS_PER_100Y;
	if (c_cycles == 4) c_cycles--;
	remdays -= c_cycles * DAYS_PER_100Y;
	
	q_cycles = remdays / DAYS_PER_4Y;
	if (q_cycles == 25) q_cycles--;
	remdays -= q_cycles * DAYS_PER_4Y;
	
	remyears = remdays / 365;
	if (remyears == 4) remyears--;
	remdays -= remyears * 365;
	
	leap = !remyears && (q_cycles || !c_cycles);
	yday = remdays + 31 + 28 + leap;
	if (yday >= 365+leap) yday -= 365+leap;
	
	years = remyears + 4*q_cycles + 100*c_cycles + 400*qc_cycles;
	
	for (months=0; days_in_month[months] <= remdays; months++)
		remdays -= days_in_month[months];
	
	if (years+100 > INT_MAX || years+100 < INT_MIN)
		return -1;
	
	tm->tm_year = years + 100;
	tm->tm_mon = months + 2;
	if (tm->tm_mon >= 12) {
		tm->tm_mon -=12;
		tm->tm_year++;
	}
	tm->tm_mday = remdays + 1;
	tm->tm_wday = wday;
	tm->tm_yday = yday;
	
	tm->tm_hour = remsecs / 3600;
	tm->tm_min = remsecs / 60 % 60;
	tm->tm_sec = remsecs % 60;
	
	/*转化成本地的*/
	tm->tm_year += 1900;
	tm->tm_mon += 1;
	//tm->tm_hour += TIME_ZONE;	/*0时区*/
	
	printf("###local time y/m/d h/m/s: %d/%d/%d %d/%d/%d  %d\n", tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_wday);
	
	return 0;
}

void CAbcDlg::OnButton9() 
{
}

typedef  LONG64 time64_t;

static const unsigned char rtc_days_in_month[] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static const unsigned short rtc_ydays[2][13] = {
	/* Normal years */
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
	/* Leap years */
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

#define LEAPS_THRU_END_OF(y) ((y)/4 - (y)/100 + (y)/400)

static inline bool is_leap_year(unsigned int year)
{
	return (!(year % 4) && (year % 100)) || !(year % 400);
}

static inline LONG64 div_s64_rem(LONG64 dividend, unsigned int divisor, unsigned int *remainder)
{
	*remainder = dividend % divisor;
	return dividend / divisor;
}

/*
 * The number of days in the month.
 */
int rtc_month_days(unsigned int month, unsigned int year)
{
	return rtc_days_in_month[month] + (is_leap_year(year) && month == 1);
}


/*
 * The number of days since January 1. (0 to 365)
 */
int rtc_year_days(unsigned int day, unsigned int month, unsigned int year)
{
	return rtc_ydays[is_leap_year(year)][month] + day-1;
}


/*
 * rtc_time64_to_tm - Converts time64_t to rtc_time.
 * Convert seconds since 01-01-1970 00:00:00 to Gregorian date.
 */
void rtc_time64_to_tm(time64_t time, struct tm *tm)
{
	unsigned int month, year, secs;
	int days;

	/* time must be positive */
	days = div_s64_rem(time, 86400, &secs);

	/* day of the week, 1970-01-01 was a Thursday */
	tm->tm_wday = (days + 4) % 7;

	year = 1970 + days / 365;
	days -= (year - 1970) * 365
		+ LEAPS_THRU_END_OF(year - 1)
		- LEAPS_THRU_END_OF(1970 - 1);
	while (days < 0) {
		year -= 1;
		days += 365 + is_leap_year(year);
	}
	tm->tm_year = year - 1900;
	tm->tm_yday = days + 1;

	for (month = 0; month < 11; month++) {
		int newdays;

		newdays = days - rtc_month_days(month, year);
		if (newdays < 0)
			break;
		days = newdays;
	}
	tm->tm_mon = month;
	tm->tm_mday = days + 1;

	tm->tm_hour = secs / 3600;
	secs -= tm->tm_hour * 3600;
	tm->tm_min = secs / 60;
	tm->tm_sec = secs - tm->tm_min * 60;

	tm->tm_isdst = 0;
}



time64_t mktime64(const unsigned int year0, const unsigned int mon0,
				  const unsigned int day, const unsigned int hour,
				  const unsigned int min, const unsigned int sec)
{
	unsigned int mon = mon0, year = year0;
	
	/* 1..12 -> 11,12,1..10 */
	if (0 >= (int) (mon -= 2)) {
		mon += 12;	/* Puts Feb last since it has leap day */
		year -= 1;
	}
	
	return ((((time64_t)
		(year/4 - year/100 + year/400 + 367*mon/12 + day) +
		year*365 - 719499
		)*24 + hour /* now have hours - midnight tomorrow handled here */
		)*60 + min /* now have minutes */
		)*60 + sec; /* finally seconds */
}

time64_t rtc_tm_to_time64(struct tm *tm)
{
	return mktime64(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void CAbcDlg::OnButton10() 
{
	struct tm t;
	time64_t ntime = 0;

	//1552478398    2019年 03月 13日 星期三 19:59:58 CST
	//rtc_time64_to_tm(1552478398, &t);
	//1552529515   2019/3/14 10:11:55
	rtc_time64_to_tm(1552529515, &t);

	ntime = rtc_tm_to_time64(&t);

	CString str;
	str.Format("time:%I64d\n%04d-%02d-%02d %02d:%02d:%02d, 星期:%d,一年第几天：%d\n",ntime,
		t.tm_year+1900,
		t.tm_mon+1,
		t.tm_mday,
		t.tm_hour+8,
		t.tm_min,
		t.tm_sec,
		t.tm_wday,
		t.tm_yday);//t.tm_hour+8是时区



	AfxMessageBox(str);
}


#define  GetBit(x,pos)   ((x>>pos) & 1)
#define  SetBit(x,pos)   ((1<<pos) | x)



bool doublechar_to_hex(unsigned char *ret, char *str, int len)
{
    char *p = str;
    unsigned int tmp = 0;
    if ((*p >= 'A') && (*p <= 'F')) {
        tmp = *p - 'A' + 10;
    } else if ((*p >= 'a') && (*p <= 'f')) {
        tmp = *p - 'a' + 10;
    } else if ((*p >= '0') && (*p <= '9')) {
        tmp = *p - '0';
    } else {
        printf("Error: not hex\n");
        return false;
    }
    //printf("tmp = %x\n", tmp);
    tmp *= 16;
	
    p++;
    if ((*p >= 'A') && (*p <= 'F')) {
        tmp += *p - 'A' + 10;
    } else if ((*p >= 'a') && (*p <= 'f')) {
        tmp += *p - 'a' + 10;
    } else if ((*p >= '0') && (*p <= '9')) {
        tmp += *p - '0';
    } else {
        printf("Error: not hex\n");
        return false;
    }
    //printf("tmp = %x\n", tmp);
    *ret = tmp;
    return true;
}

bool mac_str_to_hex(unsigned char *des, char *str)
{
    int i = 0;
    char *s = str;
    unsigned char *d = des;
    bool ret = false;
	
    for (i = 0; i < 6; i++) {
        ret = doublechar_to_hex(d, s, 2);
        if (ret < 0) {
            return ret;
        }
        d++;
        s += 2;
    }
    for (i = 0; i< 6; i++) {
        printf("des[%d] = 0x%02x\n", i, des[i]);
    }
	
    return ret;
}



void hextostr11(unsigned char *buf, int len, char *out)
{
	unsigned char i = 0,j=0;
	unsigned char temp = 0;

	for (i = 0; i < len; i++)
	{
		for (j=0;j<2;j++)
		{
			if (j == 0)
			{
				temp = buf[i] / 16;
			}
			else
			{
				temp = buf[i] % 16;
			}
			if (temp >= 0xa && temp <= 0xf)
			{
				out[i*2+j]=temp+'a'-10;
			}
			else if(temp>=0xA && temp <= 0xF)
			{
				out[i*2+j] = temp + 'A' - 10;
			}
			else if (temp >= 0 && temp <= 9)
			{
				out[i*2+j] = temp + '0' ;
			}
		}
		
	}
}

char *hex2Str( uint8_t data)
{
	
	char hex[] = "0123456789ABCDEF";
	static char str[3];
	char *pStr = str;
	
	*pStr++ = hex[data >> 4];
	*pStr++ = hex[data & 0x0F];
	*pStr = 0;
	
	return str;
}

char *hex2Str_ex( uint8_t data)
{

	char hex[] = "0123456789abcdef";
	static char str[3] = {0};
	char *pStr = str;

	*pStr++ = hex[data >> 4];
	*pStr++ = hex[data & 0x0F];
	*pStr = 0;

	return str;
}

void HexToStr(uint8_t *buf, uint8_t len, char *out)
{
	uint8_t i = 0,j = 0;
	uint8_t temp = 0;
	char *p = NULL;

	for (i = 0; i < len; i++)
	{
		p = hex2Str_ex(buf[i]);
		memcpy(out + (i * 2), p, 2);
	}
}

/*返回0 失败，1成功*/
int str2hex(char *pstr, BYTE *out_byte)
{
	char temp;
	uint8_t i = 0, j = 1;
	uint8_t v = 0;

	if (strlen(pstr) < 2)
	{
		return 0;
	}

	for (i = 0; i < 2; i++)
	{
		temp = *(pstr + i);
		if ((temp >= '0') && (temp <= '9'))
		{
			v += ((temp - '0') << (j * 4));
		}
		else if ((temp >= 'a') && (temp <= 'f'))
		{
			v += ((temp - 'a' + 10) << (j * 4));
		}
		else if ((temp >= 'A') && (temp <= 'F'))
		{
			v += ((temp - 'A' + 10) << (j * 4));
		}
		else
		{
			return 0;
		}

		j--;
	}

	*out_byte = v;

	return 1;
	
}

/*返回转换成功的字节个数*/
int str2hex_ex(char *pstr, unsigned char *out, int out_len)
{
	int i = 0;
	int str_len = strlen(pstr);
	int count = str_len/2;
	unsigned char v = 0;
	
	if (((str_len % 2) != 0) || (count > out_len))
	{
		return 0;
	}

	for (i = 0; i < count; i++)
	{
		if (1 != str2hex(pstr + (i * 2), &v))
		{
			return i;
		}
		out[i] = v;
	}

	return count;
}


//字符串截取
char *myStrncpy(char *dest, const char *src, int n)
{
	int size = sizeof(char)*(n + 1);
	char *tmp = (char*)malloc(size); // 开辟大小为n+1的临时内存tmp
		
	if (tmp)
	{
		memset(tmp, '\0', size); // 将内存初始化为0
		memcpy(tmp, src, size - 1); // 将src的前n个字节拷贝到tmp
		memcpy(dest, tmp, size); // 将临时空间tmp的内容拷贝到dest
			
		free(tmp); // 释放内存
		return dest;
	}
	else
	{
		return NULL;
	}
}

//得到软件编译时间?
void GetSoftWareBuildTargetTime(RTC_TIME_DEF *ptime)
{
	char arrDate[20]; //Jul 03 2018
	char arrTime[20]; //06:17:05
	char pDest[20];
//	RTC_TIME_DEF stTime;
	
	sprintf(arrDate,"%s",__DATE__);//Jul 03 2018
	sprintf(arrTime,"%s",__TIME__);//06:17:05
	sprintf(pDest, "%s", myStrncpy(pDest, arrDate, 3));
	
	if (strcmp(pDest, "Jan") == 0) ptime->nMonth = 1;
	else if (strcmp(pDest, "Feb") == 0) ptime->nMonth = 2;
	else if (strcmp(pDest, "Mar") == 0) ptime->nMonth = 3;
	else if (strcmp(pDest, "Apr") == 0) ptime->nMonth = 4;
	else if (strcmp(pDest, "May") == 0) ptime->nMonth = 5;
	else if (strcmp(pDest, "Jun") == 0) ptime->nMonth = 6;
	else if (strcmp(pDest, "Jul") == 0) ptime->nMonth = 7;
	else if (strcmp(pDest, "Aug") == 0) ptime->nMonth = 8;
	else if (strcmp(pDest, "Sep") == 0) ptime->nMonth = 9;
	else if (strcmp(pDest, "Oct") == 0) ptime->nMonth = 10;
	else if (strcmp(pDest, "Nov") == 0) ptime->nMonth = 11;
	else if (strcmp(pDest, "Dec") == 0) ptime->nMonth = 12;
	else ptime->nMonth = 1;
	
	sprintf(pDest, "%s", myStrncpy(pDest, arrDate+4, 2));

	ptime->nDay = atoi(pDest);
	
	sprintf(pDest, "%s", myStrncpy(pDest, arrDate + 4 + 3, 4));

	ptime->nYear = atoi(pDest);
	

	sprintf(pDest, "%s", myStrncpy(pDest, arrTime, 2));
	ptime->nHour = atoi(pDest);
	sprintf(pDest, "%s", myStrncpy(pDest, arrTime+3, 2));
	ptime->nMinute = atoi(pDest);
	sprintf(pDest, "%s", myStrncpy(pDest, arrTime + 3 + 3, 2));
	ptime->nSecond = atoi(pDest);
	
	return ;
}


#define BURSIZE 2048
int hex2dec(char c)
{
    if ('0' <= c && c <= '9')
    {
        return c - '0';
    }
    else if ('a' <= c && c <= 'f')
    {
        return c - 'a' + 10;
    }
    else if ('A' <= c && c <= 'F')
    {
        return c - 'A' + 10;
    }
    else
    {
        return -1;
    }
}

char dec2hex(short int c)
{
    if (0 <= c && c <= 9)
    {
        return c + '0';
    }
    else if (10 <= c && c <= 15)
    {
        return c + 'A' - 10;
    }
    else
    {
        return -1;
    }
}


//编码一个url 
void urlencode(char url[])
{
    int i = 0;
    int len = strlen(url);
    int res_len = 0;
    char res[BURSIZE];
    for (i = 0; i < len; ++i)
    {
        char c = url[i];
        if (    ('0' <= c && c <= '9') ||
			('a' <= c && c <= 'z') ||
			('A' <= c && c <= 'Z') ||
			c == '/' || c == '.')
        {
            res[res_len++] = c;
        }
        else
        {
            int j = (short int)c;
            if (j < 0)
                j += 256;
            int i1, i0;
            i1 = j / 16;
            i0 = j - i1 * 16;
            res[res_len++] = '%';
            res[res_len++] = dec2hex(i1);
            res[res_len++] = dec2hex(i0);
        }
    }
    res[res_len] = '\0';
    strcpy(url, res);
}

// 解码url
void urldecode(char url[])
{
    int i = 0;
    int len = strlen(url);
    int res_len = 0;
    char res[BURSIZE];
    for (i = 0; i < len; ++i)
    {
        char c = url[i];
        if (c != '%')
        {
            res[res_len++] = c;
        }
        else
        {
            char c1 = url[++i];
            char c0 = url[++i];
            int num = 0;
            num = hex2dec(c1) * 16 + hex2dec(c0);
            res[res_len++] = num;
        }
    }
    res[res_len] = '\0';
    strcpy(url, res);
}

int http_get_rsp_status_code(U8 *rsp_head)
{
	U8 *begin, *end;
	U8 szCode[16] = {0};
	
	if (!rsp_head)
	{
		return -1;
	}
	
	if ((begin = strstr(rsp_head, "HTTP/1.1")) != NULL)
	{
		if ((begin = strstr(begin, " ")) != NULL)
		{
			begin++;
			if ((end = strstr(begin, " ")) != NULL)
			{
				memcpy(szCode, begin, end - begin);
				return atoi(szCode);
			}
		}
	}
	
	return -1;
}

static int http_get_rsp_content_len(char *rsp_head)
{
	char *begin, *end;
	char szCode[32] = {0};
	
	if (!rsp_head)
	{
		return -1;
	}

	if ((begin = strstr(rsp_head, "Content-Length:")) != NULL)
	{
		begin += strlen("Content-Length:");
		if ((end = strstr(begin, "\r\n")) != NULL)
		{
			memcpy(szCode, begin, end - begin);
			return atoi(szCode);
		}
	}

	return -1;
}

/*****************************************************************************
*功能:获取http response中的content data内容
*参数:
	pData -- response全部内容
	len -- pData的长度
	content_len--response中content data的长度
	res -- 返回的content data
	res_len -- res buff的长度
*返回值:
	0 -- 成功
	-1 -- 失败
*其他说明
author by chenbikui 20190702
*****************************************************************************/
static int http_get_rsp_content_data(char *pData, int len, int content_len, void* res, int res_len)
{
	char *begin = NULL;
	
	if (!pData)
	{
		return -1;
	}

	if ((begin = strstr(pData, "\r\n\r\n")) != NULL)
	{
		begin += strlen("\r\n\r\n");
		if (content_len < res_len)
		{
			memcpy(res , begin, content_len);
			memset((char*)res + content_len, 0, res_len - content_len);
			return 0;
		}
		else
		{
			printf("error:res_len is small %s < %d\n", content_len, res_len);
			return -1;
		}
	}

	return -1;
}


int parse_https_content_data(char *pData, int len, void* res, int res_len)
{
	int content_len = 0;

	content_len = http_get_rsp_content_len1(pData);
	printf("content_len:%d\n", content_len);
	if (content_len < 1)
	{
		printf("warnning: content_len = %d\n", content_len);
		return -1;
	}

	if (0 != http_get_rsp_content_data1(pData, len, content_len, res, res_len))
	{
		return -1;
	}
	
	return 0;
}



void CAbcDlg::OnButtonHttp() 
{
	char *pContent = "HTTP/1.1 200 OK\r\nServer: nginx \
		\r\nDate: Tue, 25 Sep 2018 10:11:00 GMT \
		\r\nContent-Type: application/octet-stream \
		\r\nContent-Length: 1031456 \
		\r\nLast-Modified: Mon, 23 Apr 2018 07:51:39 GMT \
		\r\nConnection: keep-alive \
		\r\nETag: \"5add908b-fbd20\" \
		\r\nAccept-Ranges: bytes \
		\r\n\r\nPK";
	int ret = 0;
	ret = http_get_rsp_status_code(pContent);
	ret = http_get_rsp_content_len(pContent);
}
