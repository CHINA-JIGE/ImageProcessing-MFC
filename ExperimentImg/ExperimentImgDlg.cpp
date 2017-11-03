
#include "stdafx.h"
#include "ExperimentImg.h"
#include "ExperimentImgDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CExperimentImgDlg �Ի���
CExperimentImgDlg::CExperimentImgDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_EXPERIMENTIMG_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	//���ضԻ����ʱ���ʼ��
	mThreadNum = 1;
	srand(time(0));
}

void CExperimentImgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//	DDX_Control(pDX, IDC_EDIT_INFO, mEditInfo);
	DDX_Control(pDX, IDC_PICTURE, mPictureBox1);
	DDX_Control(pDX, IDC_CHECK_100, m_CheckBox_Circulate);
	DDX_Control(pDX, IDC_BUTTON_OPEN2, mButton_OpenFile2);
	DDX_Control(pDX, IDC_PICTURE2, mPictureBox2);
	DDX_Control(pDX, IDC_PICTURE3, mPictureBox_Result);
	DDX_Control(pDX, IDC_BUTTON_OPEN, mButton_OpenFile1);
}

BEGIN_MESSAGE_MAP(CExperimentImgDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &CExperimentImgDlg::OnBnClickedButtonOpen)
	ON_CBN_SELCHANGE(IDC_COMBO_FUNCTION, &CExperimentImgDlg::OnCbnSelchangeComboFunction)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_THREADNUM, &CExperimentImgDlg::OnNMCustomdrawSliderThreadnum)
	ON_BN_CLICKED(IDC_BUTTON_PROCESS, &CExperimentImgDlg::OnBnClickedButtonProcess)
	ON_BN_CLICKED(IDC_BUTTON_OPEN2, &CExperimentImgDlg::OnBnClickedButtonOpen2)
	//�Զ���Ķ��߳̽���֪ͨ�Ļص�����
	ON_MESSAGE(WM_NOISE, &CExperimentImgDlg::OnThreadMsgReceived_AddNoise)
	ON_MESSAGE(WM_MEDIAN_FILTER, &CExperimentImgDlg::OnThreadMsgReceived_MedianFilter)
	ON_MESSAGE(WM_BICUBIC_FILTER_ROTATION,&CExperimentImgDlg::OnThreadMsgReceived_RotationWithBicubicFilter)
	ON_MESSAGE(WM_AUTO_COLOR_GRADATION, &CExperimentImgDlg::OnThreadMsgReceived_AutoColorGradation)
	ON_MESSAGE(WM_AUTO_WHITE_BALANCE, &CExperimentImgDlg::OnThreadMsgReceived_WhiteBalance)
	ON_MESSAGE(WM_IMAGE_BLENDING, &CExperimentImgDlg::OnThreadMsgReceived_ImageBlending)
	ON_MESSAGE(WM_BILATERAL_FILTER, &CExperimentImgDlg::OnThreadMsgReceived_BilateralFilter)
	ON_BN_CLICKED(IDC_BUTTON1, &CExperimentImgDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CExperimentImgDlg ��Ϣ�������

BOOL CExperimentImgDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);
	
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
//	mEditInfo.SetWindowTextW(CString("File Path"));
	CComboBox * cmb_function = ((CComboBox*)GetDlgItem(IDC_COMBO_FUNCTION));
	cmb_function->InsertString(0,_T("��������"));
	cmb_function->InsertString(1,_T("��ֵ�˲�"));
	cmb_function->InsertString(2,_T("�������˲�"));
	cmb_function->InsertString(3,_T("�Զ�ɫ��"));
	cmb_function->InsertString(4,_T("�Զ���ƽ��"));
	cmb_function->SetCurSel(0);

	CComboBox * cmb_thread = ((CComboBox*)GetDlgItem(IDC_COMBO_THREAD));
	cmb_thread->InsertString(0, _T("WIN���߳�"));
	cmb_thread->InsertString(1, _T("OpenMP"));
	cmb_thread->InsertString(2, _T("Boost���߳�"));
	cmb_thread->SetCurSel(0);

	CSliderCtrl * slider = ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_THREADNUM));
	slider->SetRange(1, cMaxThreadNum, TRUE);
	slider->SetPos(cMaxThreadNum);

	//AfxBeginThread((AFX_THREADPROC)&CExperimentImgDlg::Update, this);

	//��ʼ��result picture box�Ļ�����
	m_pImageResult = new CImage();
	m_pImageResult->Create(cPictureBufferWidth, cPictureBufferHeight, 24, 0);
	m_pImage1 = new CImage();
	m_pImage1->Create(cPictureBufferWidth, cPictureBufferHeight, 24, 0);
	m_pImage2 = new CImage();
	m_pImage2->Create(cPictureBufferWidth, cPictureBufferHeight, 24, 0);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CExperimentImgDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		//CAboutDlg dlgAbout;
		//dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CExperimentImgDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();

		RenderToPictureBox(&mPictureBox1, m_pImage1);
		RenderToPictureBox(&mPictureBox2, m_pImage2);
		RenderToPictureBox(&mPictureBox_Result, m_pImageResult);
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CExperimentImgDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CExperimentImgDlg::RenderToPictureBox(CStatic * pPicBox, CImage * pImage)
{
	//��ȾCImage��Picturebox��
	if (pImage != nullptr)
	{
		CRect originalRect;
		CRect scaledRect;
		int height = pImage->GetHeight();
		int width = pImage->GetWidth();

		//get Context Device Chart 
		pPicBox->GetClientRect(&originalRect);
		CDC *pDC = pPicBox->GetDC();
		SetStretchBltMode(pDC->m_hDC, STRETCH_HALFTONE);

		//stretch pictures
		if (width <= originalRect.Width() && height <= originalRect.Height())
		{
			scaledRect = CRect(originalRect.TopLeft(), CSize(width, height));
			pImage->StretchBlt(pDC->m_hDC, scaledRect, SRCCOPY);
		}
		else
		{
			float xScale = (float)originalRect.Width() / (float)width;
			float yScale = (float)originalRect.Height() / (float)height;
			float ScaleIndex = (xScale <= yScale ? xScale : yScale);
			int scaleWidth = width*ScaleIndex;
			int scaleHeight = height*ScaleIndex;
			scaledRect = CRect(originalRect.TopLeft(), CSize(scaleWidth,scaleHeight));
			pImage->StretchBlt(pDC->m_hDC, scaledRect, SRCCOPY);
		}

		ReleaseDC(pDC);
	}
}

