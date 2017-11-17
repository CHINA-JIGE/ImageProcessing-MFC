#include "stdafx.h"
#include "ImageProcessor.h"
#include <vector>
#include <algorithm>

//中值滤波用到，不知道干嘛的。。。
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
			COLOR3 c = mFunction_GetPixel(pImgSrc, sampleCoordX, sampleCoordY);
			mFunction_SetPixel(pImgDest, i, j, c);
			//COLORREF sampleColor =pImgSrc->GetPixel(sampleCoordX, sampleCoordY);
			//pImgDest->SetPixel(i, j, sampleColor);
		}
	}
}

UINT ImageProcessor::MedianFilter_WIN(CImage* pImgSrc, CImage* pImgDest, int numThreads)
{
	int subLength = pImgSrc->GetWidth() * pImgSrc->GetHeight() / numThreads;

	ThreadParam* pParamArray = new ThreadParam[numThreads];
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

	ThreadParam* pParamArray = new ThreadParam[numThreads];
	//OpenMP 并不是一个简单的函数库，而是被众多【编译器】所支持的并行计算
	//框架/协议，只需要在一个for前面加上#pragma omp parallel xxxxx 就可以
	//很舒服地把并行化的工作扔给编译器做，简直6666666666666！！！
#pragma omp parallel for num_threads(numThreads)
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
	ThreadParam* pParamArray = new ThreadParam[numThreads];
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
	ThreadParam* pParamArray = new ThreadParam[numThreads];
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

UINT ImageProcessor::Rotate_WIN(CImage * pImgSrc, CImage * pImgDest, int numThreads, float radianAngle,float scaleFactor)
{
	int subLength = pImgSrc->GetWidth() * pImgSrc->GetHeight() / numThreads;
	ThreadParam_Rotation* pParamArray = new ThreadParam_Rotation[numThreads];
	for (int i = 0; i < numThreads; ++i)
	{
		pParamArray[i].startIndex = i * subLength;
		pParamArray[i].endIndex = i != numThreads - 1 ?
			(i + 1) * subLength - 1 : pImgSrc->GetWidth() * pImgSrc->GetHeight() - 1;
		pParamArray[i].pSrc = pImgSrc;
		pParamArray[i].pDest = pImgDest;
		pParamArray[i].radianAngle = radianAngle;
		pParamArray[i].scaleFactor = scaleFactor;
		//new thread
		AfxBeginThread((AFX_THREADPROC)&ImageProcessor::mFunction_RotateForTargetRegion, &pParamArray[i]);
	}
	return 0;
}

UINT ImageProcessor::Rotate_OpenMP(CImage * pImgSrc, CImage * pImgDest, int numThreads, float radianAngle,float scaleFactor)
{
	int subLength = pImgSrc->GetWidth() * pImgSrc->GetHeight() / numThreads;
	ThreadParam_Rotation* pParamArray = new ThreadParam_Rotation[numThreads];
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
		pParamArray[i].radianAngle = radianAngle;
		pParamArray[i].scaleFactor = scaleFactor;
		ImageProcessor::mFunction_RotateForTargetRegion(&pParamArray[i]);
	}
	return 0;
}

UINT ImageProcessor::Rotate_CUDA(CImage * pImgSrc, CImage * pImgDest, float radianAngle, float scaleFactor)
{	
	//转到.cu文件看CUDA c++代码吧，这个函数在ImageProcessor.h里面
	//extern "C"地声明了一波
	int w = pImgSrc->GetWidth();
	int h = pImgSrc->GetHeight();
	int pitch = pImgSrc->GetPitch();
	int positivePitch = abs(pitch);//pitch居然可以是负的，我服气了
	const int bytesPerPixel = sizeof(COLOR3);

	//CImage的像素数据储存太反人类了。。。
	//pitch是负的，所以在GetBits()的正和负方向都有数据，服。。。
	byte* pSrcData = (byte*)pImgSrc->GetBits();
	byte* pDestData = (byte*)pImgDest->GetBits();
	byte* pSrcDataLeastAddress = pSrcData-(h-1)*positivePitch;
	byte* pDestDataLeastAddress = pDestData- (h - 1)*positivePitch;

	cudaHost_RotateAndScale(
		(const unsigned char*)pSrcDataLeastAddress,
		(unsigned char*)pDestDataLeastAddress,
		w,h,positivePitch,radianAngle,scaleFactor);

	//因为不是cpu多线程，cuda的线程已经在.cu里面sync过了，所以
	//就不post调用回调函数的那个message了
	CTime endTime = CTime::GetTickCount();
	CString timeStr;
	timeStr.Format(_T("CUDA计算耗时:0 s"));
	AfxMessageBox(timeStr);
	//主动触发picturebox的onPaint事件
	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_PAINT, 1, NULL);

	return 0;
}

