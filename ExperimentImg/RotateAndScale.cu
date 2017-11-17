/*
.cu文件是CUDA c++文件，多了一点奇怪的语法调用kernel函数（就是单个gpu线程的函数），

其中dispatch GPU threads的host函数实现在这个cu文件里，函数要加extern "C" 前缀

然后在ISO c++里面要写一个同样的extern "C"【声明】，并且可以在.cpp里面调用这个

.cu里面的host函数（算是CUDA的gpu端入口点吧）

*/
#pragma warning (disable : 4819)

#include <cuda_runtime.h>
#include <helper_functions.h>
#include <helper_cuda.h>

const int cBlockWidth = 16;
const int cBlockHeight = 16;
const int cBytePerPixel = 3;

__global__ void cudaKernel_RotateAndScale(const unsigned char* srcImage, unsigned char* destImage, int width, int height,int pitch,float angle,float scaleFactor)
{
	//blockIdx --- thread group index
	//thread index --- thread index with each thread group/block
	int pixelX = blockIdx.x * cBlockWidth + threadIdx.x;
	int pixelY = blockIdx.y * cBlockHeight + threadIdx.y;

	//********************************************************************
	float halfPixelWidth = float(width) / 2.0f;//用于坐标的归一化
	float halfPixelHeight = float(height) / 2.0f;
	//normalized X Y are mapped to [-1,1], centered at the center of screen
	float centeredPixelX = float(pixelX - halfPixelWidth) *(1.0f / scaleFactor);
	float centeredPixelY = float(halfPixelHeight - pixelY)*(1.0f / scaleFactor);
	/*
	[cos	sin]	[x]
	[-sin	cos]	[y]
	*/

	//缩放图片，此处的缩放系数和缩放效果成【反比】，自己好好想想
	float centeredRotatedPixelX = -(centeredPixelX  * cos(-angle) - centeredPixelY * sin(-angle));
	float centeredRotatedPixelY = -(centeredPixelX * sin(-angle) + centeredPixelY * cos(-angle));

	//fRotatedPixelX和Y的除以scaleFactor是纹理的放缩
	float fRotatedPixelX = (centeredRotatedPixelX + halfPixelWidth) / 2.0f / scaleFactor;
	float fRotatedPixelY = (halfPixelHeight - centeredRotatedPixelY) / 2.0f / scaleFactor;

	//如果当前像素旋转后没有出界（意味着可以采样）
	unsigned char sampleColor[3];
	if (fRotatedPixelX >= 0 && fRotatedPixelX < width - 1 &&
		fRotatedPixelY >= 0 && fRotatedPixelY < height - 1)
	{
		int rotatedPixelX = int(fRotatedPixelX);
		int rotatedPixelY = int(fRotatedPixelY);
		unsigned char c1[3], c2[3], c3[3], c4[3];//4个RGB color

		for (int i = 0; i < 3; ++i)c1[i] = srcImage[(rotatedPixelY +0) * pitch + (rotatedPixelX +0)* cBytePerPixel + 2 - i];
		for (int i = 0; i < 3; ++i)c2[i] = srcImage[(rotatedPixelY +0) * pitch + (rotatedPixelX +1) * cBytePerPixel + 2 - i];
		for (int i = 0; i < 3; ++i)c3[i] = srcImage[(rotatedPixelY +1) * pitch + (rotatedPixelX +0) * cBytePerPixel + 2 - i];
		for (int i = 0; i < 3; ++i)c4[i] = srcImage[(rotatedPixelY +1) * pitch + (rotatedPixelX +1) * cBytePerPixel + 2 - i];

		//插值系数
		float t1 = fRotatedPixelX - float(rotatedPixelX);
		float t2 = fRotatedPixelY - float(rotatedPixelY);

		//Hermite三阶插值
		auto Hermite = [](float t, const unsigned char c1[3], const unsigned char c2[3],unsigned char c3[3] )
		{
			//hermite插值的卷积核  : 2|x|^3 - 3|x|^2 +1，把考察点放在卷积核函数的原点
			float factor1 = 2.0f * t * t * t - 3.0f * t * t + 1.0f;
			float factor2 = 2.0f * (1.0f - t) * (1.0f - t) * (1.0f - t) - 3.0f * (1.0f - t) * (1.0f - t) + 1.0f;
			c3[0] = c1[0] * factor1 + c2[0]*factor2;
			c3[1] = c1[1] * factor1 + c2[1]*factor2;
			c3[2] = c1[2] * factor1 + c2[2]*factor2;
		};

		unsigned char tmp1[3], tmp2[3];
		Hermite(t1, c1, c2, tmp1);
		Hermite(t1, c3, c4, tmp2);

		Hermite(t2, tmp1, tmp2, sampleColor);
	}
	else
	{
		//出界的给我变黑好吧
		sampleColor[0] = 0;
		sampleColor[1] = 0;
		sampleColor[2] = 0;
	}


	//********************************************************************
	//输出
	destImage[pixelY * pitch + pixelX *cBytePerPixel + 2] = sampleColor[0];//r
	destImage[pixelY * pitch + pixelX *cBytePerPixel + 1] = sampleColor[1];//g
	destImage[pixelY * pitch + pixelX *cBytePerPixel + 0] = sampleColor[2];//b
}

