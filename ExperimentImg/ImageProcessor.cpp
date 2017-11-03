#include "stdafx.h"
#include "ImageProcessor.h"
#include <vector>
#include <algorithm>

static bool GetValue(int p[], int size, int &value)
{
	//数组中间的值
	int zxy = p[(size - 1) / 2];
	//用于记录原数组的下标
	int *a = new int[size];
	int index = 0;
	for (int i = 0; i<size; ++i)
		a[index++] = i;

	for (int i = 0; i<size - 1; i++)
		for (int j = i + 1; j<size; j++)
			if (p[i]>p[j]) {
				int tempA = a[i];
				a[i] = a[j];
				a[j] = tempA;
				int temp = p[i];
				p[i] = p[j];
				p[j] = temp;

			}
	int zmax = p[size - 1];
	int zmin = p[0];
	int zmed = p[(size - 1) / 2];

	if (zmax>zmed&&zmin<zmed) {
		if (zxy>zmin&&zxy<zmax)
			value = (size - 1) / 2;
		else
			value = a[(size - 1) / 2];
		delete[]a;
		return true;
	}
	else {
		delete[]a;
		return false;
	}

}

void ImageProcessor::ImageCopy(CImage * pImgSrc, CImage * pImgDest)
{
	int srcWidth = pImgSrc->GetWidth();
	int srcHeight = pImgSrc->GetHeight();
	int destWidth = pImgDest->GetWidth();
	int destHeight = pImgDest->GetHeight();

	//void* pDestImage = pImgDest->GetPixelAddress(0, 0);

	if (srcWidth == 0 || srcHeight == 0)
	{
		AfxMessageBox(_T("ImageCopy:图片复制失败，源图片width/height为0"));
		return;
	}

	if (destWidth == 0 || destHeight == 0)
	{
		AfxMessageBox(_T("ImageCopy:图片复制失败，目标图片width/height为0"));
		return;
	}

#pragma omp parallel for num_threads(mThreadNum)
	for (int j = 0; j < destHeight; ++j)
	{
		for (int i = 0; i < destWidth; ++i)
		{
			float normalizedX = float(i) / destWidth;
			float normalizedY = float(j) / destHeight;
			//0.999 等下向下取整求单点插值像素坐标时不会越界
			if (normalizedX >= 1.0f)normalizedX = 0.99999f;
			if (normalizedY >= 1.0f)normalizedY = 0.99999f;

			int sampleCoordX = int(srcWidth * normalizedX);
			int sampleCoordY = int(srcHeight * normalizedY);
			COLORREF sampleColor = pImgSrc->GetPixel(sampleCoordX, sampleCoordY);
			pImgDest->SetPixel(i, j, sampleColor);
		}
	}
}

UINT ImageProcessor::MedianFilter_WIN(CImage* pImgSrc, CImage* pImgDest, int numThreads)
{
	int subLength = pImgSrc->GetWidth() * pImgSrc->GetHeight() / numThreads;

	ThreadParam* pParamArray = new ThreadParam[cMaxThreadNum];
	for (int i = 0; i < numThreads; ++i)
	{
		pParamArray[i].startIndex = i * subLength;
		pParamArray[i].endIndex = i != numThreads - 1 ?
			(i + 1) * subLength - 1 : pImgSrc->GetWidth() * pImgSrc->GetHeight() - 1;
		pParamArray[i].maxSpan = cMedianFilterMaxSpan;
		pParamArray[i].pSrc = pImgSrc;
		pParamArray[i].pDest = pImgDest;

		//new thread
		AfxBeginThread((AFX_THREADPROC)&ImageProcessor::mFunction_MedianFilterForTargetRegion, &pParamArray[i]);
	}

	return 0;
}

UINT ImageProcessor::MedianFilter_OpenMP(CImage* pImgSrc, CImage* pImgDest, int numThreads)
{
	int subLength = pImgSrc->GetWidth() * pImgSrc->GetHeight() / numThreads;

	ThreadParam* pParamArray = new ThreadParam[cMaxThreadNum];
	//OpenMP 并不是一个简单的函数库，而是被众多【编译器】所支持的并行计算
	//框架/协议，只需要在一个for前面加上#pragma omp parallel xxxxx 就可以
	//很舒服地把并行化的工作扔给编译器做，简直6666666666666！！！
#pragma omp parallel for num_threads(mThreadNum)
	for (int i = 0; i < numThreads; ++i)
	{
		pParamArray[i].startIndex = i * subLength;
		pParamArray[i].endIndex = i != numThreads - 1 ?
			(i + 1) * subLength - 1 : pImgSrc->GetWidth() * pImgSrc->GetHeight() - 1;
		pParamArray[i].maxSpan = cMedianFilterMaxSpan;
		pParamArray[i].pSrc = pImgSrc;
		pParamArray[i].pDest = pImgDest;
		ImageProcessor::mFunction_MedianFilterForTargetRegion(&pParamArray[i]);
	}
	return 0;
}

