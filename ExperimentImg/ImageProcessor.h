#pragma once
#pragma once
#define NOISE 0.2
struct ThreadParam
{
	CImage * src;
	int startIndex;
	int endIndex;
	int maxSpan;//为模板中心到边缘的距离
};

static bool GetValue(int p[], int size, int &value);

class ImageProcessor
{
public:
	static UINT MedianFilter_WIN(CImage* pImage,int numThreads,ThreadParam* pParamArray);
	static UINT MedianFilter_OpenMP(CImage* pImage, int numThreads, ThreadParam* pParamArray);


	static UINT AddNoise_WIN(CImage* pImage, int numThreads, ThreadParam* pParamArray);
	static UINT AddNoise_OpenMP(CImage* pImage, int numThreads, ThreadParam* pParamArray);

private:
	static UINT mFunction_MedianFilterForTargetRegion(LPVOID  pThreadParam);//单个线程执行的功能
	static UINT mFunction_AddNoiseForTargetRegion(LPVOID param);

	static const UINT cMedianFilterMaxSpan = 15;
};