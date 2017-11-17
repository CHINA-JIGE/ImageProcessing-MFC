#include "stdafx.h"
#include "ImageProcessor.h"
#include <vector>
#include <algorithm>

//��ֵ�˲��õ�����֪������ġ�����
static bool GetValue(int p[], int size, int &value)
{
	//�����м��ֵ
	int zxy = p[(size - 1) / 2];
	//���ڼ�¼ԭ������±�
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
		AfxMessageBox(_T("ImageCopy:ͼƬ����ʧ�ܣ�ԴͼƬwidth/heightΪ0"));
		return;
	}

	if (destWidth == 0 || destHeight == 0)
	{
		AfxMessageBox(_T("ImageCopy:ͼƬ����ʧ�ܣ�Ŀ��ͼƬwidth/heightΪ0"));
		return;
	}

#pragma omp parallel for num_threads(mThreadNum)
	for (int j = 0; j < destHeight; ++j)
	{
		for (int i = 0; i < destWidth; ++i)
		{
			float normalizedX = float(i) / destWidth;
			float normalizedY = float(j) / destHeight;
			//0.999 ��������ȡ���󵥵��ֵ��������ʱ����Խ��
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
	//OpenMP ������һ���򵥵ĺ����⣬���Ǳ��ڶࡾ����������֧�ֵĲ��м���
	//���/Э�飬ֻ��Ҫ��һ��forǰ�����#pragma omp parallel xxxxx �Ϳ���
	//������ذѲ��л��Ĺ����Ӹ�������������ֱ6666666666666������
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
	//OpenMP ������һ���򵥵ĺ����⣬���Ǳ��ڶࡾ����������֧�ֵĲ��м���
	//���/Э�飬ֻ��Ҫ��һ��forǰ�����#pragma omp parallel xxxxx �Ϳ���
	//������ذѲ��л��Ĺ����Ӹ�������������ֱ6666666666666������
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
	//OpenMP ������һ���򵥵ĺ����⣬���Ǳ��ڶࡾ����������֧�ֵĲ��м���
	//���/Э�飬ֻ��Ҫ��һ��forǰ�����#pragma omp parallel xxxxx �Ϳ���
	//������ذѲ��л��Ĺ����Ӹ�������������ֱ6666666666666������
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
	//ת��.cu�ļ���CUDA c++����ɣ����������ImageProcessor.h����
	//extern "C"��������һ��
	int w = pImgSrc->GetWidth();
	int h = pImgSrc->GetHeight();
	int pitch = pImgSrc->GetPitch();
	int positivePitch = abs(pitch);//pitch��Ȼ�����Ǹ��ģ��ҷ�����
	const int bytesPerPixel = sizeof(COLOR3);

	//CImage���������ݴ���̫�������ˡ�����
	//pitch�Ǹ��ģ�������GetBits()�����͸����������ݣ���������
	byte* pSrcData = (byte*)pImgSrc->GetBits();
	byte* pDestData = (byte*)pImgDest->GetBits();
	byte* pSrcDataLeastAddress = pSrcData-(h-1)*positivePitch;
	byte* pDestDataLeastAddress = pDestData- (h - 1)*positivePitch;

	cudaHost_RotateAndScale(
		(const unsigned char*)pSrcDataLeastAddress,
		(unsigned char*)pDestDataLeastAddress,
		w,h,positivePitch,radianAngle,scaleFactor);

	//��Ϊ����cpu���̣߳�cuda���߳��Ѿ���.cu����sync���ˣ�����
	//�Ͳ�post���ûص��������Ǹ�message��
	CTime endTime = CTime::GetTickCount();
	CString timeStr;
	timeStr.Format(_T("CUDA�����ʱ:0 s"));
	AfxMessageBox(timeStr);
	//��������picturebox��onPaint�¼�
	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_PAINT, 1, NULL);

	return 0;
}

UINT ImageProcessor::AutoLevels_OpenMP(CImage * pImgSrc, CImage * pImgDest, int numThreads)
{
	ThreadParam_AutoLevels param;
	//OpenMP ������һ���򵥵ĺ����⣬���Ǳ��ڶࡾ����������֧�ֵĲ��м���
	param.pSrc = pImgSrc;
	param.pDest = pImgDest;
	ImageProcessor::mFunction_AutoLevels(&param);
	return 0;
}

