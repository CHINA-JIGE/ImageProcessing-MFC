/*
.cu�ļ���CUDA c++�ļ�������һ����ֵ��﷨����kernel���������ǵ���gpu�̵߳ĺ�������

����dispatch GPU threads��host����ʵ�������cu�ļ������Ҫ��extern "C" ǰ׺

Ȼ����ISO c++����Ҫдһ��ͬ����extern "C"�������������ҿ�����.cpp����������

.cu�����host����������CUDA��gpu����ڵ�ɣ�

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
	//���
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

	//������֪�������ʲô�ã�
	cudaSetDevice(0);

	//�����Դ棬�Դ��ߴ�Ҫ���ڴ��е�һ��
	unsigned char* pDeviceSrcData = nullptr;
	unsigned char* pDeviceDestData = nullptr;

	//MFC��CImage��pitch��Ȼ�Ǹ��ģ�����������������������
	//���ٷ�����һ���𣿣���
	int imageByteSize = height * pitch;

	cudaMalloc((void**)&pDeviceSrcData, imageByteSize);
	err = cudaGetLastError();
	if (err != cudaSuccess)return 11;

	cudaMalloc((void**)&pDeviceDestData, imageByteSize);
	err = cudaGetLastError();
	if (err != cudaSuccess)return 12;

	//�ڴ������update���Դ�
	cudaMemcpy(pDeviceSrcData, srcImage, imageByteSize, cudaMemcpyHostToDevice);
	err = cudaGetLastError();
	if (err != cudaSuccess)return 13;

	//���ģ���ú˺���(ÿ��gpu thread��Ҫִ��kernel function)
	//������kernel function���棬src dataִ�����߼���д��Dest data ����
	//���������� �� <<<blocksPerGrid, threadsPerBlock>>>

	dim3 dimGrid(width / cBlockWidth, height / cBlockHeight);	//����(grid)��ά�ȣ�grid�ĵ�Ԫ���߳���
	dim3 dimBlock(cBlockWidth, cBlockHeight);	//�߳���(Thread Group)/��(block)�ĳߴ�
	cudaKernel_AutoLevels << <dimGrid, dimBlock >> >
		(pDeviceSrcData, pDeviceDestData, pitch, minR, maxR, minG, maxG, minB, maxB);
	err = cudaGetLastError();
	if (err != cudaSuccess)return 14;

	//gpu threads��ͬ���ȴ�
	cudaDeviceSynchronize();
	err = cudaGetLastError();
	if (err != cudaSuccess)return 15;

	//��ȡ�����copy���ڴ�
	cudaMemcpy(destImage, pDeviceDestData, imageByteSize, cudaMemcpyDeviceToHost);
	err = cudaGetLastError();
	if (err != cudaSuccess)return 16;

	//??��֪�������reset device??
	cudaDeviceReset();
	err = cudaGetLastError();
	if (err != cudaSuccess)return 16;

	return 0;
}