UINT ImageProcessor::AddNoise_WIN(CImage* pImgSrc, CImage* pImgDest, int numThreads)
{
	int subLength = pImgSrc->GetWidth() * pImgSrc->GetHeight() / numThreads;
	ThreadParam* pParamArray = new ThreadParam[cMaxThreadNum];
	for (int i = 0; i < numThreads; ++i)
	{
		pParamArray[i].startIndex = i * subLength;
		pParamArray[i].endIndex = i != numThreads - 1 ?
			(i + 1) * subLength - 1 : pImgSrc->GetWidth() * pImgSrc->GetHeight() - 1;
		pParamArray[i].pSrc = pImgSrc;
		pParamArray[i].pDest = pImgDest;

		//new thread
		AfxBeginThread((AFX_THREADPROC)&ImageProcessor::mFunction_AddNoiseForTargetRegion, &pParamArray[i]);
	}
	return 0;
}

UINT ImageProcessor::AddNoise_OpenMP(CImage* pImgSrc, CImage* pImgDest, int numThreads)
{
	int subLength = pImgSrc->GetWidth() * pImgSrc->GetHeight() / numThreads;
	ThreadParam* pParamArray = new ThreadParam[cMaxThreadNum];
	//OpenMP 并不是一个简单的函数库，而是被众多【编译器】所支持的并行计算
	//框架/协议，只需要在一个for前面加上#pragma omp parallel xxxxx 就可以
	//很舒服地把并行化的工作扔给编译器做，简直6666666666666！！！
#pragma omp parallel for num_threads(mThreadNum)
	for (int i = 0; i < numThreads; ++i)
	{
		pParamArray[i].startIndex = i * subLength;
		pParamArray[i].endIndex = i != numThreads - 1 ?
			(i + 1) * subLength - 1 : pImgSrc->GetWidth() * pImgSrc->GetHeight() - 1;
		pParamArray[i].pSrc = pImgSrc;
		pParamArray[i].pDest = pImgDest;
		ImageProcessor::mFunction_AddNoiseForTargetRegion(&pParamArray[i]);
	}
	return 0;
}

UINT ImageProcessor::Rotate_WIN(CImage * pImgSrc, CImage * pImgDest, int numThreads, float radianAngle)
{


	return 0;
}

UINT ImageProcessor::Rotate_OpenMP(CImage * pImgSrc, CImage * pImgDest, int numThreads, float radianAngle)
{
	return 0;
}

/**************************************

						PRIVATE

***************************************/

//template <int bytesPerPixel>
inline void ImageProcessor::mFunction_SetPixel(CImage* pImage, int x, int y, COLOR3 c)
{
	//int bytesPerPixel = pImage->GetBPP() / 8;
	const int bytesPerPixel = 3;
	int pitch = pImage->GetPitch();
	byte* pData = (byte*)pImage->GetBits();

	//编译期的模版函数优化
	/*if (bytesPerPixel == 1)
	{
		//灰度
		*(pImage + pitch * y + x * bytesPerPixel) = c.r;
		*(pImage + pitch * y + x * bytesPerPixel + 1) = c.r;
		*(pImage + pitch * y + x * bytesPerPixel + 2) = c.r;
	}
	else*/

	*(pData + pitch * y + x * bytesPerPixel+ 2) = c.r;
	*(pData + pitch * y + x * bytesPerPixel+ 1) = c.g;
	*(pData + pitch * y + x * bytesPerPixel+ 0) = c.b;

}

//template <int bytesPerPixel>
inline COLOR3 ImageProcessor::mFunction_GetPixel(CImage * pImage, int x, int y)
{
	//int bytesPerPixel = pImage->GetBPP() / 8;
	const int bytesPerPixel = 3;
	int pitch = pImage->GetPitch();
	byte* pData = (byte*)pImage->GetBits();

	//编译期的模版函数优化
	/*if (bytesPerPixel == 1)
	{
		byte r = *(pSrcRealData + pitch*y+x*bytesPerPixel + 2);
		return COLOR3(r, r, r);
	}
	else*/

	byte r = *(pData + pitch*y + x*bytesPerPixel + 2);
	byte g = *(pData + pitch*y + x*bytesPerPixel + 1);
	byte b = *(pData + pitch*y + x*bytesPerPixel + 0);
	return COLOR3(r, g, b);
}