UINT ImageProcessor::AutoLevels_OpenMP(CImage * pImgSrc, CImage * pImgDest, int numThreads)
{
	ThreadParam_AutoLevels param;
	//OpenMP 并不是一个简单的函数库，而是被众多【编译器】所支持的并行计算
	param.pSrc = pImgSrc;
	param.pDest = pImgDest;
	ImageProcessor::mFunction_AutoLevels(&param);
	return 0;
}

UINT ImageProcessor::AutoLevels_CUDA(CImage * pImgSrc, CImage * pImgDest)
{
	//转到.cu文件看CUDA c++代码吧，这个函数在ImageProcessor.h里面
	//extern "C"地声明了一波
	int w = pImgSrc->GetWidth();
	int h = pImgSrc->GetHeight();
	int pitch = pImgSrc->GetPitch();
	int positivePitch = abs(pitch);//pitch居然可以是负的，我服气了
	const int bytesPerPixel = sizeof(COLOR3);

	//统计像素值时低切与高切的threshold
	const byte lowCut = 20;
	const byte highCut = 230;

	//统计直方图，遍历全图这种东西就不要
	byte maxR = 0, maxG = 0, maxB = 0;
	byte minR = 255, minG = 255, minB = 255;
#pragma omp parallel for num_threads(mThreadNum)
	for (int j = 0; j < h; ++j)
	{
		for (int i = 0; i < w; ++i)
		{
			COLOR3 c = mFunction_GetPixel(pImgSrc, i, j);
			if (c.r > maxR)maxR = c.r;
			if (c.g > maxG)maxG = c.g;
			if (c.b > maxB)maxB = c.b;
			if (c.r < minR)minR = c.r;
			if (c.g < minG)minG = c.g;
			if (c.b < minB)minB = c.b;
		}
	}
	if (maxR > highCut)maxR = highCut;
	if (maxG > highCut)maxG = highCut;
	if (maxB > highCut)maxB = highCut;
	if (minR < lowCut)minR = lowCut;
	if (minG < lowCut)minG = lowCut;
	if (minB < lowCut)minB = lowCut;

	//CImage的像素数据储存太反人类了。。。
	//pitch是负的，所以在GetBits()的正和负方向都有数据，服。。。
	byte* pSrcData = (byte*)pImgSrc->GetBits();
	byte* pDestData = (byte*)pImgDest->GetBits();
	byte* pSrcDataLeastAddress = pSrcData - (h - 1)*positivePitch;
	byte* pDestDataLeastAddress = pDestData - (h - 1)*positivePitch;

	cudaHost_AutoLevels(
		(const unsigned char*)pSrcDataLeastAddress,
		(unsigned char*)pDestDataLeastAddress,
		w, h, positivePitch, minR, maxR, minG, maxG, minB, maxB);


	//因为不是cpu多线程，cuda的线程已经在.cu里面sync过了，所以
	//就不post调用回调函数的那个message了
	CTime endTime = CTime::GetTickCount();
	CString timeStr;
	timeStr.Format(_T("CUDA计算耗时:0 s"));
	AfxMessageBox(timeStr);
	//主动触发picturebox的onPaint事件
	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_PAINT, 1, NULL);

	return 0;
}

UINT ImageProcessor::AutoWhiteBalance_OpenMP(CImage * pImgSrc, CImage * pImgDest, int numThreads)
{
	ThreadParam_AutoWhiteBalance param;
	//OpenMP 并不是一个简单的函数库，而是被众多【编译器】所支持的并行计算
	param.pSrc = pImgSrc;
	param.pDest = pImgDest;
	ImageProcessor::mFunction_AutoWhiteBalance(&param);
	return 0;
}

