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

typedef unsigned char byte;

__global__ void cudaKernel_AutoLevels(
	const unsigned char* srcImage, unsigned char* destImage, int pitch,
	byte minR, byte maxR,
	byte minG, byte maxG,
	byte minB, byte maxB
	)
{
	//blockIdx --- thread group index
	//thread index --- thread index with each thread group/block
	int pixelX = blockIdx.x * cBlockWidth + threadIdx.x;
	int pixelY = blockIdx.y * cBlockHeight + threadIdx.y;

	//********************************************************************
	byte c[3];
	c[0] = srcImage[pixelY * pitch + pixelX* cBytePerPixel + 2];
	c[1] = srcImage[pixelY * pitch + pixelX* cBytePerPixel + 1];
	c[2] = srcImage[pixelY * pitch + pixelX* cBytePerPixel + 0];

	byte rangeR = maxR - minR;
	byte rangeG = maxG - minG;
	byte rangeB = maxB - minB;

	if (rangeR != 0)
	{
		float ratioR = float(c[0] - minR) / rangeR;
		if (ratioR >= 0.0f && ratioR <= 1.0f) c[0] = byte(255.0f * ratioR);
	}

	if (rangeG != 0)
	{
		float ratioG = float(c[1] - minG) / rangeG;
		if (ratioG >= 0.0f && ratioG <= 1.0f)c[1] = byte(255.0f * ratioG);
	}

	if (rangeB != 0)
	{
		float ratioB = float(c[2] - minB) / rangeB;
		if (ratioB >= 0.0f && ratioB <= 1.0f)c[2] = byte(255.0f * ratioB);
	}

	//********************************************************************
	//输出
	destImage[pixelY * pitch + pixelX *cBytePerPixel + 2] = c[0];//r
	destImage[pixelY * pitch + pixelX *cBytePerPixel + 1] = c[1];//g
	destImage[pixelY * pitch + pixelX *cBytePerPixel + 0] = c[2];//b
}

/*entry point of CUDA*/
extern "C" int cudaHost_AutoLevels(const unsigned char* srcImage, unsigned char* destImage, 
	int width, int height, int pitch,
	byte minR, byte maxR,
	byte minG, byte maxG,
	byte minB, byte maxB
	)
{
	cudaError_t err = cudaSuccess;

	//（呃不知道这个有什么用）
	cudaSetDevice(0);

	//分配显存，显存块尺寸要和内存中的一样
	unsigned char* pDeviceSrcData = nullptr;
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
	cudaMemcpy(pDeviceSrcData, srcImage, imageByteSize, cudaMemcpyHostToDevice);
	err = cudaGetLastError();
	if (err != cudaSuccess)return 13;

	//大规模调用核函数(每个gpu thread都要执行kernel function)
	//其中在kernel function里面，src data执行完逻辑就写到Dest data 里面
	//三个尖括号 ： <<<blocksPerGrid, threadsPerBlock>>>

	dim3 dimGrid(width / cBlockWidth, height / cBlockHeight);	//网格(grid)的维度，grid的单元是线程组
	dim3 dimBlock(cBlockWidth, cBlockHeight);	//线程组(Thread Group)/块(block)的尺寸
	cudaKernel_AutoLevels << <dimGrid, dimBlock >> >
		(pDeviceSrcData, pDeviceDestData, pitch, minR, maxR, minG, maxG, minB, maxB);
	err = cudaGetLastError();
	if (err != cudaSuccess)return 14;

	//gpu threads的同步等待
	cudaDeviceSynchronize();
	err = cudaGetLastError();
	if (err != cudaSuccess)return 15;

	//获取结果，copy回内存
	cudaMemcpy(destImage, pDeviceDestData, imageByteSize, cudaMemcpyDeviceToHost);
	err = cudaGetLastError();
	if (err != cudaSuccess)return 16;

	//??不知道干嘛的reset device??
	cudaDeviceReset();
	err = cudaGetLastError();
	if (err != cudaSuccess)return 16;

	return 0;
}