#pragma once
#pragma once

static const UINT cMaxThreadNum = 8;
static const UINT cMedianFilterMaxSpan = 15;

struct COLOR3
{
	COLOR3():r(0),g(0),b(0) {}

	COLOR3(byte _r, byte _g, byte _b):r(_r),g(_g),b(_b){}

	byte r;
	byte g;
	byte b;
};

struct ThreadParam
{
	CImage * pSrc;
	CImage* pDest;
	int startIndex;
	int endIndex;
	int maxSpan;//为模板中心到边缘的距离
};

struct ThreadParam_Rotation
{
	CImage * pSrc;
	CImage* pDest;
	int startIndex;
	int endIndex;
	float radianAngle;
};

static bool GetValue(int p[], int size, int &value);

class ImageProcessor
{
public:
	static void ImageCopy(CImage * pImgSrc, CImage * pImgDest);

	static UINT MedianFilter_WIN(CImage* pImgSrc,CImage* pImgDest,int numThreads);
	static UINT MedianFilter_OpenMP(CImage* pImgSrc, CImage* pImgDest, int numThreads);


	static UINT AddNoise_WIN(CImage* pImgSrc, CImage* pImgDest, int numThreads);
	static UINT AddNoise_OpenMP(CImage* pImgSrc, CImage* pImgDest, int numThreads);

	static UINT Rotate_WIN(CImage* pImgSrc, CImage* pImgDest, int numThreads, float radianAngle);
	static UINT Rotate_OpenMP(CImage* pImgSrc, CImage* pImgDest, int numThreads, float radianAngle);

	static UINT AutoColorGradation_WIN(CImage* pImgSrc, CImage* pImgDest, int numThreads);
	static UINT AutoColorGradation_OpenMP(CImage* pImgSrc, CImage* pImgDest, int numThreads);

	static UINT AutoWhiteBalance_WIN(CImage* pImgSrc, CImage* pImgDest, int numThreads);
	static UINT AutoWhiteBalance_OpenMP(CImage* pImgSrc, CImage* pImgDest, int numThreads);

	//图像融合
	static UINT ImageBlending_WIN(CImage* pImgSrc1, CImage* pImgSrc2, CImage* pImgDest, int numThreads,float alpha);
	static UINT ImageBlending_OpenMP(CImage* pImgSrc1, CImage* pImgSrc2, CImage* pImgDest, int numThreads, float alpha);

	//双边滤波
	static UINT BilateralFilter_BOOST(CImage* pImgSrc, CImage* pImgDest, int numThreads);

private:
	//template <int bytePerPixel>
	static void mFunction_SetPixel(CImage* pImage, int x, int y, COLOR3 color);

	///template <int bytesPerPixel>
	static COLOR3	mFunction_GetPixel(CImage* pImage, int x, int y);

	static UINT mFunction_MedianFilterForTargetRegion(LPVOID  pThreadParam);//单个线程执行的功能
	static UINT mFunction_AddNoiseForTargetRegion(LPVOID pThreadParam);
	static UINT mFunction_RotateForTargetRegion(LPVOID pThreadParam);


};