UINT ImageProcessor::ImageBlending_WIN(CImage * pImgSrc1, CImage * pImgSrc2, CImage * pImgDest, int numThreads, float alpha)
{
	//it's been assumed that img1 / img2 / imgDest are with SAME SIZE!!!!

	int w = pImgSrc1->GetWidth();
	int h = pImgSrc1->GetHeight();
	int subLength = w* h / numThreads;
	ThreadParam_ImageBlending* pParamArray = new ThreadParam_ImageBlending[numThreads];
	for (int i = 0; i < numThreads; ++i)
	{
		pParamArray[i].startIndex = i * subLength;
		pParamArray[i].endIndex = i != numThreads - 1 ?
			(i + 1) * subLength - 1 : w * h - 1;
		pParamArray[i].pSrc1 = pImgSrc1;
		pParamArray[i].pSrc2 = pImgSrc2;
		pParamArray[i].pDest = pImgDest;
		pParamArray[i].alpha = alpha;

		//new thread
		AfxBeginThread((AFX_THREADPROC)&ImageProcessor::mFunction_ImageBlending, &pParamArray[i]);
	}
	return 0;
}

UINT ImageProcessor::ImageBlending_OpenMP(CImage * pImgSrc1, CImage * pImgSrc2, CImage * pImgDest, int numThreads, float alpha)
{
	int w = pImgSrc1->GetWidth();
	int h = pImgSrc1->GetHeight();
	int subLength = w* h / numThreads;
	ThreadParam_ImageBlending* pParamArray = new ThreadParam_ImageBlending[numThreads];
	//OpenMP 并不是一个简单的函数库，而是被众多【编译器】所支持的并行计算
	//框架/协议，只需要在一个for前面加上#pragma omp parallel xxxxx 就可以
	//很舒服地把并行化的工作扔给编译器做，简直6666666666666！！！
#pragma omp parallel for num_threads(mThreadNum)
	for (int i = 0; i < numThreads; ++i)
	{
		pParamArray[i].startIndex = i * subLength;
		pParamArray[i].endIndex = i != numThreads - 1 ?
			(i + 1) * subLength - 1 : w * h - 1;
		pParamArray[i].pSrc1 = pImgSrc1;
		pParamArray[i].pSrc2 = pImgSrc2;
		pParamArray[i].pDest = pImgDest;
		pParamArray[i].alpha = alpha;
		ImageProcessor::mFunction_ImageBlending(&pParamArray[i]);
	}
	return 0;
}

UINT ImageProcessor::BilateralFilter_OpenMP(CImage * pImgSrc, CImage * pImgDest, int numThreads)
{
	int subLength = pImgSrc->GetWidth() * pImgSrc->GetHeight() / numThreads;
	ThreadParam_BilateralFilter* pParamArray = new ThreadParam_BilateralFilter[numThreads];
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
		pParamArray[i].sigma_d = 8.0f;
		pParamArray[i].sigma_r = 20.0f;
		ImageProcessor::mFunction_BilateralFilter(&pParamArray[i]);
	}
	return 0;
}

/**************************************

						PRIVATE

***************************************/

inline void ImageProcessor::mFunction_SetPixel(CImage* pImage, int x, int y,const COLOR3& c)
{
	//int bytesPerPixel = pImage->GetBPP() / 8;
	const int bytesPerPixel = 3;
	int pitch = pImage->GetPitch();
	byte* pData = (byte*)pImage->GetBits();

	*(pData + pitch * y + x * bytesPerPixel+ 2) = c.r;
	*(pData + pitch * y + x * bytesPerPixel+ 1) = c.g;
	*(pData + pitch * y + x * bytesPerPixel+ 0) = c.b;

}

inline COLOR3 ImageProcessor::mFunction_GetPixel(CImage * pImage, int x, int y)
{
	//int bytesPerPixel = pImage->GetBPP() / 8;
	const int bytesPerPixel = 3;
	int pitch = pImage->GetPitch();
	byte* pData = (byte*)pImage->GetBits();

	byte r = *(pData + pitch*y + x*bytesPerPixel + 2);
	byte g = *(pData + pitch*y + x*bytesPerPixel + 1);
	byte b = *(pData + pitch*y + x*bytesPerPixel + 0);
	return COLOR3(r, g, b);
}

