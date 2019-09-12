1、下载WDK并安装  https://www.microsoft.com/en-us/download/details.aspx?id=11800
2、
把C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\include中的所有文件复制到C:\WinDDK\7600.16385.1\inc\api中，相同的文件覆盖
3、
在vs2010中添加
属性-》c/c++->附加包含目录：C:\WinDDK\7600.16385.1\inc\api
属性-》链接器->附加库目录：C:\WinDDK\7600.16385.1\lib\win7\i386   （不知道为什么要添加i386  32位的，其实我的win7系统为64位）

4、用法
#include <Dbt.h>
#include <SetupAPI.h>
extern "C"
{
#include <hidsdi.h>
}



#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")