/*entry point of CUDA*/
extern "C" int cudaHost_RotateAndScale(const unsigned char* srcImage, unsigned char* destImage, int width,int height, int pitch, float angle, float scaleFactor)
{
	cudaError_t err = cudaSuccess;

	//（呃不知道这个有什么用）
	cudaSetDevice(0);

	//分配显存，显存块尺寸要和内存中的一样
	unsigned char* pDeviceSrcData =nullptr;
	unsigned char* pDeviceDestData = nullptr;

	//MFC的CImage的pitch居然是负的！！！！！！！！！！！！
	//敢再反人类一点吗？？？
	int imageByteSize = height * pitch;

	cudaMalloc((void**)&pDeviceSrcData, imageByteSize);
	err = cudaGetLastError();
	if (err != cudaSuccess)return 11;

	cudaMalloc((void**)&pDeviceDestData, imageByteSize);
	err = cudaGetLastError();
	if (err != cudaSuccess)return 12;

	//内存的数据update到显存
	cudaMemcpy(pDeviceSrcData, srcImage, imageByteSize , cudaMemcpyHostToDevice);
	err = cudaGetLastError();
	if (err != cudaSuccess)return 13;

	//大规模调用核函数(每个gpu thread都要执行kernel function)
	//其中在kernel function里面，src data执行完逻辑就写到Dest data 里面
	//三个尖括号 ： <<<blocksPerGrid, threadsPerBlock>>>

	dim3 dimGrid(width / cBlockWidth, height / cBlockHeight);	//网格(grid)的维度，grid的单元是线程组
	dim3 dimBlock(cBlockWidth, cBlockHeight);	//线程组(Thread Group)/块(block)的尺寸
	cudaKernel_RotateAndScale << <dimGrid, dimBlock >> >
		(pDeviceSrcData, pDeviceDestData,width,height,pitch,angle,scaleFactor);
	err = cudaGetLastError();
	if (err != cudaSuccess)return 14;

	//gpu threads的同步等待
	cudaDeviceSynchronize();
	err = cudaGetLastError();
	if (err != cudaSuccess)return 15;

	//获取结果，copy回内存
	cudaMemcpy(destImage, pDeviceDestData, imageByteSize,  cudaMemcpyDeviceToHost);
	err = cudaGetLastError();
	if (err != cudaSuccess)return 16;

	//??不知道干嘛的reset device??
	cudaDeviceReset();
	err = cudaGetLastError();
	if (err != cudaSuccess)return 16;

	return 0;
}