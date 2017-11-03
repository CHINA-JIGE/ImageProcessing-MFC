
// ExperimentImgDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "ImageProcessor.h"


// CExperimentImgDlg 对话框
class CExperimentImgDlg : public CDialogEx
{
// 构造
public:
	CExperimentImgDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_EXPERIMENTIMG_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
	CImage*	getImage() { return m_pImage1; }


	void	RenderToPictureBox(CStatic* pPicBox, CImage* pImage);

	afx_msg LRESULT OnThreadMsgReceived_MedianFilter(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT OnThreadMsgReceived_AddNoise(WPARAM wParam, LPARAM lParam); 

	afx_msg LRESULT OnThreadMsgReceived_RotationWithBicubicFilter(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT OnThreadMsgReceived_AutoColorGradation(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT OnThreadMsgReceived_WhiteBalance(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT OnThreadMsgReceived_ImageBlending(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT OnThreadMsgReceived_BilateralFilter(WPARAM wParam, LPARAM lParam);

private:

	void		mFunction_MedianFilter();

	void		mFunction_AddNoise();

	void		mFunction_Rotate(float angle);

	void		mFunction_AutoColorGradation();

	void		mFunction_AutoWhiteBalance();

	void		mFunction_ImageBlending();

	void		mFunction_BilateralFilter();

protected:
	HICON			m_hIcon;
	int				mThreadNum;//要使用的线程数
	CTime			mStartTime;
	CImage *		m_pImage1;
	CImage*		m_pImage2;
	CImage*		m_pImageResult;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:

	//默认的图像缓冲区(CImage)的大小
	static const int cPictureBufferWidth = 640;
	static const int cPictureBufferHeight = 480;

	CEdit mEditInfo;
	CButton mButton_OpenFile1;
	CButton mButton_OpenFile2;
	CButton m_CheckBox_Circulate;
	CStatic mPictureBox1;
	CStatic mPictureBox2;
	CStatic mPictureBox_Result;

	afx_msg void OnBnClickedButtonOpen();
	afx_msg void OnBnClickedButtonOpen2();
	afx_msg void OnCbnSelchangeComboFunction();
	afx_msg void OnNMCustomdrawSliderThreadnum(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonProcess();
	afx_msg void OnBnClickedButton1();
};
