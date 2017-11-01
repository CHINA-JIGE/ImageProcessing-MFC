#pragma once
#pragma once
#define NOISE 0.2
struct ThreadParam
{
	CImage * src;
	int startIndex;
	int endIndex;
	int maxSpan;//Ϊģ�����ĵ���Ե�ľ���
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
	static UINT mFunction_MedianFilterForTargetRegion(LPVOID  pThreadParam);//�����߳�ִ�еĹ���
	static UINT mFunction_AddNoiseForTargetRegion(LPVOID param);

	static const UINT cMedianFilterMaxSpan = 15;
};