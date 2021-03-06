
#include "stdafx.h"
#include "ExperimentImg.h"
#include "ExperimentImgDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CExperimentImgDlg 对话框
CExperimentImgDlg::CExperimentImgDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_EXPERIMENTIMG_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	//加载对话框的时候初始化
	mThreadNum = 1;
	srand(time(0));
}

void CExperimentImgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//	DDX_Control(pDX, IDC_EDIT_INFO, mEditInfo);
	DDX_Control(pDX, IDC_PICTURE, mPictureBox1);
	//DDX_Control(pDX, IDC_CHECK_100, m_CheckBox_Circulate);
	DDX_Control(pDX, IDC_BUTTON_OPEN2, mButton_OpenFile2);
	DDX_Control(pDX, IDC_PICTURE2, mPictureBox2);
	DDX_Control(pDX, IDC_PICTURE3, mPictureBox_Result);
	DDX_Control(pDX, IDC_BUTTON_OPEN, mButton_OpenFile1);
	DDX_Control(pDX, IDC_SLIDER1, mSlideControl_Alpha);
	DDX_Control(pDX, IDC_SLIDER_THREADNUM, mSlideControl_ThreadNum);
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
	//自定义的多线程结束通知的回调函数
	ON_MESSAGE(WM_NOISE, &CExperimentImgDlg::OnThreadMsgReceived_AddNoise)
	ON_MESSAGE(WM_MEDIAN_FILTER, &CExperimentImgDlg::OnThreadMsgReceived_MedianFilter)
	ON_MESSAGE(WM_BICUBIC_FILTER_ROTATION,&CExperimentImgDlg::OnThreadMsgReceived_RotationWithBicubicFilter)
	ON_MESSAGE(WM_AUTO_LEVELS, &CExperimentImgDlg::OnThreadMsgReceived_AutoLevels)
	ON_MESSAGE(WM_AUTO_WHITE_BALANCE, &CExperimentImgDlg::OnThreadMsgReceived_WhiteBalance)
	ON_MESSAGE(WM_IMAGE_BLENDING, &CExperimentImgDlg::OnThreadMsgReceived_ImageBlending)
	ON_MESSAGE(WM_BILATERAL_FILTER, &CExperimentImgDlg::OnThreadMsgReceived_BilateralFilter)
	ON_BN_CLICKED(IDC_BUTTON1, &CExperimentImgDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CExperimentImgDlg 消息处理程序

BOOL CExperimentImgDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
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

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
//	mEditInfo.SetWindowTextW(CString("File Path"));
	CComboBox * cmb_function = ((CComboBox*)GetDlgItem(IDC_COMBO_FUNCTION));
	cmb_function->InsertString(0,_T("椒盐噪声(左)"));
	cmb_function->InsertString(1,_T("中值滤波(左)"));
	cmb_function->InsertString(2,_T("旋转与三阶采样(左)"));
	cmb_function->InsertString(3,_T("自动色阶(左)"));
	cmb_function->InsertString(4,_T("自动白平衡(左)"));
	cmb_function->InsertString(5, _T("图像融合(左+右)"));
	cmb_function->InsertString(6, _T("双边滤波(左)"));
	cmb_function->SetCurSel(0);

	CComboBox * cmb_thread = ((CComboBox*)GetDlgItem(IDC_COMBO_THREAD));
	cmb_thread->InsertString(0, _T("WIN多线程"));
	cmb_thread->InsertString(1, _T("OpenMP"));
	cmb_thread->InsertString(2, _T("CUDA"));
	cmb_thread->SetCurSel(0);

	mSlideControl_ThreadNum.SetRange(1, cMaxThreadNum, TRUE);
	mSlideControl_ThreadNum.SetPos(cMaxThreadNum);

	mSlideControl_Alpha.SetRange(0, 100, TRUE);
	mSlideControl_Alpha.SetPos(50);


	//初始化result picture box的缓冲区
	m_pImageResult = new CImage();
	m_pImageResult->Create(cPictureBufferWidth, cPictureBufferHeight, 24, 0);
	m_pImage1 = new CImage();
	m_pImage1->Create(cPictureBufferWidth, cPictureBufferHeight, 24, 0);
	m_pImage2 = new CImage();
	m_pImage2->Create(cPictureBufferWidth, cPictureBufferHeight, 24, 0);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CExperimentImgDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
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

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CExperimentImgDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CExperimentImgDlg::RenderToPictureBox(CStatic * pPicBox, CImage * pImage)
{
	//渲染CImage到Picturebox上
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
			int scaleWidth = int(width*ScaleIndex);
			int scaleHeight = int(height*ScaleIndex);
			scaledRect = CRect(originalRect.TopLeft(), CSize(scaleWidth,scaleHeight));
			pImage->StretchBlt(pDC->m_hDC, scaledRect, SRCCOPY);
		}

		ReleaseDC(pDC);
	}
}

