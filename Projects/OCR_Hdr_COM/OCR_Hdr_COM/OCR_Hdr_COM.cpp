// OCR_Hdr_COM.cpp: WinMain 的实现


#include "stdafx.h"
#include "resource.h"
#include "OCRHdrCOM_i.h"


using namespace ATL;

#include <stdio.h>

class COCRHdrCOMModule : public ATL::CAtlServiceModuleT< COCRHdrCOMModule, IDS_SERVICENAME >
{
public :
	DECLARE_LIBID(LIBID_OCRHdrCOMLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_OCRHDRCOM, "{8f3f8985-2af5-4d53-ae31-d77577d5c231}")
	HRESULT InitializeSecurity() throw()
	{
		// TODO : 调用 CoInitializeSecurity 并为服务提供适当的安全设置
		// 建议 - PKT 级别的身份验证、
		// RPC_C_IMP_LEVEL_IDENTIFY 的模拟级别
		//以及适当的非 NULL 安全描述符。

		return S_OK;
	}
};

COCRHdrCOMModule _AtlModule;



//
extern "C" int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/,
								LPTSTR /*lpCmdLine*/, int nShowCmd)
{
	return _AtlModule.WinMain(nShowCmd);
}