UINT ImageProcessor::mFunction_MedianFilterForTargetRegion(LPVOID  pThreadParam)
{
	ThreadParam* param = (ThreadParam*)pThreadParam;

	int maxWidth = param->pSrc->GetWidth();
	int maxHeight = param->pSrc->GetHeight();
	int startIndex = param->startIndex;
	int endIndex = param->endIndex;
	int maxSpan = param->maxSpan;
	int maxLength = (maxSpan * 2 + 1) * (maxSpan * 2 + 1);

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

					COLOR3 c = mFunction_GetPixel(param->pSrc, tmpX, tmpY);
					pixelR[index] = c.r;
					pixelG[index] = c.g;
					pixelB[index] = c.b;
					pixel[index++] = int(c.b * 0.299f + 0.587f*c.g + c.r * 0.144f);
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
			COLOR3 c(pixelR[med], pixelG[med], pixelB[med]);
			mFunction_SetPixel(param->pDest, x, y, c);
		}

	}

	delete[] pixel;
	delete[] pixelR;
	delete[] pixelG;
	delete[] pixelB;

	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_MEDIAN_FILTER, 1, NULL);
	return 0;
}

UINT ImageProcessor::mFunction_AddNoiseForTargetRegion(LPVOID  pThreadParam)
{
	ThreadParam* param = (ThreadParam*)pThreadParam;
	int maxWidth = param->pSrc->GetWidth();
	int maxHeight = param->pSrc->GetHeight();
	int startIndex = param->startIndex;
	int endIndex = param->endIndex;

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

			//RGB
			mFunction_SetPixel(param->pDest, x, y, COLOR3(value, value, value));
		}
		else
		{
			//不然就当前像素不变成噪声，从原图copy像素过来
			COLOR3 c = mFunction_GetPixel(param->pSrc, x, y);
			mFunction_SetPixel(param->pDest, x, y, c);
		}
	}
	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_NOISE, 1, NULL);
	return 0;
}

UINT ImageProcessor::mFunction_RotateForTargetRegion(LPVOID pThreadParam)
{
	ThreadParam_Rotation* param = (ThreadParam_Rotation*)pThreadParam;
	int maxWidth = param->pSrc->GetWidth();
	int maxHeight = param->pSrc->GetHeight();
	float halfPixelWidth = float(maxWidth) / 2.0f;//用于坐标的归一化
	float halfPixelHeight = float(maxHeight) / 2.0f;
	int startIndex = param->startIndex;
	int endIndex = param->endIndex;
	float angle = param->radianAngle;//旋转角度
	float scaleFactor = param->scaleFactor;

	//整个过程就像在写pixel shader
	for (int i = startIndex; i <= endIndex; ++i)
	{
		int x = i % maxWidth;
		int y = i / maxWidth;

		//normalized X Y are mapped to [-1,1], centered at the center of screen
		float centeredPixelX = float(x - halfPixelWidth) *(1.0f / scaleFactor);
		float centeredPixelY = float(halfPixelHeight - y)*(1.0f / scaleFactor);
		/*
		[cos	sin]	[x]
		[-sin	cos]	[y]
		*/

		//缩放图片，此处的缩放系数和缩放效果成【反比】，自己好好想想
		float centeredRotatedPixelX =  -(centeredPixelX  * cosf(angle) - centeredPixelY * sinf(angle));
		float centeredRotatedPixelY =  -(centeredPixelX * sinf(angle) + centeredPixelY * cosf(angle));
		
		//fRotatedPixelX和Y的除以scaleFactor是纹理的放缩
		float fRotatedPixelX = (centeredRotatedPixelX + halfPixelWidth) / 2.0f / scaleFactor;
		float fRotatedPixelY = (halfPixelHeight - centeredRotatedPixelY) / 2.0f / scaleFactor;

		COLOR3 sampleColor;
		//如果当前像素旋转后没有出界（意味着可以采样）
		if (fRotatedPixelX >= 0 && fRotatedPixelX < maxWidth-1 &&
			fRotatedPixelY >= 0 && fRotatedPixelY < maxHeight-1)
		{
			int rotatedPixelX = int(fRotatedPixelX);
			int rotatedPixelY = int(fRotatedPixelY);
			COLOR3 c1 = mFunction_GetPixel(param->pSrc, rotatedPixelX, rotatedPixelY);
			COLOR3 c2 = mFunction_GetPixel(param->pSrc, rotatedPixelX+1, rotatedPixelY);
			COLOR3 c3 = mFunction_GetPixel(param->pSrc, rotatedPixelX, rotatedPixelY+1);
			COLOR3 c4 = mFunction_GetPixel(param->pSrc, rotatedPixelX+1, rotatedPixelY+1);
			//插值系数
			float t1 = fRotatedPixelX - float(rotatedPixelX);
			float t2 = fRotatedPixelY - float(rotatedPixelY);

			//Hermite三阶插值
			auto Hermite = [](float t, const COLOR3& c1, const COLOR3& c2)->COLOR3
			{
				//hermite插值的卷积核  : 2|x|^3 - 3|x|^2 +1，把考察点放在卷积核函数的原点
				float factor1 = 2.0f * t * t * t - 3.0f * t * t + 1.0f;
				float factor2 = 2.0f * (1.0f-t) * (1.0f - t) * (1.0f - t) - 3.0f * (1.0f - t) * (1.0f - t) + 1.0f;
				return COLOR3(
					c1.r * factor1 + c2.r*factor2,
					c1.g * factor1 + c2.g*factor2,
					c1.b * factor1 + c2.b*factor2);
			};
			COLOR3 tmp1 = Hermite(t1, c1, c2);
			COLOR3 tmp2 = Hermite(t1, c3, c4);
			sampleColor = Hermite(t2, tmp1, tmp2);
		}
		else
		{
			//出界的给我变黑好吧
			sampleColor = COLOR3(0, 0, 0);
		}

		mFunction_SetPixel(param->pDest, x, y, sampleColor);
	}
	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_BICUBIC_FILTER_ROTATION, 1, NULL);
	return 0;
}

