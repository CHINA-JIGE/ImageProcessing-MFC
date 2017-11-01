
// ExperimentImgDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "ImageProcessor.h"


#define MAX_THREAD 8


/*struct DrawPara
{
	CImage* pImgSrc;
	CDC* pDC;
	int oriX;
	int oriY;
	int width;
	int height;
};*/

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
	CImage*		getImage() { return m_pImgSrc; }

	void					MedianFilter();

	void					AddNoise();

	void					RenderToPictureBox(CStatic* pPicBox, CImage* pImage);

	static UINT		Update(void* p);

	//void ThreadDraw(DrawPara *p);

	//void ImageCopy(CImage* pImgSrc, CImage* pImgDrt);

	afx_msg LRESULT OnMedianFilterThreadMsgReceived(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT OnNoiseThreadMsgReceived(WPARAM wParam, LPARAM lParam); 

protected:
	HICON m_hIcon;
	CImage * m_pImgSrc;
	int m_nThreadNum;
	ThreadParam* m_pThreadParam;
	CTime startTime;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:

	CEdit mEditInfo;
	CStatic mPictureControl;
	CButton m_CheckCirculation;

	afx_msg void OnBnClickedButtonOpen();
	afx_msg void OnCbnSelchangeComboFunction();
	afx_msg void OnNMCustomdrawSliderThreadnum(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonProcess();

};
