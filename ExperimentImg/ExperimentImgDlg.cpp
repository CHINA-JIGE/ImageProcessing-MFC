
// ExperimentImgDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "ExperimentImg.h"
#include "ExperimentImgDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CExperimentImgDlg �Ի���



CExperimentImgDlg::CExperimentImgDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_EXPERIMENTIMG_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	//���ضԻ����ʱ���ʼ��
	m_pImgSrc = NULL;
//	m_pImgCpy = NULL;
	m_nThreadNum = 1;
	m_pThreadParam = new ThreadParam[MAX_THREAD];
	srand(time(0));
}

void CExperimentImgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//	DDX_Control(pDX, IDC_EDIT_INFO, mEditInfo);
	DDX_Control(pDX, IDC_PICTURE, mPictureControl);
	DDX_Control(pDX, IDC_CHECK_100, m_CheckCirculation);
}

BEGIN_MESSAGE_MAP(CExperimentImgDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &CExperimentImgDlg::OnBnClickedButtonOpen)
//	ON_EN_CHANGE(IDC_EDIT1, &CExperimentImgDlg::OnEnChangeEdit1)
//	ON_EN_CHANGE(IDC_EDIT_INFO, &CExperimentImgDlg::OnEnChangeEditInfo)
ON_CBN_SELCHANGE(IDC_COMBO_FUNCTION, &CExperimentImgDlg::OnCbnSelchangeComboFunction)
ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_THREADNUM, &CExperimentImgDlg::OnNMCustomdrawSliderThreadnum)
ON_BN_CLICKED(IDC_BUTTON_PROCESS, &CExperimentImgDlg::OnBnClickedButtonProcess)
ON_MESSAGE(WM_NOISE, &CExperimentImgDlg::OnNoiseThreadMsgReceived)
ON_MESSAGE(WM_MEDIAN_FILTER, &CExperimentImgDlg::OnMedianFilterThreadMsgReceived)
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
	slider->SetRange(1, MAX_THREAD, TRUE);
	slider->SetPos(MAX_THREAD);

	AfxBeginThread((AFX_THREADPROC)&CExperimentImgDlg::Update, this);
	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CExperimentImgDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
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

		RenderToPictureBox(&mPictureControl, m_pImgSrc);

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
		CRect rect;
		CRect rect1;
		int height = pImage->GetHeight();
		int width = pImage->GetWidth();

		//get Context Device Chart 
		pPicBox->GetClientRect(&rect);
		CDC *pDC = pPicBox->GetDC();
		SetStretchBltMode(pDC->m_hDC, STRETCH_HALFTONE);

		//stretch pictures
		if (width <= rect.Width() && height <= rect.Width())
		{
			rect1 = CRect(rect.TopLeft(), CSize(width, height));
			pImage->StretchBlt(pDC->m_hDC, rect1, SRCCOPY);
		}
		else
		{
			float xScale = (float)rect.Width() / (float)width;
			float yScale = (float)rect.Height() / (float)height;
			float ScaleIndex = (xScale <= yScale ? xScale : yScale);
			rect1 = CRect(rect.TopLeft(), CSize((int)width*ScaleIndex, (int)height*ScaleIndex));
			//��picture control������ͼ�����ź�Ĵ�С
			//				CWnd *pWnd;
			//				pWnd = GetDlgItem(IDC_PICTURE);
			//				pWnd->MoveWindow(CRect((int)rect.top, (int)rect.left, (int)width*ScaleIndex, (int)height*ScaleIndex));
			pImage->StretchBlt(pDC->m_hDC, rect1, SRCCOPY);
		}
		ReleaseDC(pDC);
	}
}

UINT CExperimentImgDlg::Update(void* p)
{
	while (1)
	{
		Sleep(200);
		CExperimentImgDlg* dlg = (CExperimentImgDlg*)p;
		if (dlg->m_pImgSrc != NULL)
		{
			int height;
			int width;
			CRect rect;
			CRect rect1;
			height = dlg->m_pImgSrc->GetHeight();
			width = dlg->m_pImgSrc->GetWidth();

			dlg->mPictureControl.GetClientRect(&rect);
			CDC *pDC = dlg->mPictureControl.GetDC();
			SetStretchBltMode(pDC->m_hDC, STRETCH_HALFTONE);

			if (width <= rect.Width() && height <= rect.Width())
			{
				rect1 = CRect(rect.TopLeft(), CSize(width, height));
				dlg->m_pImgSrc->StretchBlt(pDC->m_hDC, rect1, SRCCOPY);
			}
			else
			{
				float xScale = (float)rect.Width() / (float)width;
				float yScale = (float)rect.Height() / (float)height;
				float ScaleIndex = (xScale <= yScale ? xScale : yScale);
				rect1 = CRect(rect.TopLeft(), CSize((int)width*ScaleIndex, (int)height*ScaleIndex));
				dlg->m_pImgSrc->StretchBlt(pDC->m_hDC, rect1, SRCCOPY);
			}
			dlg->ReleaseDC(pDC);
		}
	}
	return 0;
}

