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
	//则区域的边长为2n+1
	static const  int	cMatrixHalfWidth	=	6;
	static const int		cMatrixHalfHeight =	6;
	static const int		cMatrixWidth = 2 * cMatrixHalfWidth + 1;
	static const int		cMatrixHeight = 2 * cMatrixHalfHeight + 1;
	
	//一共cMatrixWidth x cMatrixHeight个像素能加权影响到某个被处理像素
	//所以要计算一个改进版的基本符合高斯分布的系数权重矩阵，需要传入两个自定的参数
	//来决定高斯分布函数的形状...

	//参考资料：
	//http://www.bubuko.com/infodetail-927606.html

	/*note:对于某个坐标为(i,j)的像素点，我们要计算它双边滤波之后的结果值，就要考察围绕着它
	的一个k*l的像素矩阵。
	公式为 g(i,j) = (k*l矩阵的像素值加权和) / (k*l矩阵权值的和) 
	分母上下的定义域都是那个k*l的“小窗口”

	其中权值是这么求的：
	权值矩阵里面的每个权重系数都是两个量的乘积。
	第一个量是“定义域核” d(i,j,k,l) = exp( -(  (i-k)^2 + (j-l)^2)/ (2*sigma_d^2) ) ),
	这个应该就是以sigma_d为系数的二维高斯分布的系数
	第二个量是“值域核”r(i,j,k,l) = exp ( -( f(i,j)与(f(i+k,j+l)的模 )^2 )/ 2(sigma_r ^2) )
	这个应该就是双边滤波对高斯滤波的改进，权重考虑了某像素值与中心点像素值的“距离”，
	那其实所谓颜色的距离就当作是欧式距离就好了。

	参数imageRegion就是用来计算值域核的，所以要输入这一小区域的像素矩阵
	*/


	float ComputeWeight(int k, int l, byte colorDifference, float sigma_d, float sigma_r)
	{
		//权值函数的第一个量：定义域核，与高斯分布和像素间距离有关
		float gaussianWeight =  exp(-0.5f * float(k*k + l*l) / (sigma_d * sigma_d));

		//权值函数的第二个量：值域核，与灰度颜色的差值有关
		float colorDistanceWeight = exp(-0.5f * float(colorDifference * colorDifference) / (sigma_r * sigma_r));

		//这个可以优化，就是把exp的两项系数合并，少一次exp，但是这里为了清晰先分开两项
		return gaussianWeight * colorDistanceWeight;
	}

	//计算最终结果
	float ComputeValue(float sigma_d, float sigma_r, std::vector<byte>& imageRegion)
	{
		float sumOfWeights = 0.0f;//权值和
		float sumOfWeightedColor = 0.0f;//颜色加权和

		//输出点(i,j)的(灰度)颜色值
		int centerPixelValue = imageRegion[cMatrixWidth * cMatrixHalfHeight + cMatrixHalfWidth];
		for (int l = 0; l < cMatrixHeight; ++l)
		{
			for (int k = 0; k < cMatrixWidth; ++k)
			{
				//当前点的(小矩阵内)颜色值
				int currentPixelValue = imageRegion[cMatrixWidth *l + k];
				byte colorDiff = abs(centerPixelValue - currentPixelValue);

				float weight = ComputeWeight(k, l, colorDiff, sigma_d, sigma_r);

				//求和
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
	int maxSpan;//为模板中心到边缘的距离
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
	float sigma_d;//双边滤波的一个自定参数，一般1~10
	float sigma_r;//双边滤波的一个自定参数，一般10~300
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

	//图像融合
	static UINT ImageBlending_WIN(CImage* pImgSrc1, CImage* pImgSrc2, CImage* pImgDest, int numThreads,float alpha);
	static UINT ImageBlending_OpenMP(CImage* pImgSrc1, CImage* pImgSrc2, CImage* pImgDest, int numThreads, float alpha);

	//双边滤波
	static UINT BilateralFilter_BOOST(CImage* pImgSrc, CImage* pImgDest, int numThreads);

private:
	//template <int bytePerPixel>
	static void mFunction_SetPixel(CImage* pImage, int x, int y,const COLOR3& color);

	//template <int bytesPerPixel>
	static COLOR3	mFunction_GetPixel(CImage* pImage, int x, int y);

	static UINT mFunction_MedianFilterForTargetRegion(LPVOID  pThreadParam);//单个线程执行的功能

	static UINT mFunction_AddNoiseForTargetRegion(LPVOID pThreadParam);

	static UINT mFunction_RotateForTargetRegion(LPVOID pThreadParam);

	static UINT mFunction_AutoLevels(LPVOID pThreadParam);

	static UINT mFunction_AutoWhiteBalance(LPVOID pThreadParam);

	static UINT mFunction_ImageBlending(LPVOID pThreadParam);

	static UINT mFunction_BilateralFilter(LPVOID pThreadParam);
};