UINT ImageProcessor::AutoLevels_CUDA(CImage * pImgSrc, CImage * pImgDest)
{
	//ת��.cu�ļ���CUDA c++����ɣ����������ImageProcessor.h����
	//extern "C"��������һ��
	int w = pImgSrc->GetWidth();
	int h = pImgSrc->GetHeight();
	int pitch = pImgSrc->GetPitch();
	int positivePitch = abs(pitch);//pitch��Ȼ�����Ǹ��ģ��ҷ�����
	const int bytesPerPixel = sizeof(COLOR3);

	//ͳ������ֵʱ��������е�threshold
	const byte lowCut = 20;
	const byte highCut = 230;

	//ͳ��ֱ��ͼ������ȫͼ���ֶ����Ͳ�Ҫ
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

	//CImage���������ݴ���̫�������ˡ�����
	//pitch�Ǹ��ģ�������GetBits()�����͸����������ݣ���������
	byte* pSrcData = (byte*)pImgSrc->GetBits();
	byte* pDestData = (byte*)pImgDest->GetBits();
	byte* pSrcDataLeastAddress = pSrcData - (h - 1)*positivePitch;
	byte* pDestDataLeastAddress = pDestData - (h - 1)*positivePitch;

	cudaHost_AutoLevels(
		(const unsigned char*)pSrcDataLeastAddress,
		(unsigned char*)pDestDataLeastAddress,
		w, h, positivePitch, minR, maxR, minG, maxG, minB, maxB);


	//��Ϊ����cpu���̣߳�cuda���߳��Ѿ���.cu����sync���ˣ�����
	//�Ͳ�post���ûص��������Ǹ�message��
	CTime endTime = CTime::GetTickCount();
	CString timeStr;
	timeStr.Format(_T("CUDA�����ʱ:0 s"));
	AfxMessageBox(timeStr);
	//��������picturebox��onPaint�¼�
	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_PAINT, 1, NULL);

	return 0;
}

UINT ImageProcessor::AutoWhiteBalance_OpenMP(CImage * pImgSrc, CImage * pImgDest, int numThreads)
{
	ThreadParam_AutoWhiteBalance param;
	//OpenMP ������һ���򵥵ĺ����⣬���Ǳ��ڶࡾ����������֧�ֵĲ��м���
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
	//OpenMP ������һ���򵥵ĺ����⣬���Ǳ��ڶࡾ����������֧�ֵĲ��м���
	//���/Э�飬ֻ��Ҫ��һ��forǰ�����#pragma omp parallel xxxxx �Ϳ���
	//������ذѲ��л��Ĺ����Ӹ�������������ֱ6666666666666������
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
	//OpenMP ������һ���򵥵ĺ����⣬���Ǳ��ڶࡾ����������֧�ֵĲ��м���
	//���/Э�飬ֻ��Ҫ��һ��forǰ�����#pragma omp parallel xxxxx �Ϳ���
	//������ذѲ��л��Ĺ����Ӹ�������������ֱ6666666666666������
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

	int *pixel = new int[maxLength];//�洢ÿ�����ص�ĻҶ�
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
			//��Ȼ�͵�ǰ���ز������������ԭͼcopy���ع���
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
	float halfPixelWidth = float(maxWidth) / 2.0f;//��������Ĺ�һ��
	float halfPixelHeight = float(maxHeight) / 2.0f;
	int startIndex = param->startIndex;
	int endIndex = param->endIndex;
	float angle = param->radianAngle;//��ת�Ƕ�
	float scaleFactor = param->scaleFactor;

	//�������̾�����дpixel shader
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

		//����ͼƬ���˴�������ϵ��������Ч���ɡ����ȡ����Լ��ú�����
		float centeredRotatedPixelX =  -(centeredPixelX  * cosf(angle) - centeredPixelY * sinf(angle));
		float centeredRotatedPixelY =  -(centeredPixelX * sinf(angle) + centeredPixelY * cosf(angle));
		
		//fRotatedPixelX��Y�ĳ���scaleFactor������ķ���
		float fRotatedPixelX = (centeredRotatedPixelX + halfPixelWidth) / 2.0f / scaleFactor;
		float fRotatedPixelY = (halfPixelHeight - centeredRotatedPixelY) / 2.0f / scaleFactor;

		COLOR3 sampleColor;
		//�����ǰ������ת��û�г��磨��ζ�ſ��Բ�����
		if (fRotatedPixelX >= 0 && fRotatedPixelX < maxWidth-1 &&
			fRotatedPixelY >= 0 && fRotatedPixelY < maxHeight-1)
		{
			int rotatedPixelX = int(fRotatedPixelX);
			int rotatedPixelY = int(fRotatedPixelY);
			COLOR3 c1 = mFunction_GetPixel(param->pSrc, rotatedPixelX, rotatedPixelY);
			COLOR3 c2 = mFunction_GetPixel(param->pSrc, rotatedPixelX+1, rotatedPixelY);
			COLOR3 c3 = mFunction_GetPixel(param->pSrc, rotatedPixelX, rotatedPixelY+1);
			COLOR3 c4 = mFunction_GetPixel(param->pSrc, rotatedPixelX+1, rotatedPixelY+1);
			//��ֵϵ��
			float t1 = fRotatedPixelX - float(rotatedPixelX);
			float t2 = fRotatedPixelY - float(rotatedPixelY);

			//Hermite���ײ�ֵ
			auto Hermite = [](float t, const COLOR3& c1, const COLOR3& c2)->COLOR3
			{
				//hermite��ֵ�ľ����  : 2|x|^3 - 3|x|^2 +1���ѿ������ھ���˺�����ԭ��
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
			//����ĸ��ұ�ںð�
			sampleColor = COLOR3(0, 0, 0);
		}

		mFunction_SetPixel(param->pDest, x, y, sampleColor);
	}
	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_BICUBIC_FILTER_ROTATION, 1, NULL);
	return 0;
}