//���ļ�1��ť�¼�
void CExperimentImgDlg::OnBnClickedButtonOpen()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	TCHAR szFilter[] = _T("JPEG(*jpg)|*.jpg|*.bmp|*.png|TIFF(*.tif)|*.tif|All Files ��*.*��|*.*||");
	CString filePath("");
	
	CFileDialog fileOpenDialog(TRUE, NULL, NULL, OFN_HIDEREADONLY, szFilter);
	if (fileOpenDialog.DoModal() == IDOK)
	{
		VERIFY(filePath = fileOpenDialog.GetPathName());
		CString strFilePath(filePath);

		//�����ļ������ŵ�dest image
		CImage tmpImage;
		tmpImage.Load(strFilePath);
		ImageProcessor::ImageCopy(&tmpImage, m_pImage1);
		this->Invalidate();
	}
}

//���ļ�2��ť�¼�
void CExperimentImgDlg::OnBnClickedButtonOpen2()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	TCHAR szFilter[] = _T("JPEG(*jpg)|*.jpg|*.bmp|*.png|TIFF(*.tif)|*.tif|All Files ��*.*��|*.*||");
	CString filePath("");

	CFileDialog fileOpenDialog(TRUE, NULL, NULL, OFN_HIDEREADONLY, szFilter);
	if (fileOpenDialog.DoModal() == IDOK)
	{
		VERIFY(filePath = fileOpenDialog.GetPathName());
		CString strFilePath(filePath);
		//		mEditInfo.SetWindowTextW(strFilePath);	//���ı�������ʾͼ��·��

		CImage tmpImage;
		tmpImage.Load(strFilePath);
		ImageProcessor::ImageCopy(&tmpImage, m_pImage2);
		this->Invalidate();
	}
}

void CExperimentImgDlg::OnCbnSelchangeComboFunction()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
}

void CExperimentImgDlg::OnNMCustomdrawSliderThreadnum(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CSliderCtrl *slider = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_THREADNUM);
	CString text("");
	mThreadNum = slider->GetPos();
	text.Format(_T("%d"), mThreadNum);
	GetDlgItem(IDC_STATIC_THREADNUM)->SetWindowText(text);
	*pResult = 0;
}

void CExperimentImgDlg::OnBnClickedButtonProcess()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CComboBox* cmb_function = ((CComboBox*)GetDlgItem(IDC_COMBO_FUNCTION));
	int func = cmb_function->GetCurSel();
	switch (func)
	{
	case 0:  //��������
		mFunction_AddNoise();
		break;
	case 1: //����Ӧ��ֵ�˲�
		mFunction_MedianFilter();
		break;
	default:
		break;
	}
}

//����ֵ�˲����߳�ִ����ϵĻص�����
LRESULT CExperimentImgDlg::OnThreadMsgReceived_MedianFilter(WPARAM wParam, LPARAM lParam)
{
	static int tempThreadCount = 0;
	static int tempProcessCount = 0;
	//CButton* clb_circulation = ((CButton*)GetDlgItem(IDC_CHECK_CIRCULATION));
	int circulation = 1; //clb_circulation->GetCheck() == 0 ? 1 : 100;

	//���ĳһ���߳�ִ�����
	if ((int)wParam == 1)
	{
		++tempThreadCount;
		// �������̶߳�������ֵ1�����һ���߳�ִ�����~��ʾʱ��
		if (mThreadNum == tempThreadCount)
		{
			CTime endTime = CTime::GetTickCount();
			CString timeStr;
			timeStr.Format(_T("��ʱ:%dms"), endTime - mStartTime);
			tempThreadCount = 0;
			tempProcessCount++;

			//ѭ����������
			AfxMessageBox(timeStr);
			//��������picturebox��onPaint�¼�
			::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_PAINT, 1, NULL);
		}
	}
	return 0;
}

