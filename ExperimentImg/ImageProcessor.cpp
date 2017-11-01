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


UINT ImageProcessor::MedianFilter_WIN(CImage* pImage, int numThreads, ThreadParam* pParamArray)
{
	int subLength = pImage->GetWidth() * pImage->GetHeight() / numThreads;
	for (int i = 0; i < numThreads; ++i)
	{
		pParamArray[i].startIndex = i * subLength;
		pParamArray[i].endIndex = i != numThreads - 1 ?
			(i + 1) * subLength - 1 : pImage->GetWidth() * pImage->GetHeight() - 1;
		pParamArray[i].maxSpan = cMedianFilterMaxSpan;
		pParamArray[i].src = pImage;

		//new thread
		AfxBeginThread((AFX_THREADPROC)&ImageProcessor::mFunction_MedianFilterForTargetRegion, &pParamArray[i]);
	}

	return 0;
}

UINT ImageProcessor::MedianFilter_OpenMP(CImage* pImage, int numThreads, ThreadParam* pParamArray)
{
	int subLength = pImage->GetWidth() * pImage->GetHeight() / numThreads;

	//OpenMP 并不是一个简单的函数库，而是被众多【编译器】所支持的并行计算
	//框架/协议，只需要在一个for前面加上#pragma omp parallel xxxxx 就可以
	//很舒服地把并行化的工作扔给编译器做，简直6666666666666！！！
#pragma omp parallel for num_threads(m_nThreadNum)
	for (int i = 0; i < numThreads; ++i)
	{
		pParamArray[i].startIndex = i * subLength;
		pParamArray[i].endIndex = i != numThreads - 1 ?
			(i + 1) * subLength - 1 : pImage->GetWidth() * pImage->GetHeight() - 1;
		pParamArray[i].maxSpan = cMedianFilterMaxSpan;
		pParamArray[i].src = pImage;
		ImageProcessor::mFunction_MedianFilterForTargetRegion(&pParamArray[i]);
	}
	return 0;
}

UINT ImageProcessor::AddNoise_WIN(CImage * pImage, int numThreads, ThreadParam * pParamArray)
{
	int subLength = pImage->GetWidth() * pImage->GetHeight() / numThreads;
	for (int i = 0; i < numThreads; ++i)
	{
		pParamArray[i].startIndex = i * subLength;
		pParamArray[i].endIndex = i != numThreads - 1 ?
			(i + 1) * subLength - 1 : pImage->GetWidth() * pImage->GetHeight() - 1;
		pParamArray[i].src = pImage;

		//new thread
		AfxBeginThread((AFX_THREADPROC)&ImageProcessor::mFunction_AddNoiseForTargetRegion, &pParamArray[i]);
	}
	return 0;
}

UINT ImageProcessor::AddNoise_OpenMP(CImage * pImage, int numThreads, ThreadParam * pParamArray)
{
	int subLength = pImage->GetWidth() * pImage->GetHeight() / numThreads;

	//OpenMP 并不是一个简单的函数库，而是被众多【编译器】所支持的并行计算
	//框架/协议，只需要在一个for前面加上#pragma omp parallel xxxxx 就可以
	//很舒服地把并行化的工作扔给编译器做，简直6666666666666！！！
#pragma omp parallel for num_threads(m_nThreadNum)
	for (int i = 0; i < numThreads; ++i)
	{
		pParamArray[i].startIndex = i * subLength;
		pParamArray[i].endIndex = i != numThreads - 1 ?
			(i + 1) * subLength - 1 : pImage->GetWidth() * pImage->GetHeight() - 1;
		pParamArray[i].src = pImage;
		ImageProcessor::mFunction_AddNoiseForTargetRegion(&pParamArray[i]);
	}
	return 0;
}

/**************************************

						PRIVATE

***************************************/

UINT ImageProcessor::mFunction_MedianFilterForTargetRegion(LPVOID  p)
{
	ThreadParam* param = (ThreadParam*)p;

	int maxWidth = param->src->GetWidth();
	int maxHeight = param->src->GetHeight();
	int startIndex = param->startIndex;
	int endIndex = param->endIndex;
	int maxSpan = param->maxSpan;
	int maxLength = (maxSpan * 2 + 1) * (maxSpan * 2 + 1);

	byte* pRealData = (byte*)param->src->GetBits();
	int pitch = param->src->GetPitch();//一行像素的byteCount
	int bytesPerPixel = param->src->GetBPP() / 8;//一个像素的byteCount

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
						pixel[index] = *(pRealData + pitch*(tmpY)+(tmpX)*bytesPerPixel);
						pixelR[index++] = pixel[index];

					}
					else
					{
						pixelR[index] = *(pRealData + pitch*(tmpY)+(tmpX)*bytesPerPixel + 2);
						pixelG[index] = *(pRealData + pitch*(tmpY)+(tmpX)*bytesPerPixel + 1);
						pixelB[index] = *(pRealData + pitch*(tmpY)+(tmpX)*bytesPerPixel);
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
				*(pRealData + pitch*y + x*bytesPerPixel) = pixelR[med];

			}
			else
			{
				*(pRealData + pitch*y + x*bytesPerPixel + 2) = pixelR[med];
				*(pRealData + pitch*y + x*bytesPerPixel + 1) = pixelG[med];
				*(pRealData + pitch*y + x*bytesPerPixel) = pixelB[med];

			}
		}

	}

	delete[]pixel;
	delete[]pixelR;
	delete[]pixelG;
	delete[]pixelB;

	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_MEDIAN_FILTER, 1, NULL);
	return 0;
}

UINT ImageProcessor::mFunction_AddNoiseForTargetRegion(LPVOID  p)
{
	ThreadParam* param = (ThreadParam*)p;
	int maxWidth = param->src->GetWidth();
	int maxHeight = param->src->GetHeight();

	int startIndex = param->startIndex;
	int endIndex = param->endIndex;
	byte* pRealData = (byte*)param->src->GetBits();
	int bytesPerPixel = param->src->GetBPP() / 8;
	int pitch = param->src->GetPitch();

	for (int i = startIndex; i <= endIndex; ++i)
	{
		int x = i % maxWidth;
		int y = i / maxWidth;
		if ((rand() % 1000) * 0.001 < NOISE)
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
				*(pRealData + pitch * y + x * bytesPerPixel) = value;
			}
			else
			{
				*(pRealData + pitch * y + x * bytesPerPixel) = value;
				*(pRealData + pitch * y + x * bytesPerPixel + 1) = value;
				*(pRealData + pitch * y + x * bytesPerPixel + 2) = value;
			}
		}
	}
	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_NOISE, 1, NULL);
	return 0;
}