UINT ImageProcessor::mFunction_AutoLevels(LPVOID pThreadParam)
{
	//�������Զ�ɫ�ױ�����˵��߳��㷨...
	ThreadParam_AutoLevels* param = (ThreadParam_AutoLevels*)pThreadParam;
	int maxWidth = param->pSrc->GetWidth();
	int maxHeight = param->pSrc->GetHeight();

	/*
	�Զ�ɫ�׵��߼�:
	f x<x_min:  y=0.0;
	if x>x_max:  y=1.0;
	if x_min < x< x_max:   y=(x-x_min)/(x_max-x_min); 
	*/

	//ͳ������ֵʱ��������е�threshold
	const byte lowCut = 20;
	const byte highCut = 230;

	//ͳ��ֱ��ͼ
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


	//ӳ����ɫ����
	byte rangeR = maxR - minR;
	byte rangeG = maxG - minG;
	byte rangeB = maxB - minB;
#pragma omp parallel for num_threads(mThreadNum)
	for (int j = 0; j < maxHeight; ++j)
	{
		for (int i = 0; i < maxWidth; ++i)
		{
			COLOR3 c = mFunction_GetPixel(param->pSrc, i, j);

			//���ĳ��ͨ���Ķ�̬��Χ��0���ǻ���ʲôɫ�װ�
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

			//���
			mFunction_SetPixel(param->pDest,i, j, c);
		}
	}

	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_AUTO_LEVELS, 1, NULL);
	return 0;
}

UINT ImageProcessor::mFunction_AutoWhiteBalance(LPVOID pThreadParam)
{
	//�������Զ�ɫ�ױ�����˵��߳��㷨...
	ThreadParam_AutoLevels* param = (ThreadParam_AutoLevels*)pThreadParam;
	int maxWidth = param->pSrc->GetWidth();
	int maxHeight = param->pSrc->GetHeight();

	/*
	�Զ���ƽ����߼�:
	�Ҷ������㷨��Gray World)���ԻҶ��������Ϊ������,�ü�����Ϊ����һ�����Ŵ���ɫ�ʱ仯��ͼ��,
	R�� G�� B ����������ƽ��ֵ����ͬһ���Ҷ�K��һ�������ַ�����ȷ���ûҶȡ�
	(1)ֱ�Ӹ���Ϊ�̶�ֵ, ȡ���ͨ�����ֵ��һ��,��ȡΪ127��128��
	(2)�� K = (R_avg+G_avg+B_avg)/3,����R_aver,G_aver,B_aver�ֱ��ʾ�졢 �̡� ������ͨ����ƽ��ֵ��
	�㷨�ĵڶ����Ƿֱ�����ͨ�������棺
	Kr=K/R_avg;
	Kg=K/G_avg;
	Kb=K/B_avg;
	 �㷨������Ϊ����Von Kries �Խ�ģ��,����ͼ���е�ÿ������R��G��B����������ֵ��
	 Rnew = R * Kr;
	 Gnew = G * Kg;
	 Bnew = B * Kb;
	*/


	//��ͨ������ֵƽ��ֵ
	float avgR=0.0f, avgG=0.0f, avgB=0.0f;
	float K = 0;
	//��ɫ����
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

			//��ɫ��������������
			int r = int(c.r * Kr);
			int g = int(c.g * Kg);
			int b = int(c.b * Kb);
			if (r > 255)r = 255;
			if (g > 255)g = 255;
			if (b > 255)b = 255;
			c.r =r;
			c.g = g;
			c.b = b;

			//���
			mFunction_SetPixel(param->pDest, i, j, c);
		}
	}

	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_AUTO_WHITE_BALANCE, 1, NULL);
	return 0;
}

