#include <vector>
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

struct BilateralFilterWeightMatrix
{
	//������ı߳�Ϊ2n+1
	static const  int	cMatrixHalfWidth	=	6;
	static const int		cMatrixHalfHeight =	6;
	static const int		cMatrixWidth = 2 * cMatrixHalfWidth + 1;
	static const int		cMatrixHeight = 2 * cMatrixHalfHeight + 1;
	
	//һ��cMatrixWidth x cMatrixHeight�������ܼ�ȨӰ�쵽ĳ������������
	//����Ҫ����һ���Ľ���Ļ������ϸ�˹�ֲ���ϵ��Ȩ�ؾ�����Ҫ���������Զ��Ĳ���
	//��������˹�ֲ���������״...

	//�ο����ϣ�
	//http://www.bubuko.com/infodetail-927606.html

	/*note:����ĳ������Ϊ(i,j)�����ص㣬����Ҫ������˫���˲�֮��Ľ��ֵ����Ҫ����Χ������
	��һ��k*l�����ؾ���
	��ʽΪ g(i,j) = (k*l���������ֵ��Ȩ��) / (k*l����Ȩֵ�ĺ�) 
	��ĸ���µĶ��������Ǹ�k*l�ġ�С���ڡ�

	����Ȩֵ����ô��ģ�
	Ȩֵ���������ÿ��Ȩ��ϵ�������������ĳ˻���
	��һ�����ǡ�������ˡ� d(i,j,k,l) = exp( -(  (i-k)^2 + (j-l)^2)/ (2*sigma_d^2) ) ),
	���Ӧ�þ�����sigma_dΪϵ���Ķ�ά��˹�ֲ���ϵ��
	�ڶ������ǡ�ֵ��ˡ�r(i,j,k,l) = exp ( -( f(i,j)��(f(i+k,j+l)��ģ )^2 )/ 2(sigma_r ^2) )
	���Ӧ�þ���˫���˲��Ը�˹�˲��ĸĽ���Ȩ�ؿ�����ĳ����ֵ�����ĵ�����ֵ�ġ����롱��
	����ʵ��ν��ɫ�ľ���͵�����ŷʽ����ͺ��ˡ�

	����imageRegion������������ֵ��˵ģ�����Ҫ������һС��������ؾ���
	*/


	float ComputeWeight(int k, int l, byte colorDifference, float sigma_d, float sigma_r)
	{
		//Ȩֵ�����ĵ�һ������������ˣ����˹�ֲ������ؼ�����й�
		float gaussianWeight =  exp(-0.5f * float(k*k + l*l) / (sigma_d * sigma_d));

		//Ȩֵ�����ĵڶ�������ֵ��ˣ���Ҷ���ɫ�Ĳ�ֵ�й�
		float colorDistanceWeight = exp(-0.5f * float(colorDifference * colorDifference) / (sigma_r * sigma_r));

		//��������Ż������ǰ�exp������ϵ���ϲ�����һ��exp����������Ϊ�������ȷֿ�����
		return gaussianWeight * colorDistanceWeight;
	}

	//�������ս��
	float ComputeValue(float sigma_d, float sigma_r, std::vector<byte>& imageRegion)
	{
		float sumOfWeights = 0.0f;//Ȩֵ��
		float sumOfWeightedColor = 0.0f;//��ɫ��Ȩ��

		//�����(i,j)��(�Ҷ�)��ɫֵ
		int centerPixelValue = imageRegion[cMatrixWidth * cMatrixHalfHeight + cMatrixHalfWidth];
		for (int l = 0; l < cMatrixHeight; ++l)
		{
			for (int k = 0; k < cMatrixWidth; ++k)
			{
				//��ǰ���(С������)��ɫֵ
				int currentPixelValue = imageRegion[cMatrixWidth *l + k];
				byte colorDiff = abs(centerPixelValue - currentPixelValue);

				float weight = ComputeWeight(k, l, colorDiff, sigma_d, sigma_r);

				//���
				sumOfWeights += weight;
				sumOfWeightedColor += (weight * float(currentPixelValue));
			}
		}

		return sumOfWeightedColor / sumOfWeights;
	}
};

struct ThreadParam
{
	CImage * pSrc;
	CImage* pDest;
	int startIndex;
	int endIndex;
	int maxSpan;//Ϊģ�����ĵ���Ե�ľ���
};

struct ThreadParam_Rotation
{
	CImage * pSrc;
	CImage* pDest;
	int startIndex;
	int endIndex;
	float radianAngle;
	float scaleFactor;
};

struct ThreadParam_AutoLevels
{
	CImage * pSrc;
	CImage* pDest;
};

struct ThreadParam_AutoWhiteBalance
{
	CImage * pSrc;
	CImage* pDest;
};

struct ThreadParam_ImageBlending
{
	CImage* pSrc1;
	CImage* pSrc2;
	CImage* pDest;
	int startIndex;
	int endIndex;
	float alpha;
};

struct ThreadParam_BilateralFilter
{
	CImage* pSrc;
	CImage* pDest;
	int startIndex;
	int endIndex;
	float sigma_d;//˫���˲���һ���Զ�������һ��1~10
	float sigma_r;//˫���˲���һ���Զ�������һ��10~300
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

	static UINT Rotate_WIN(CImage* pImgSrc, CImage* pImgDest, int numThreads, float radianAngle, float scaleFactor);
	static UINT Rotate_OpenMP(CImage* pImgSrc, CImage* pImgDest, int numThreads, float radianAngle, float scaleFactor);

	static UINT AutoLevels_OpenMP(CImage* pImgSrc, CImage* pImgDest, int numThreads);

	static UINT AutoWhiteBalance_OpenMP(CImage* pImgSrc, CImage* pImgDest, int numThreads);

	//ͼ���ں�
	static UINT ImageBlending_WIN(CImage* pImgSrc1, CImage* pImgSrc2, CImage* pImgDest, int numThreads,float alpha);
	static UINT ImageBlending_OpenMP(CImage* pImgSrc1, CImage* pImgSrc2, CImage* pImgDest, int numThreads, float alpha);

	//˫���˲�
	static UINT BilateralFilter_BOOST(CImage* pImgSrc, CImage* pImgDest, int numThreads);

private:
	//template <int bytePerPixel>
	static void mFunction_SetPixel(CImage* pImage, int x, int y,const COLOR3& color);

	//template <int bytesPerPixel>
	static COLOR3	mFunction_GetPixel(CImage* pImage, int x, int y);

	static UINT mFunction_MedianFilterForTargetRegion(LPVOID  pThreadParam);//�����߳�ִ�еĹ���

	static UINT mFunction_AddNoiseForTargetRegion(LPVOID pThreadParam);

	static UINT mFunction_RotateForTargetRegion(LPVOID pThreadParam);

	static UINT mFunction_AutoLevels(LPVOID pThreadParam);

	static UINT mFunction_AutoWhiteBalance(LPVOID pThreadParam);

	static UINT mFunction_ImageBlending(LPVOID pThreadParam);

	static UINT mFunction_BilateralFilter(LPVOID pThreadParam);
};