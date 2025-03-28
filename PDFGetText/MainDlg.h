// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////
#pragma once

class CMainDlg : public SHostWnd
{
public:
	CMainDlg();
	~CMainDlg();

	void OnClose();
	void OnMaximize();
	void OnRestore();
	void OnMinimize();
	void OnSize(UINT nType, CSize size);
	BOOL OnInitDialog(HWND wndFocus, LPARAM lInitParam);

protected:
	void OnLanguage(int nID);
	void OnLanguageBtnCN();
	void OnLanguageBtnJP();

	void OnBnClickSelectFile();
	void OnBnClickSelectOutput();
	void OnBnClickParse();
	void OnBnClickPrePage();
	void OnBnClickNextPage();
	//soui消息
	EVENT_MAP_BEGIN()
		EVENT_NAME_COMMAND(L"btn_close", OnClose)
		EVENT_NAME_COMMAND(L"btn_min", OnMinimize)
		EVENT_NAME_COMMAND(L"btn_max", OnMaximize)
		EVENT_NAME_COMMAND(L"btn_restore", OnRestore)
		EVENT_NAME_COMMAND(L"zh_cn", OnLanguageBtnCN)
		EVENT_NAME_COMMAND(L"jp", OnLanguageBtnJP)

		EVENT_NAME_COMMAND(L"btn_select_file", OnBnClickSelectFile)
		EVENT_NAME_COMMAND(L"btn_select_output", OnBnClickSelectOutput)
		EVENT_NAME_COMMAND(L"btn_parase", OnBnClickParse)

		EVENT_NAME_COMMAND(L"btn_pre_page", OnBnClickPrePage)
		EVENT_NAME_COMMAND(L"btn_next_page", OnBnClickNextPage)
	EVENT_MAP_END()
		
	//HostWnd真实窗口消息处理
	BEGIN_MSG_MAP_EX(CMainDlg)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_CLOSE(OnClose)
		MSG_WM_SIZE(OnSize)
		CHAIN_MSG_MAP(SHostWnd)
		REFLECT_NOTIFICATIONS_EX()
	END_MSG_MAP()

private:
	int m_nCurIndex;
	SStringW m_sstrPDFPath;
	SStringW m_sstrOutPath;

	CPP_ThreadPool threadpool;
};