//打开文件1按钮事件
void CExperimentImgDlg::OnBnClickedButtonOpen()
{
	// TODO: 在此添加控件通知处理程序代码
	TCHAR szFilter[] = _T("JPEG(*jpg)|*.jpg|*.bmp|*.png|TIFF(*.tif)|*.tif|All Files （*.*）|*.*||");
	CString filePath("");
	
	CFileDialog fileOpenDialog(TRUE, NULL, NULL, OFN_HIDEREADONLY, szFilter);
	if (fileOpenDialog.DoModal() == IDOK)
	{
		VERIFY(filePath = fileOpenDialog.GetPathName());
		CString strFilePath(filePath);

		//加载文件并缩放到dest image
		CImage tmpImage;
		tmpImage.Load(strFilePath);
		ImageProcessor::ImageCopy(&tmpImage, m_pImage1);
		this->Invalidate();
	}
}

//打开文件2按钮事件
void CExperimentImgDlg::OnBnClickedButtonOpen2()
{
	// TODO: 在此添加控件通知处理程序代码
	TCHAR szFilter[] = _T("JPEG(*jpg)|*.jpg|*.bmp|*.png|TIFF(*.tif)|*.tif|All Files （*.*）|*.*||");
	CString filePath("");

	CFileDialog fileOpenDialog(TRUE, NULL, NULL, OFN_HIDEREADONLY, szFilter);
	if (fileOpenDialog.DoModal() == IDOK)
	{
		VERIFY(filePath = fileOpenDialog.GetPathName());
		CString strFilePath(filePath);
		//		mEditInfo.SetWindowTextW(strFilePath);	//在文本框中显示图像路径

		CImage tmpImage;
		tmpImage.Load(strFilePath);
		ImageProcessor::ImageCopy(&tmpImage, m_pImage2);
		this->Invalidate();
	}
}

void CExperimentImgDlg::OnCbnSelchangeComboFunction()
{
	// TODO: 在此添加控件通知处理程序代码
}

void CExperimentImgDlg::OnNMCustomdrawSliderThreadnum(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	CSliderCtrl *slider = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_THREADNUM);
	CString text("");
	mThreadNum = slider->GetPos();
	text.Format(_T("%d"), mThreadNum);
	GetDlgItem(IDC_STATIC_THREADNUM)->SetWindowText(text);
	*pResult = 0;
}

void CExperimentImgDlg::OnBnClickedButtonProcess()
{
	// TODO: 在此添加控件通知处理程序代码
	CComboBox* cmb_function = ((CComboBox*)GetDlgItem(IDC_COMBO_FUNCTION));
	int func = cmb_function->GetCurSel();
	switch (func)
	{
	case 0:  //椒盐噪声
		mFunction_AddNoise();
		break;
	case 1: //自适应中值滤波
		mFunction_MedianFilter();
		break;
	case 2://旋转与缩放
		mFunction_Rotate();
		break;
	case 3://自动色阶
		mFunction_AutoLevels();
		break;
	case 4://自动白平衡
		mFunction_AutoWhiteBalance();
		break;
	case 5://图像融合
		mFunction_ImageBlending();
		break;
	case 6://双边滤波
		mFunction_BilateralFilter();
		break;
	default:
		break;
	}
}

//刷新图像
void CExperimentImgDlg::OnBnClickedButton1()
{
	::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_PAINT, 1, NULL);
}