UINT ImageProcessor::mFunction_AutoLevels(LPVOID pThreadParam)
{
	//在这里自动色阶被搞成了单线程算法...
	ThreadParam_AutoLevels* param = (ThreadParam_AutoLevels*)pThreadParam;
	int maxWidth = param->pSrc->GetWidth();
	int maxHeight = param->pSrc->GetHeight();

	/*
	自动色阶的逻辑:
	f x<x_min:  y=0.0;
	if x>x_max:  y=1.0;
	if x_min < x< x_max:   y=(x-x_min)/(x_max-x_min); 
	*/

	//统计像素值时低切与高切的threshold
	const byte lowCut = 20;
	const byte highCut = 230;

	//统计直方图
	byte maxR = 0, maxG = 0, maxB = 0;
	byte minR= 255,	minG= 255,	minB= 255;
#pragma omp parallel for num_threads(mThreadNum)
	for (int j = 0; j < maxHeight; ++j)
	{
		for (int i = 0; i < maxWidth; ++i)
		{
			COLOR3 c = mFunction_GetPixel(param->pSrc, i, j);
			if (c.r > maxR)maxR = c.r;
			if (c.g > maxG)maxG = c.g;
			if (c.b > maxB)maxB = c.b;
			if (c.r < minR)minR = c.r;
			if (c.g < minG)minG = c.g;
			if (c.b < minB)minB = c.b;
		}
	}
	if (maxR > highCut)maxR = highCut;
	if (maxG > highCut)maxG = highCut;
	if (maxB > highCut)maxB = highCut;
	if (minR < lowCut)minR = lowCut;
	if (minG < lowCut)minG = lowCut;
	if (minB < lowCut)minB = lowCut;


	//映射颜色区间
	byte rangeR = maxR - minR;
	byte rangeG = maxG - minG;
	byte rangeB = maxB - minB;
#pragma omp parallel for num_threads(mThreadNum)
	for (int j = 0; j < maxHeight; ++j)
	{
		for (int i = 0; i < maxWidth; ++i)
		{
			COLOR3 c = mFunction_GetPixel(param->pSrc, i, j);

			//如果某个通道的动态范围是0，那还调什么色阶啊
			if (rangeR != 0)
			{
				float ratioR = float(c.r - minR) / rangeR;
				if(ratioR>=0.0f && ratioR<=1.0f) c.r = byte(255.0f * ratioR);
			}

			if (rangeG != 0)
			{
				float ratioG = float(c.g - minG) / rangeG;
				if (ratioG >= 0.0f && ratioG<=1.0f)c.g = byte(255.0f * ratioG);
			}

			if (rangeB != 0)
			{
				float ratioB = float(c.b - minB) / rangeB;
				if (ratioB >= 0.0f && ratioB<=1.0f)c.b = byte(255.0f * ratioB);
			}

			//输出
			mFunction_SetPixel(param->pDest,i, j, c);
		}
	}

	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_AUTO_LEVELS, 1, NULL);
	return 0;
}