/*void CExperimentImgDlg::ThreadDraw(DrawPara *p)
{
	CRect rect;
	GetClientRect(&rect);
	CDC     memDC;             // ���ڻ����ͼ���ڴ滭��  
	CBitmap memBitmap;         // ���ڻ����ͼ���ڴ滭��
	memDC.CreateCompatibleDC(p->pDC);  // ������ԭ���ʼ��ݵĻ���
	memBitmap.CreateCompatibleBitmap(p->pDC, p->width, p->height);  // ������ԭλͼ���ݵ��ڴ滭��
	memDC.SelectObject(&memBitmap);      // ���������뻭���Ĺ���
	memDC.FillSolidRect(rect, p->pDC->GetBkColor());
	p->pDC->SetStretchBltMode(HALFTONE);
	// ��pImgSrc���������Ż����ڴ滭����
	p->pImgSrc->StretchBlt(memDC.m_hDC, 0, 0, p->width, p->height);

	// ���ѻ��õĻ������Ƶ������Ļ�������
	p->pDC->BitBlt(p->oriX, p->oriY, p->width, p->height, &memDC, 0, 0, SRCCOPY);
	memBitmap.DeleteObject();
	memDC.DeleteDC();
}*/

/*void CExperimentImgDlg::ImageCopy(CImage* pImgSrc, CImage* pImgDrt)
{
	int MaxColors = pImgSrc->GetMaxColorTableEntries();
	RGBQUAD* ColorTab;
	ColorTab = new RGBQUAD[MaxColors];

	CDC *pDCsrc, *pDCdrc;
	if (!pImgDrt->IsNull())
	{
		pImgDrt->Destroy();
	}
	pImgDrt->Create(pImgSrc->GetWidth(), pImgSrc->GetHeight(), pImgSrc->GetBPP(), 0);

	if (pImgSrc->IsIndexed())
	{
		pImgSrc->GetColorTable(0, MaxColors, ColorTab);
		pImgDrt->SetColorTable(0, MaxColors, ColorTab);
	}

	pDCsrc = CDC::FromHandle(pImgSrc->GetDC());
	pDCdrc = CDC::FromHandle(pImgDrt->GetDC());
	pDCdrc->BitBlt(0, 0, pImgSrc->GetWidth(), pImgSrc->GetHeight(), pDCsrc, 0, 0, SRCCOPY);
	pImgSrc->ReleaseDC();
	pImgDrt->ReleaseDC();
	delete ColorTab;

}*/

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
//		mEditInfo.SetWindowTextW(strFilePath);	//���ı�������ʾͼ��·��

		if (m_pImgSrc != NULL)
		{
			m_pImgSrc->Destroy();
			delete m_pImgSrc;
		}
		m_pImgSrc = new CImage();
		m_pImgSrc->Load(strFilePath);
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
	m_nThreadNum = slider->GetPos();
	text.Format(_T("%d"), m_nThreadNum);
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
		AddNoise();
		break;
	case 1: //����Ӧ��ֵ�˲�
		MedianFilter();
		break;
	default:
		break;
	}
}

void CExperimentImgDlg::AddNoise()
{
	CComboBox* cmb_thread = ((CComboBox*)GetDlgItem(IDC_COMBO_THREAD));
	int thread = cmb_thread->GetCurSel();
	CButton* clb_circulation = ((CButton*)GetDlgItem(IDC_CHECK_CIRCULATION));
	int circulation = clb_circulation->GetCheck() == 0 ? 1:100;
	startTime = CTime::GetTickCount();
	switch (thread)
	{
	case 0://win���߳�
	{
		ImageProcessor::AddNoise_WIN(m_pImgSrc, m_nThreadNum, m_pThreadParam);
		break;
	}

	//OpenMp
	case 1:
	{
		ImageProcessor::AddNoise_OpenMP(m_pImgSrc, m_nThreadNum, m_pThreadParam);
		break;
	}

	case 2://boost
	{
		AfxMessageBox(_T("�������������㷨δʵ�ֻ���boost��Ĳ��м��٣�"));
		break; 
	}
	}
}