//【中值滤波】线程执行完毕的回调函数
LRESULT CExperimentImgDlg::OnThreadMsgReceived_MedianFilter(WPARAM wParam, LPARAM lParam)
{
	static int tempThreadCount = 0;
	static int tempProcessCount = 0;
	//CButton* clb_circulation = ((CButton*)GetDlgItem(IDC_CHECK_CIRCULATION));
	int circulation = 1; //clb_circulation->GetCheck() == 0 ? 1 : 100;

	//如果某一个线程执行完成
	if ((int)wParam == 1)
	{
		++tempThreadCount;
		// 当所有线程都返回了值1代表第一批线程执行完成~显示时间
		if (mThreadNum == tempThreadCount)
		{
			CTime endTime = CTime::GetTickCount();
			CString timeStr;
			timeStr.Format(_T("耗时:%dms"), endTime - mStartTime);
			tempThreadCount = 0;
			tempProcessCount++;

			//循环次数够了
			AfxMessageBox(timeStr);
			//主动触发picturebox的onPaint事件
			::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_PAINT, 1, NULL);
		}
	}
	return 0;
}

//【椒盐噪声】线程执行完毕的回调函数
LRESULT CExperimentImgDlg::OnThreadMsgReceived_AddNoise(WPARAM wParam, LPARAM lParam)
{
	static int tempThreadCount = 0;
	static int tempProcessCount = 0;
	//CButton* clb_circulation = ((CButton*)GetDlgItem(IDC_CHECK_CIRCULATION));
	int circulation = 1;// clb_circulation->GetCheck() == 0 ? 1 : 100;

	//如果某一个线程执行完成
	if ((int)wParam == 1)
	{
		tempThreadCount++;
		// 当所有线程都返回了值1代表第一批线程执行完成~显示时间
		if (mThreadNum == tempThreadCount)
		{
			tempThreadCount = 0;
			tempProcessCount++;

			//循环次数够了
			CTime endTime = CTime::GetTickCount();
			CString timeStr;
			timeStr.Format(_T("耗时:%dms"), endTime - mStartTime);
			AfxMessageBox(timeStr);				
			//主动触发picturebox的onPaint事件
			::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_PAINT, 1, NULL);
		}
	}
	return 0;
}

//【图像旋转与三次插值】线程执行完毕的回调函数
LRESULT CExperimentImgDlg::OnThreadMsgReceived_RotationWithBicubicFilter(WPARAM wParam, LPARAM lParam)
{
	static int tempThreadCount = 0;
	static int tempProcessCount = 0;

	//如果某一个线程执行完成
	if ((int)wParam == 1)
	{
		tempThreadCount++;
		// 当所有线程都返回了值1代表第一批线程执行完成~显示时间
		if (mThreadNum == tempThreadCount)
		{
			tempThreadCount = 0;
			tempProcessCount++;

			//循环次数够了
			CTime endTime = CTime::GetTickCount();
			CString timeStr;
			timeStr.Format(_T("耗时:%dms"), endTime - mStartTime);
			AfxMessageBox(timeStr);
			//主动触发picturebox的onPaint事件
			::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_PAINT, 1, NULL);
		}
	}
	return 0;
}

//【自动色阶】线程执行完毕的回调函数
LRESULT CExperimentImgDlg::OnThreadMsgReceived_AutoLevels(WPARAM wParam, LPARAM lParam)
{
	//此算法就开了一个线程- -

	if ((int)wParam == 1)
	{
		CTime endTime = CTime::GetTickCount();
		CString timeStr;
		timeStr.Format(_T("耗时:%dms"), endTime - mStartTime);
		AfxMessageBox(timeStr);
		//主动触发picturebox的onPaint事件
		::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_PAINT, 1, NULL);
	}
	return 0;
}

//【自动白平衡】线程执行完毕的回调函数
LRESULT CExperimentImgDlg::OnThreadMsgReceived_WhiteBalance(WPARAM wParam, LPARAM lParam)
{
	if ((int)wParam == 1)
	{
		CTime endTime = CTime::GetTickCount();
		CString timeStr;
		timeStr.Format(_T("耗时:%dms"), endTime - mStartTime);
		AfxMessageBox(timeStr);
		//主动触发picturebox的onPaint事件
		::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_PAINT, 1, NULL);
	}
	return 0;
}