UINT ImageProcessor::mFunction_AutoWhiteBalance(LPVOID pThreadParam)
{
	//在这里自动色阶被搞成了单线程算法...
	ThreadParam_AutoLevels* param = (ThreadParam_AutoLevels*)pThreadParam;
	int maxWidth = param->pSrc->GetWidth();
	int maxHeight = param->pSrc->GetHeight();

	/*
	自动白平衡的逻辑:
	灰度世界算法（Gray World)是以灰度世界假设为基础的,该假设认为对于一幅有着大量色彩变化的图像,
	R、 G、 B 三个分量的平均值趋于同一个灰度K。一般有两种方法来确定该灰度。
	(1)直接给定为固定值, 取其各通道最大值的一半,即取为127或128；
	(2)令 K = (R_avg+G_avg+B_avg)/3,其中R_aver,G_aver,B_aver分别表示红、 绿、 蓝三个通道的平均值。
	算法的第二步是分别计算各通道的增益：
	Kr=K/R_avg;
	Kg=K/G_avg;
	Kb=K/B_avg;
	 算法第三步为根据Von Kries 对角模型,对于图像中的每个像素R、G、B，计算其结果值：
	 Rnew = R * Kr;
	 Gnew = G * Kg;
	 Bnew = B * Kb;
	*/


	//三通道像素值平均值
	float avgR=0.0f, avgG=0.0f, avgB=0.0f;
	float K = 0;
	//颜色增益
	float Kr, Kg, Kb;

	uint64_t sumR = 0, sumG = 0, sumB = 0;
	for (int j = 0; j < maxHeight; ++j)
	{
		for (int i = 0; i < maxWidth; ++i)
		{
			COLOR3 c = mFunction_GetPixel(param->pSrc, i, j);
			sumR += c.r;
			sumG += c.g;
			sumB += c.b;
		}
	}
	avgR = (double(sumR) / (maxWidth*maxHeight));
	avgG = (double(sumG) / (maxWidth*maxHeight));
	avgB = (double(sumB) / (maxWidth*maxHeight));
	K = (avgR + avgG + avgB) / 3.0f;
	Kr = K / avgR;
	Kg = K / avgG;
	Kb = K / avgB;


#pragma omp parallel for num_threads(mThreadNum)
	for (int j = 0; j < maxHeight; ++j)
	{
		for (int i = 0; i < maxWidth; ++i)
		{
			COLOR3 c = mFunction_GetPixel(param->pSrc, i, j);

			//颜色各分量乘以增益
			int r = int(c.r * Kr);
			int g = int(c.g * Kg);
			int b = int(c.b * Kb);
			if (r > 255)r = 255;
			if (g > 255)g = 255;
			if (b > 255)b = 255;
			c.r =r;
			c.g = g;
			c.b = b;

			//输出
			mFunction_SetPixel(param->pDest, i, j, c);
		}
	}

	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_AUTO_WHITE_BALANCE, 1, NULL);
	return 0;
}

UINT ImageProcessor::mFunction_ImageBlending(LPVOID pThreadParam)
{
	//在这里自动色阶被搞成了单线程算法...
	ThreadParam_ImageBlending* param = (ThreadParam_ImageBlending*)pThreadParam;
	int maxWidth = param->pSrc1->GetWidth();
	int maxHeight = param->pSrc1->GetHeight();

	for (int i = param->startIndex; i <= param->endIndex; ++i)
	{
		int x = i % maxWidth;
		int y = i / maxWidth;

		COLOR3 c1 = mFunction_GetPixel(param->pSrc1, x, y);
		COLOR3 c2 = mFunction_GetPixel(param->pSrc2, x, y);
		float alpha = param->alpha;
		COLOR3 c =
		{
			byte(c2.r * alpha + c1.r * (1.0f - alpha)),
			byte(c2.g * alpha + c1.g * (1.0f - alpha)),
			byte(c2.b * alpha + c1.b * (1.0f - alpha))
		};

		//输出
		mFunction_SetPixel(param->pDest, x,y, c);
	}

	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_IMAGE_BLENDING, 1, NULL);
	return 0;
}

