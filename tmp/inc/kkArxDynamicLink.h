#pragma once

#ifdef KK_OBJECTARX
#define KK_OBJECTARX_API __declspec(dllexport)
#else
#define KK_OBJECTARX_API __declspec(dllimport)
#endif

#include <kkArxUtility.h>

#ifndef KK_OBJECTARX
// �൱����vs��������� ����������
#pragma comment(lib, "ArxApi.lib")
#endif