//【图像融合】线程执行完毕的回调函数
LRESULT CExperimentImgDlg::OnThreadMsgReceived_ImageBlending(WPARAM wParam, LPARAM lParam)
{
	static int tempThreadCount = 0;
	static int tempProcessCount = 0;
	//CButton* clb_circulation = ((CButton*)GetDlgItem(IDC_CHECK_CIRCULATION));
	int circulation = 1;// clb_circulation->GetCheck() == 0 ? 1 : 100;

	//如果某一个线程执行完成
	if ((int)wParam == 1)
	{
		tempThreadCount++;
		// 当所有线程都返回了值1代表第一批线程执行完成~显示时间
		if (mThreadNum == tempThreadCount)
		{
			tempThreadCount = 0;
			tempProcessCount++;

			//循环次数够了
			CTime endTime = CTime::GetTickCount();
			CString timeStr;
			timeStr.Format(_T("耗时:%dms"), endTime - mStartTime);
			AfxMessageBox(timeStr);
			//主动触发picturebox的onPaint事件
			::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_PAINT, 1, NULL);
		}
	}
	return 0;
}

//【双边滤波】线程执行完毕的回调函数
LRESULT CExperimentImgDlg::OnThreadMsgReceived_BilateralFilter(WPARAM wParam, LPARAM lParam)
{
	static int tempThreadCount = 0;
	static int tempProcessCount = 0;
	int circulation = 1;// clb_circulation->GetCheck() == 0 ? 1 : 100;

	//如果某一个线程执行完成
	if ((int)wParam == 1)
	{
		tempThreadCount++;
		// 当所有线程都返回了值1代表第一批线程执行完成~显示时间
		if (mThreadNum == tempThreadCount)
		{
			tempThreadCount = 0;
			tempProcessCount++;

			//循环次数够了
			CTime endTime = CTime::GetTickCount();
			CString timeStr;
			timeStr.Format(_T("耗时:%dms"), endTime - mStartTime);
			AfxMessageBox(timeStr);
			//主动触发picturebox的onPaint事件
			::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_PAINT, 1, NULL);
		}
	}
	return 0;
}

/*************************************

					PRIVATE

**************************************/

void CExperimentImgDlg::mFunction_AddNoise()
{
	//从comboBox里面获取多线程的方案
	CComboBox* cmb_parallelScheme = ((CComboBox*)GetDlgItem(IDC_COMBO_THREAD));
	int parallelScheme = cmb_parallelScheme->GetCurSel();
	mStartTime = CTime::GetTickCount();
	switch (parallelScheme)
	{
	case 0://mfc多线程
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

	case 2://CUDA
	{
		AfxMessageBox(_T("【椒盐噪声】算法未实现基于CUDA的并行加速！"));
		break;
	}
	}
}

void CExperimentImgDlg::mFunction_MedianFilter()
{
	//从comboBox里面获取多线程的方案
	CComboBox* cmb_parallelScheme = ((CComboBox*)GetDlgItem(IDC_COMBO_THREAD));
	int parallelScheme = cmb_parallelScheme->GetCurSel();

	mStartTime = CTime::GetTickCount();
	switch (parallelScheme)
	{
	case 0://win多线程
	{
		ImageProcessor::MedianFilter_WIN(m_pImage1, m_pImageResult, mThreadNum);
		break;
	}

	case 1://openmp
	{
		ImageProcessor::MedianFilter_OpenMP(m_pImage1, m_pImageResult, mThreadNum);
		break;
	}

	case 2://CUDA
		AfxMessageBox(_T("【中值滤波】算法未实现基于CUDA的并行加速！"));
		break;
	}
}

void CExperimentImgDlg::mFunction_Rotate()
{
	//从comboBox里面获取多线程的方案
	CComboBox* cmb_parallelScheme = ((CComboBox*)GetDlgItem(IDC_COMBO_THREAD));
	int parallelScheme = cmb_parallelScheme->GetCurSel();
	mStartTime = CTime::GetTickCount();
	float angle = float(mSlideControl_Alpha.GetPos()) / mSlideControl_Alpha.GetRangeMax() * 2.0f * 3.1415926f;
	float scale = 0.5f;

	switch (parallelScheme)
	{
	case 0://win多线程
	{
		ImageProcessor::Rotate_WIN(m_pImage1, m_pImageResult, mThreadNum,angle,scale);
		break;
	}

	case 1://openmp
	{
		ImageProcessor::Rotate_OpenMP(m_pImage1, m_pImageResult, mThreadNum,angle, scale);
		break;
	}

	case 2://CUDA
		ImageProcessor::Rotate_CUDA(m_pImage1, m_pImageResult, angle, scale);
		break;
	}
}