UINT ImageProcessor::mFunction_MedianFilterForTargetRegion(LPVOID  p)
{
	ThreadParam* param = (ThreadParam*)p;

	int maxWidth = param->pSrc->GetWidth();
	int maxHeight = param->pSrc->GetHeight();
	int startIndex = param->startIndex;
	int endIndex = param->endIndex;
	int maxSpan = param->maxSpan;
	int maxLength = (maxSpan * 2 + 1) * (maxSpan * 2 + 1);

	byte* pSrcRealData = (byte*)param->pSrc->GetBits();
	byte* pDestRealData = (byte*)param->pDest->GetBits();
	int pitch = param->pSrc->GetPitch();//一行像素的byteCount，但是假定src和Dest的尺寸是一样的
	int bytesPerPixel = param->pSrc->GetBPP() / 8;//一个像素的byteCount

	int *pixel = new int[maxLength];//存储每个像素点的灰度
	int *pixelR = new int[maxLength];
	int *pixelB = new int[maxLength];
	int *pixelG = new int[maxLength];
	int index = 0;
	for (int i = startIndex; i <= endIndex; ++i)
	{
		int Sxy = 1;
		int med = 0;
		int state = 0;
		int x = i % maxWidth;
		int y = i / maxWidth;
		while (Sxy <= maxSpan)
		{
			index = 0;
			for (int tmpY = y - Sxy; tmpY <= y + Sxy && tmpY <maxHeight; tmpY++)
			{
				if (tmpY < 0) continue;
				for (int tmpX = x - Sxy; tmpX <= x + Sxy && tmpX<maxWidth; tmpX++)
				{
					if (tmpX < 0) continue;
					if (bytesPerPixel == 1)
					{
						pixel[index] = *(pSrcRealData + pitch*(tmpY)+(tmpX)*bytesPerPixel);
						pixelR[index++] = pixel[index];

					}
					else
					{
						pixelR[index] = *(pSrcRealData + pitch*(tmpY)+(tmpX)*bytesPerPixel + 2);
						pixelG[index] = *(pSrcRealData + pitch*(tmpY)+(tmpX)*bytesPerPixel + 1);
						pixelB[index] = *(pSrcRealData + pitch*(tmpY)+(tmpX)*bytesPerPixel);
						pixel[index++] = int(pixelB[index] * 0.299 + 0.587*pixelG[index] + pixelR[index] * 0.144);

					}
				}

			}
			if (index <= 0)
				break;
			if ((state = GetValue(pixel, index, med)) == 1)
				break;

			Sxy++;
		};

		if (state)
		{
			if (bytesPerPixel == 1)
			{
				*(pDestRealData + pitch*y + x*bytesPerPixel) = pixelR[med];
			}
			else
			{
				*(pDestRealData + pitch*y + x*bytesPerPixel + 2) = pixelR[med];
				*(pDestRealData + pitch*y + x*bytesPerPixel + 1) = pixelG[med];
				*(pDestRealData + pitch*y + x*bytesPerPixel) = pixelB[med];

			}
		}

	}

	delete[] pixel;
	delete[] pixelR;
	delete[] pixelG;
	delete[] pixelB;

	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_MEDIAN_FILTER, 1, NULL);
	return 0;
}

UINT ImageProcessor::mFunction_AddNoiseForTargetRegion(LPVOID  p)
{
	ThreadParam* param = (ThreadParam*)p;
	int maxWidth = param->pSrc->GetWidth();
	int maxHeight = param->pSrc->GetHeight();

	int startIndex = param->startIndex;
	int endIndex = param->endIndex;
	byte* pSrcRealData = (byte*)param->pSrc->GetBits();
	byte* pDestRealData = (byte*)param->pDest->GetBits();

	int bytesPerPixel = param->pSrc->GetBPP() / 8;
	int pitch = param->pSrc->GetPitch();

	for (int i = startIndex; i <= endIndex; ++i)
	{
		int x = i % maxWidth;
		int y = i / maxWidth;
		//
		const float cNoiseThreshold = 0.2f;
		if ((rand() % 1000) * 0.001 < cNoiseThreshold)
		{
			int value = 0;
			if (rand() % 1000 < 500)
			{
				value = 0;
			}
			else
			{
				value = 255;
			}

			if (bytesPerPixel == 1)
			{
				//灰度图
				*(pDestRealData + pitch * y + x * bytesPerPixel) = value;
			}
			else
			{
				//RGB
				*(pDestRealData + pitch * y + x * bytesPerPixel) = value;
				*(pDestRealData + pitch * y + x * bytesPerPixel + 1) = value;
				*(pDestRealData + pitch * y + x * bytesPerPixel + 2) = value;
			}
		}
		else
		{
			//不然就当前像素不变成噪声，从原图copy像素过来
			int offset = pitch * y + x * bytesPerPixel;
			*(pDestRealData + offset+2) = *(pSrcRealData + offset+2);
			*(pDestRealData + offset +1) = *(pSrcRealData + offset+1);
			*(pDestRealData + offset +0 ) = *(pSrcRealData + offset+0);
		}
	}
	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_NOISE, 1, NULL);
	return 0;
}