//�������������߳�ִ����ϵĻص�����
LRESULT CExperimentImgDlg::OnThreadMsgReceived_AddNoise(WPARAM wParam, LPARAM lParam)
{
	static int tempThreadCount = 0;
	static int tempProcessCount = 0;
	//CButton* clb_circulation = ((CButton*)GetDlgItem(IDC_CHECK_CIRCULATION));
	int circulation = 1;// clb_circulation->GetCheck() == 0 ? 1 : 100;

	//���ĳһ���߳�ִ�����
	if ((int)wParam == 1)
	{
		tempThreadCount++;
		// �������̶߳�������ֵ1�����һ���߳�ִ�����~��ʾʱ��
		if (mThreadNum == tempThreadCount)
		{
			tempThreadCount = 0;
			tempProcessCount++;

			//ѭ����������
			CTime endTime = CTime::GetTickCount();
			CString timeStr;
			timeStr.Format(_T("����%d��,��ʱ:%dms"), circulation, endTime - mStartTime);
			AfxMessageBox(timeStr);				
			//��������picturebox��onPaint�¼�
			::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_PAINT, 1, NULL);
		}
	}
	return 0;
}

//��ͼ����ת�����β�ֵ���߳�ִ����ϵĻص�����
LRESULT CExperimentImgDlg::OnThreadMsgReceived_RotationWithBicubicFilter(WPARAM wParam, LPARAM lParam)
{
	static int tempThreadCount = 0;
	static int tempProcessCount = 0;

	//���ĳһ���߳�ִ�����
	if ((int)wParam == 1)
	{
		tempThreadCount++;
		// �������̶߳�������ֵ1�����һ���߳�ִ�����~��ʾʱ��
		if (mThreadNum == tempThreadCount)
		{
			tempThreadCount = 0;
			tempProcessCount++;

			//ѭ����������
			CTime endTime = CTime::GetTickCount();
			CString timeStr;
			timeStr.Format(_T("����%d��,��ʱ:%dms"), 1, endTime - mStartTime);
			AfxMessageBox(timeStr);
			//��������picturebox��onPaint�¼�
			::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_PAINT, 1, NULL);
		}
	}
	return 0;
}

//���Զ�ɫ�ס��߳�ִ����ϵĻص�����
LRESULT CExperimentImgDlg::OnThreadMsgReceived_AutoColorGradation(WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}

//���Զ���ƽ�⡿�߳�ִ����ϵĻص�����
LRESULT CExperimentImgDlg::OnThreadMsgReceived_WhiteBalance(WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}

//��ͼ���ںϡ��߳�ִ����ϵĻص�����
LRESULT CExperimentImgDlg::OnThreadMsgReceived_ImageBlending(WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}

//��˫���˲����߳�ִ����ϵĻص�����
LRESULT CExperimentImgDlg::OnThreadMsgReceived_BilateralFilter(WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}

/*************************************

					PRIVATE

**************************************/

void CExperimentImgDlg::mFunction_AddNoise()
{
	CComboBox* cmb_parallelScheme = ((CComboBox*)GetDlgItem(IDC_COMBO_THREAD));
	int parallelScheme = cmb_parallelScheme->GetCurSel();
	mStartTime = CTime::GetTickCount();
	switch (parallelScheme)
	{
	case 0://win���߳�
	{
		ImageProcessor::AddNoise_WIN(m_pImage1, m_pImageResult, mThreadNum);
		break;
	}

	//OpenMp
	case 1:
	{
		ImageProcessor::AddNoise_OpenMP(m_pImage1, m_pImageResult, mThreadNum);
		break;
	}

	case 2://boost
	{
		AfxMessageBox(_T("�������������㷨δʵ�ֻ���boost��Ĳ��м��٣�"));
		break;
	}
	}
}

void CExperimentImgDlg::mFunction_MedianFilter()
{
	//��comboBox�����ȡҪ��ʾ�Ĺ���
	CComboBox* cmb_parallelScheme = ((CComboBox*)GetDlgItem(IDC_COMBO_THREAD));
	int parallelScheme = cmb_parallelScheme->GetCurSel();

	mStartTime = CTime::GetTickCount();
	switch (parallelScheme)
	{
	case 0://win���߳�
	{
		ImageProcessor::MedianFilter_WIN(m_pImage1, m_pImageResult, mThreadNum);
		break;
	}

	case 1://openmp
	{
		ImageProcessor::MedianFilter_OpenMP(m_pImage1, m_pImageResult, mThreadNum);
		break;
	}

	case 2://boost
		AfxMessageBox(_T("����ֵ�˲����㷨δʵ�ֻ���boost��Ĳ��м��٣�"));
		break;
	}
}

void CExperimentImgDlg::OnBnClickedButton1()
{
	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_PAINT, 1, NULL);
}