void CExperimentImgDlg::MedianFilter()
{
	//��comboBox�����ȡҪ��ʾ�Ĺ���
	CComboBox* cmb_thread = ((CComboBox*)GetDlgItem(IDC_COMBO_THREAD));
	int thread = cmb_thread->GetCurSel();
	CButton* clb_circulation = ((CButton*)GetDlgItem(IDC_CHECK_CIRCULATION));
	int circulation = clb_circulation->GetCheck() == 0 ? 1 : 4;

	startTime = CTime::GetTickCount();
	m_nThreadNum;
	switch (thread)
	{
	case 0://win���߳�
	{
		ImageProcessor::MedianFilter_WIN(m_pImgSrc,m_nThreadNum,m_pThreadParam);
		break;
	}

	case 1://openmp
	{
		ImageProcessor::MedianFilter_OpenMP(m_pImgSrc, m_nThreadNum, m_pThreadParam);
		break;
	}

	case 2://boost
		AfxMessageBox(_T("����ֵ�˲����㷨δʵ�ֻ���boost��Ĳ��м��٣�"));
		break;
	}
}

//����ֵ�˲����߳�ִ����ϵĻص�����
LRESULT CExperimentImgDlg::OnMedianFilterThreadMsgReceived(WPARAM wParam, LPARAM lParam)
{
	static int tempThreadCount = 0;
	static int tempProcessCount = 0;
	CButton* clb_circulation = ((CButton*)GetDlgItem(IDC_CHECK_CIRCULATION));
	int circulation = clb_circulation->GetCheck() == 0 ? 1 : 100;

	//���ĳһ���߳�ִ�����
	if ((int)wParam == 1)
	{
		// �������̶߳�������ֵ1�����һ���߳�ִ�����~��ʾʱ��
		++tempThreadCount;
		if (m_nThreadNum == tempThreadCount)
		{
			CTime endTime = CTime::GetTickCount();
			CString timeStr;
			timeStr.Format(_T("��ʱ:%dms"), endTime - startTime);
			tempThreadCount = 0;
			tempProcessCount++;

			if (tempProcessCount < circulation)
			{
				//С��ѭ�������Ļ�����ִ��һ��
				ImageProcessor::MedianFilter_WIN(m_pImgSrc, m_nThreadNum, m_pThreadParam);
			}
			else
			{
				//ѭ����������
				tempProcessCount = 0;
				CTime endTime = CTime::GetTickCount();
				CString timeStr;
				timeStr.Format(_T("����%d��,��ʱ:%dms"), circulation, endTime - startTime);
				AfxMessageBox(timeStr);
			}
		}
	}
	return 0;
}

//�������������߳�ִ����ϵĻص�����
LRESULT CExperimentImgDlg::OnNoiseThreadMsgReceived(WPARAM wParam, LPARAM lParam)
{
	static int tempThreadCount = 0;
	static int tempProcessCount = 0;
	CButton* clb_circulation = ((CButton*)GetDlgItem(IDC_CHECK_CIRCULATION));
	int circulation = clb_circulation->GetCheck() == 0 ? 1 : 100;

	//���ĳһ���߳�ִ�����
	if ((int)wParam == 1)
	{
		tempThreadCount++;
		// �������̶߳�������ֵ1�����һ���߳�ִ�����~��ʾʱ��
		if (m_nThreadNum == tempThreadCount)
		{
			tempThreadCount = 0;
			tempProcessCount++;
			if (tempProcessCount < circulation)
			{
				//С��ѭ�������Ļ�����ִ��һ��
				ImageProcessor::AddNoise_WIN(m_pImgSrc, m_nThreadNum, m_pThreadParam);
			}
			else
			{
				//ѭ����������
				tempProcessCount = 0;
				CTime endTime = CTime::GetTickCount();
				CString timeStr;
				timeStr.Format(_T("����%d��,��ʱ:%dms"), circulation, endTime - startTime);
				AfxMessageBox(timeStr);
			}
		}
	}
	return 0;
}