UINT ImageProcessor::mFunction_ImageBlending(LPVOID pThreadParam)
{
	//�������Զ�ɫ�ױ�����˵��߳��㷨...
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

		//���
		mFunction_SetPixel(param->pDest, x,y, c);
	}

	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_IMAGE_BLENDING, 1, NULL);
	return 0;
}

UINT ImageProcessor::mFunction_BilateralFilter(LPVOID pThreadParam)
{
	/*
	�������������ôһ���̼��ɣ����Ǹ�˹�˲���һ�ָĽ�
	�ۺ��˶������ֵ��������ÿһ��Ȩ�ؾ����ϵ��
	���忴��struct BilateralFilterMaskMatrix �����ע�Ͱ�
	*/
	ThreadParam_BilateralFilter* param = (ThreadParam_BilateralFilter*)pThreadParam;
	int maxWidth = param->pSrc->GetWidth();
	int maxHeight = param->pSrc->GetHeight();

	//�ƶ�Ȩֵ����ĳߴ�
	int stencilMatrixHalfW = BilateralFilterWeightMatrix::cMatrixHalfWidth;
	int stencilMatrixHalfH = BilateralFilterWeightMatrix::cMatrixHalfHeight;
	int stencilMatrixW = BilateralFilterWeightMatrix::cMatrixWidth;
	int stencilMatrixH = BilateralFilterWeightMatrix::cMatrixHeight;

	for (int i = param->startIndex; i <= param->endIndex; ++i)
	{
		int x = i % maxWidth;
		int y = i / maxWidth;

		//Ϊ�����Լ����һ�㣬�߽���������Ǵ�����Ե����˲���- -
		//���if��֤��ÿ����������ص�(i,j)�����Ժ��Ա߽�������ʹ��Ȩֵ����
		if (x >= stencilMatrixHalfW  && x < (maxWidth - stencilMatrixHalfW) &&
			y >= stencilMatrixHalfH  && y < (maxHeight - stencilMatrixHalfH))
		{
			//ÿһ�����ص�(i,j)���Ƕ�Ҫ��������˫���˲�Ȩ�ؾ���
			BilateralFilterWeightMatrix weightMatrixR;
			BilateralFilterWeightMatrix weightMatrixG;
			BilateralFilterWeightMatrix weightMatrixB;

			//����ÿ�μ���Ȩ�ؾ������Ƕ�Ҫ���㡰ֵ��ˡ�������ζ��ԭͼ�����һС
			//������������ض�Ҫ����ȥ...���鷳����
			//���Һ������Ƕ��ڡ��Ҷ�ͼ�����Ĳ���������Ҫһ����ͨ���ش����˰�
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

			//Ϊ�����(i,j)��ÿ��ͨ�����������ֵ
			float resultR = weightMatrixR.ComputeValue(param->sigma_d, param->sigma_r, imageRegionR);
			float resultG = weightMatrixG.ComputeValue(param->sigma_d, param->sigma_r, imageRegionG);
			float resultB = weightMatrixB.ComputeValue(param->sigma_d, param->sigma_r, imageRegionB);

			COLOR3 result = { byte(resultR),byte(resultG),byte(resultB) };
			mFunction_SetPixel(param->pDest, x, y, result);
		}
		else
		{
			//��Ե�����˲�- -ֱ��copy���Ƚ���
			COLOR3 c = mFunction_GetPixel(param->pSrc, x, y);
			mFunction_SetPixel(param->pDest, x, y, c);
		}
	}

	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_BILATERAL_FILTER, 1, NULL);
	return 0;
}