UINT ImageProcessor::mFunction_BilateralFilter(LPVOID pThreadParam)
{
	/*
	这个函数是有那么一点点刺激吧，算是高斯滤波的一种改进
	综合了定义域和值域来计算每一个权重矩阵的系数
	具体看看struct BilateralFilterMaskMatrix 那里的注释吧
	*/
	ThreadParam_BilateralFilter* param = (ThreadParam_BilateralFilter*)pThreadParam;
	int maxWidth = param->pSrc->GetWidth();
	int maxHeight = param->pSrc->GetHeight();

	//移动权值矩阵的尺寸
	int stencilMatrixHalfW = BilateralFilterWeightMatrix::cMatrixHalfWidth;
	int stencilMatrixHalfH = BilateralFilterWeightMatrix::cMatrixHalfHeight;
	int stencilMatrixW = BilateralFilterWeightMatrix::cMatrixWidth;
	int stencilMatrixH = BilateralFilterWeightMatrix::cMatrixHeight;

	for (int i = param->startIndex; i <= param->endIndex; ++i)
	{
		int x = i % maxWidth;
		int y = i / maxWidth;

		//为了我自己舒服一点，边界的像素我是打算忽略掉不滤波的- -
		//这个if保证了每个考察的像素点(i,j)都可以忽略边界条件地使用权值矩阵
		if (x >= stencilMatrixHalfW  && x < (maxWidth - stencilMatrixHalfW) &&
			y >= stencilMatrixHalfH  && y < (maxHeight - stencilMatrixHalfH))
		{
			//每一个像素点(i,j)我们都要计算它的双边滤波权重矩阵
			BilateralFilterWeightMatrix weightMatrixR;
			BilateralFilterWeightMatrix weightMatrixG;
			BilateralFilterWeightMatrix weightMatrixB;

			//由于每次计算权重矩阵我们都要计算“值域核”，这意味着原图像的这一小
			//矩阵区域的像素都要传进去...好麻烦啊。
			//而且好像是是对于【灰度图】做的操作，所以要一个个通道地处理了啊
			std::vector<byte> imageRegionR(stencilMatrixW * stencilMatrixH);
			std::vector<byte> imageRegionG(stencilMatrixW * stencilMatrixH);
			std::vector<byte> imageRegionB(stencilMatrixW * stencilMatrixH);
			for (int l = -stencilMatrixHalfH; l <= stencilMatrixHalfH; ++l)
			{
				for (int k = -stencilMatrixHalfW; k <= stencilMatrixHalfW; ++k)
				{
					COLOR3 c = mFunction_GetPixel(param->pSrc, x + k, y + l);
					int index = (l + stencilMatrixHalfH) * stencilMatrixW + (k + stencilMatrixHalfW);
					imageRegionR[index] = c.r;
					imageRegionG[index] = c.g;
					imageRegionB[index] = c.b;
				}
			}

			//为考察点(i,j)的每个通道都最终输出值
			float resultR = weightMatrixR.ComputeValue(param->sigma_d, param->sigma_r, imageRegionR);
			float resultG = weightMatrixG.ComputeValue(param->sigma_d, param->sigma_r, imageRegionG);
			float resultB = weightMatrixB.ComputeValue(param->sigma_d, param->sigma_r, imageRegionB);

			COLOR3 result = { byte(resultR),byte(resultG),byte(resultB) };
			mFunction_SetPixel(param->pDest, x, y, result);
		}
		else
		{
			//边缘区域不滤波- -直接copy，比较懒
			COLOR3 c = mFunction_GetPixel(param->pSrc, x, y);
			mFunction_SetPixel(param->pDest, x, y, c);
		}
	}

	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_BILATERAL_FILTER, 1, NULL);
	return 0;
}