void CExperimentImgDlg::mFunction_AutoLevels()
{
	//从comboBox里面获取多线程的方案
	CComboBox* cmb_parallelScheme = ((CComboBox*)GetDlgItem(IDC_COMBO_THREAD));
	int parallelScheme = cmb_parallelScheme->GetCurSel();
	mStartTime = CTime::GetTickCount();

	//自动色阶一开始用统计全图像素RGB动态范围，要是这也用多线程来统计，返回了之后
	//再开多次多线程来映射颜色，这怕是要烦死 - -干脆就不让多线程了

	switch (parallelScheme)
	{
	case 0://win多线程
	{
		AfxMessageBox(_T("【自动色阶】算法未实现基于MFC多线程的并行加速！"));
		break;
	}

	case 1://openmp
	{
		ImageProcessor::AutoLevels_OpenMP(m_pImage1, m_pImageResult, 1);
		break;
	}

	case 2://CUDA
		ImageProcessor::AutoLevels_CUDA(m_pImage1, m_pImageResult);
		break;
	}
}

void CExperimentImgDlg::mFunction_AutoWhiteBalance()
{
	//从comboBox里面获取多线程的方案
	CComboBox* cmb_parallelScheme = ((CComboBox*)GetDlgItem(IDC_COMBO_THREAD));
	int parallelScheme = cmb_parallelScheme->GetCurSel();
	mStartTime = CTime::GetTickCount();

	//自动色阶一开始用统计全图像素RGB动态范围，要是这也用多线程来统计，返回了之后
	//再开多次多线程来映射颜色，这怕是要烦死 - -干脆就不让多线程了

	switch (parallelScheme)
	{
	case 0://win多线程
	{
		AfxMessageBox(_T("【自动白平衡】算法未实现基于MFC多线程的并行加速！"));
		break;
	}

	case 1://openmp
	{
		ImageProcessor::AutoWhiteBalance_OpenMP(m_pImage1, m_pImageResult, 1);
		break;
	}

	case 2://CUDA
		AfxMessageBox(_T("【自动白平衡】算法未实现基于CUDA的并行加速！"));
		break;
	}
}

void CExperimentImgDlg::mFunction_ImageBlending()
{
	//从comboBox里面获取多线程的方案
	CComboBox* cmb_parallelScheme = ((CComboBox*)GetDlgItem(IDC_COMBO_THREAD));
	int parallelScheme = cmb_parallelScheme->GetCurSel();
	mStartTime = CTime::GetTickCount();

	//获取融合的alpha值
	float alpha = mSlideControl_Alpha.GetPos() / float(mSlideControl_Alpha.GetRangeMax());
	switch (parallelScheme)
	{
	case 0://win多线程
	{
		ImageProcessor::ImageBlending_WIN(m_pImage1, m_pImage2, m_pImageResult, mThreadNum, alpha);
		break;
	}

	case 1://openmp
	{
		ImageProcessor::ImageBlending_OpenMP(m_pImage1,m_pImage2, m_pImageResult, mThreadNum, alpha);
		break;
	}

	case 2://CUDA
		AfxMessageBox(_T("【图像融合】算法未实现基于CUDA的并行加速！"));
		break;
	}
}

void CExperimentImgDlg::mFunction_BilateralFilter()
{
	//从comboBox里面获取多线程的方案
	CComboBox* cmb_parallelScheme = ((CComboBox*)GetDlgItem(IDC_COMBO_THREAD));
	int parallelScheme = cmb_parallelScheme->GetCurSel();
	mStartTime = CTime::GetTickCount();

	switch (parallelScheme)
	{
	case 0://win多线程
	{
		AfxMessageBox(_T("【双边滤波】算法未实现基于MFC多线程的并行加速！"));
		break;
	}

	case 1://openmp
	{
		ImageProcessor::BilateralFilter_OpenMP(m_pImage1, m_pImageResult, mThreadNum);
		break;
	}

	case 2://CUDA
		AfxMessageBox(_T("【双边滤波】算法未实现基于CUDA的并行加速！"));
		break;
	}
}

