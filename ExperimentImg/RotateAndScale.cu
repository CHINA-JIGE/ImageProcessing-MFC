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

__global__ void cudaKernel_RotateAndScale(const unsigned char* srcImage, unsigned char* destImage, int width, int height,int pitch,float angle,float scaleFactor)
{
	//blockIdx --- thread group index
	//thread index --- thread index with each thread group/block
	int pixelX = blockIdx.x * cBlockWidth + threadIdx.x;
	int pixelY = blockIdx.y * cBlockHeight + threadIdx.y;

	//********************************************************************
	float halfPixelWidth = float(width) / 2.0f;//��������Ĺ�һ��
	float halfPixelHeight = float(height) / 2.0f;
	//normalized X Y are mapped to [-1,1], centered at the center of screen
	float centeredPixelX = float(pixelX - halfPixelWidth) *(1.0f / scaleFactor);
	float centeredPixelY = float(halfPixelHeight - pixelY)*(1.0f / scaleFactor);
	/*
	[cos	sin]	[x]
	[-sin	cos]	[y]
	*/

	//����ͼƬ���˴�������ϵ��������Ч���ɡ����ȡ����Լ��ú�����
	float centeredRotatedPixelX = -(centeredPixelX  * cos(-angle) - centeredPixelY * sin(-angle));
	float centeredRotatedPixelY = -(centeredPixelX * sin(-angle) + centeredPixelY * cos(-angle));

	//fRotatedPixelX��Y�ĳ���scaleFactor������ķ���
	float fRotatedPixelX = (centeredRotatedPixelX + halfPixelWidth) / 2.0f / scaleFactor;
	float fRotatedPixelY = (halfPixelHeight - centeredRotatedPixelY) / 2.0f / scaleFactor;

	//�����ǰ������ת��û�г��磨��ζ�ſ��Բ�����
	unsigned char sampleColor[3];
	if (fRotatedPixelX >= 0 && fRotatedPixelX < width - 1 &&
		fRotatedPixelY >= 0 && fRotatedPixelY < height - 1)
	{
		int rotatedPixelX = int(fRotatedPixelX);
		int rotatedPixelY = int(fRotatedPixelY);
		unsigned char c1[3], c2[3], c3[3], c4[3];//4��RGB color

		for (int i = 0; i < 3; ++i)c1[i] = srcImage[(rotatedPixelY +0) * pitch + (rotatedPixelX +0)* cBytePerPixel + 2 - i];
		for (int i = 0; i < 3; ++i)c2[i] = srcImage[(rotatedPixelY +0) * pitch + (rotatedPixelX +1) * cBytePerPixel + 2 - i];
		for (int i = 0; i < 3; ++i)c3[i] = srcImage[(rotatedPixelY +1) * pitch + (rotatedPixelX +0) * cBytePerPixel + 2 - i];
		for (int i = 0; i < 3; ++i)c4[i] = srcImage[(rotatedPixelY +1) * pitch + (rotatedPixelX +1) * cBytePerPixel + 2 - i];

		//��ֵϵ��
		float t1 = fRotatedPixelX - float(rotatedPixelX);
		float t2 = fRotatedPixelY - float(rotatedPixelY);

		//Hermite���ײ�ֵ
		auto Hermite = [](float t, const unsigned char c1[3], const unsigned char c2[3],unsigned char c3[3] )
		{
			//hermite��ֵ�ľ����  : 2|x|^3 - 3|x|^2 +1���ѿ������ھ���˺�����ԭ��
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
		//����ĸ��ұ�ںð�
		sampleColor[0] = 0;
		sampleColor[1] = 0;
		sampleColor[2] = 0;
	}


	//********************************************************************
	//���
	destImage[pixelY * pitch + pixelX *cBytePerPixel + 2] = sampleColor[0];//r
	destImage[pixelY * pitch + pixelX *cBytePerPixel + 1] = sampleColor[1];//g
	destImage[pixelY * pitch + pixelX *cBytePerPixel + 0] = sampleColor[2];//b
}

/*entry point of CUDA*/
extern "C" int cudaHost_RotateAndScale(const unsigned char* srcImage, unsigned char* destImage, int width,int height, int pitch, float angle, float scaleFactor)
{
	cudaError_t err = cudaSuccess;

	//������֪�������ʲô�ã�
	cudaSetDevice(0);

	//�����Դ棬�Դ��ߴ�Ҫ���ڴ��е�һ��
	unsigned char* pDeviceSrcData =nullptr;
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
	cudaMemcpy(pDeviceSrcData, srcImage, imageByteSize , cudaMemcpyHostToDevice);
	err = cudaGetLastError();
	if (err != cudaSuccess)return 13;

	//���ģ���ú˺���(ÿ��gpu thread��Ҫִ��kernel function)
	//������kernel function���棬src dataִ�����߼���д��Dest data ����
	//���������� �� <<<blocksPerGrid, threadsPerBlock>>>

	dim3 dimGrid(width / cBlockWidth, height / cBlockHeight);	//����(grid)��ά�ȣ�grid�ĵ�Ԫ���߳���
	dim3 dimBlock(cBlockWidth, cBlockHeight);	//�߳���(Thread Group)/��(block)�ĳߴ�
	cudaKernel_RotateAndScale << <dimGrid, dimBlock >> >
		(pDeviceSrcData, pDeviceDestData,width,height,pitch,angle,scaleFactor);
	err = cudaGetLastError();
	if (err != cudaSuccess)return 14;

	//gpu threads��ͬ���ȴ�
	cudaDeviceSynchronize();
	err = cudaGetLastError();
	if (err != cudaSuccess)return 15;

	//��ȡ�����copy���ڴ�
	cudaMemcpy(destImage, pDeviceDestData, imageByteSize,  cudaMemcpyDeviceToHost);
	err = cudaGetLastError();
	if (err != cudaSuccess)return 16;

	//??��֪�������reset device??
	cudaDeviceReset();
	err = cudaGetLastError();
	if (err != cudaSuccess)return 16;

	return 0;
}