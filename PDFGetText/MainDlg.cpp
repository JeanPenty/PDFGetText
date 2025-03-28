// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MainDlg.h"	
	
CMainDlg::CMainDlg() : SHostWnd(_T("LAYOUT:XML_MAINWND"))
{
	m_nCurIndex = 0;

	threadpool.init(1);
	threadpool.start();
}

CMainDlg::~CMainDlg()
{
	threadpool.waitForAllDone();
	threadpool.stop();
}

BOOL CMainDlg::OnInitDialog(HWND hWnd, LPARAM lParam)
{
	return 0;
}

void CMainDlg::OnLanguageBtnCN()
{
	OnLanguage(1);
}
void CMainDlg::OnLanguageBtnJP()
{
	OnLanguage(0);
}
void CMainDlg::OnLanguage(int nID)
{
	ITranslatorMgr *pTransMgr = SApplication::getSingletonPtr()->GetTranslator();
	SASSERT(pTransMgr);
	bool bCnLang = nID == 1;

	pugi::xml_document xmlLang;
	if (SApplication::getSingletonPtr()->LoadXmlDocment(xmlLang, bCnLang ? _T("translator:lang_cn") : _T("translator:lang_jp")))
	{
		CAutoRefPtr<ITranslator> lang;
		pTransMgr->CreateTranslator(&lang);
		lang->Load(&xmlLang.child(L"language"), 1);//1=LD_XML
		TCHAR lngName[TR_MAX_NAME_LEN] = {0};
		lang->GetName(lngName);
		pTransMgr->SetLanguage(lngName);
		pTransMgr->InstallTranslator(lang);
		SDispatchMessage(UM_SETLANGUAGE,0,0);
	}
}

//TODO:消息映射
void CMainDlg::OnClose()
{
	SNativeWnd::DestroyWindow();
}

void CMainDlg::OnMaximize()
{
	SendMessage(WM_SYSCOMMAND, SC_MAXIMIZE);
}
void CMainDlg::OnRestore()
{
	SendMessage(WM_SYSCOMMAND, SC_RESTORE);
}
void CMainDlg::OnMinimize()
{
	SendMessage(WM_SYSCOMMAND, SC_MINIMIZE);
}

void CMainDlg::OnSize(UINT nType, CSize size)
{
	SetMsgHandled(FALSE);	
	SWindow *pBtnMax = FindChildByName(L"btn_max");
	SWindow *pBtnRestore = FindChildByName(L"btn_restore");
	if(!pBtnMax || !pBtnRestore) return;
	
	if (nType == SIZE_MAXIMIZED)
	{
		pBtnRestore->SetVisible(TRUE);
		pBtnMax->SetVisible(FALSE);
	}
	else if (nType == SIZE_RESTORED)
	{
		pBtnRestore->SetVisible(FALSE);
		pBtnMax->SetVisible(TRUE);
	}
}

void CMainDlg::OnBnClickSelectFile()
{
	SEdit* pEdit = FindChildByName2<SEdit>(L"edit_path");
	SEdit* pContent = FindChildByName2<SEdit>(L"edit_content");

	m_nCurIndex = 0;

	CFileDialogEx openDlg(TRUE, _T("文件"), 0, 6,
		_T("pdf files\0*.pdf\0\0\0"));
	if (openDlg.DoModal() == IDOK)
	{
		SStringW sstrFilePath = openDlg.m_szFileName;
		SStringW sstrFileName = openDlg.m_szFileTitle;

		pEdit->SetWindowTextW(sstrFilePath);
		m_sstrPDFPath = sstrFilePath;

		//解析第一页的数据
		std::string strPath = S_CW2A(m_sstrPDFPath, CP_UTF8);
		fz_context* ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
		fz_register_document_handlers(ctx);
		fz_document* doc = fz_open_document(ctx, strPath.c_str());
		int nCount = fz_count_pages(ctx, doc);
		if (nCount > 0)
		{
			fz_page* page = fz_load_page(ctx, doc, m_nCurIndex);
			fz_stext_page* text_page = fz_new_stext_page(ctx, fz_bound_page(ctx, page));
			fz_device* device = fz_new_stext_device(ctx, text_page, NULL);
			fz_run_page(ctx, page, device, fz_identity, NULL);
			fz_stext_block* block;

			SStringW sstrData;
			for (block = text_page->first_block; block; block = block->next)
			{
				if (block->type == FZ_STEXT_BLOCK_TEXT)
				{
					SStringW sstrLine;
					fz_stext_line* line;
					for (line = block->u.t.first_line; line; line = line->next)
					{
						SStringW sstrContent;
						fz_stext_char* ch;
						for (ch = line->first_char; ch; ch = ch->next)
						{
							//获取字符
							SStringW sstrChar;
							if (ch->c == 65533)
								sstrData += L" ";
							else
							{
								sstrChar.Format(L"%c", ch->c);
								sstrData += sstrChar;
								sstrContent += sstrChar;
							}
						}
						sstrLine += sstrContent;
						sstrData += L"\n";
					}

					int kkk = 0;
				}
			}

			fz_drop_device(ctx, device);
			fz_drop_page(ctx, page);

			pContent->SetWindowTextW(sstrData);

			SStatic* pCurPage = FindChildByName2<SStatic>(L"txt_curpage");
			pCurPage->SetWindowTextW(L"当前页码：1");
		}
		fz_drop_document(ctx, doc);
		fz_drop_context(ctx);
	}
}

void CMainDlg::OnBnClickSelectOutput()
{
	TCHAR szSelected[MAX_PATH];//用来存放文件夹路径 
	BROWSEINFO bi;
	LPITEMIDLIST pidl;

	bi.hwndOwner = this->m_hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = szSelected;
	bi.lpszTitle = _T("选择输出文件路径");
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = NULL;
	bi.lParam = NULL;
	bi.iImage = NULL;

	SStringW sstrOutPath;
	if ((pidl = SHBrowseForFolder(&bi)) != NULL)
	{
		if (SUCCEEDED(SHGetPathFromIDList(pidl, szSelected))) //得到文件夹的全路径，不要的话，只得本文件夹名  
		{
			sstrOutPath = szSelected;  	//获得文件夹的全路径
		}
	}

	SEdit* pEditOutput = FindChildByName2<SEdit>(L"edit_output_path");
	pEditOutput->SetWindowTextW(sstrOutPath);

	m_sstrOutPath = sstrOutPath;
}


void ParasePDFText(std::string& strOutput, SStringW& sstrPDFPath)
{
	std::ofstream outFile;
	outFile.open(strOutput.c_str());

	std::string strPath = S_CW2A(sstrPDFPath, CP_UTF8);
	fz_context* ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
	fz_register_document_handlers(ctx);
	fz_document* doc = fz_open_document(ctx, strPath.c_str());
	int nCount = fz_count_pages(ctx, doc);

	for (int i = 0; i < nCount; i++)
	{
		outFile << "Page: " << i + 1 << std::endl;

		fz_page* page = fz_load_page(ctx, doc, i);
		fz_stext_page* text_page = fz_new_stext_page(ctx, fz_bound_page(ctx, page));
		fz_device* device = fz_new_stext_device(ctx, text_page, NULL);
		fz_run_page(ctx, page, device, fz_identity, NULL);
		fz_stext_block* block;

		SStringW sstrData;
		for (block = text_page->first_block; block; block = block->next)
		{
			if (block->type == FZ_STEXT_BLOCK_TEXT)
			{
				fz_stext_line* line;
				for (line = block->u.t.first_line; line; line = line->next)
				{
					fz_stext_char* ch;
					for (ch = line->first_char; ch; ch = ch->next)
					{
						//获取字符
						SStringW sstrChar;
						if (ch->c == 65533)
							sstrData += L" ";
						else
						{
							sstrChar.Format(L"%c", ch->c);
							sstrData += sstrChar;
						}
					}
					sstrData += L"\n";
				}
			}
		}

		std::string strContent = S_CW2A(sstrData);
		outFile << strContent.c_str() << std::endl;

		fz_drop_device(ctx, device);
		fz_drop_page(ctx, page);
	}
	outFile.close();

	fz_drop_document(ctx, doc);
	fz_drop_context(ctx);
}

void CMainDlg::OnBnClickParse()
{
	if (m_sstrOutPath == L"") return;
	if (m_sstrPDFPath == L"") return;
	
	std::string strName;
	std::string strPDFPath = S_CW2A(m_sstrPDFPath);
	size_t szPos = strPDFPath.find_last_of("\\");
	if (szPos != std::string::npos)
	{
		std::string strFileName = strPDFPath.substr(szPos + 1);
		size_t pos = strFileName.find_last_of(".");
		if (pos != std::string::npos)
		{
			strName = strFileName.substr(0, pos);
		}
	}

	//创建一个文本文件，  然后往文本文件中写解析出的信息
	int kkk = 0;
	std::string strOutPath = S_CW2A(m_sstrOutPath);
	std::string strOutFile = strOutPath + "\\" + strName + ".txt";
	threadpool.exec(ParasePDFText, strOutFile, m_sstrPDFPath);
}

void CMainDlg::OnBnClickPrePage()
{
	if (m_sstrPDFPath == L"") return;

	SEdit* pContent = FindChildByName2<SEdit>(L"edit_content");
	m_nCurIndex -= 1;
	if (m_nCurIndex < 0)
	{
		m_nCurIndex = 0;
		return;
	}

	std::string strPath = S_CW2A(m_sstrPDFPath, CP_UTF8);
	fz_context* ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
	fz_register_document_handlers(ctx);
	fz_document* doc = fz_open_document(ctx, strPath.c_str());
	int nCount = fz_count_pages(ctx, doc);
	if (nCount > m_nCurIndex)
	{
		fz_page* page = fz_load_page(ctx, doc, m_nCurIndex);
		fz_stext_page* text_page = fz_new_stext_page(ctx, fz_bound_page(ctx, page));
		fz_device* device = fz_new_stext_device(ctx, text_page, NULL);
		fz_run_page(ctx, page, device, fz_identity, NULL);
		fz_stext_block* block;
		SStringW sstrData;
		for (block = text_page->first_block; block; block = block->next)
		{
			if (block->type == FZ_STEXT_BLOCK_TEXT)
			{
				fz_stext_line* line;
				for (line = block->u.t.first_line; line; line = line->next)
				{
					fz_stext_char* ch;
					for (ch = line->first_char; ch; ch = ch->next)
					{
						//获取字符
						SStringW sstrChar;
						if (ch->c == 65533)
							sstrData += L" ";
						else
						{
							sstrChar.Format(L"%c", ch->c);
							sstrData += sstrChar;
						}
					}
					sstrData += L"\n";
				}
			}
		}

		fz_drop_device(ctx, device);
		fz_drop_page(ctx, page);

		pContent->SetWindowTextW(sstrData);

		SStatic* pCurPage = FindChildByName2<SStatic>(L"txt_curpage");
		SStringW sstrPage;
		sstrPage.Format(L"当前页码：%d", m_nCurIndex + 1);
		pCurPage->SetWindowTextW(sstrPage);
	}

	fz_drop_document(ctx, doc);
	fz_drop_context(ctx);
}

void CMainDlg::OnBnClickNextPage()
{
	if (m_sstrPDFPath == L"") return;

	SEdit* pContent = FindChildByName2<SEdit>(L"edit_content");
	m_nCurIndex += 1;

	std::string strPath = S_CW2A(m_sstrPDFPath, CP_UTF8);
	fz_context* ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
	fz_register_document_handlers(ctx);
	fz_document* doc = fz_open_document(ctx, strPath.c_str());
	int nCount = fz_count_pages(ctx, doc);
	if (nCount > m_nCurIndex)
	{
		fz_page* page = fz_load_page(ctx, doc, m_nCurIndex);
		fz_stext_page* text_page = fz_new_stext_page(ctx, fz_bound_page(ctx, page));
		fz_device* device = fz_new_stext_device(ctx, text_page, NULL);
		fz_run_page(ctx, page, device, fz_identity, NULL);
		fz_stext_block* block;

		SStringW sstrData;
		for (block = text_page->first_block; block; block = block->next)
		{
			if (block->type == FZ_STEXT_BLOCK_TEXT)
			{
				fz_stext_line* line;
				for (line = block->u.t.first_line; line; line = line->next)
				{
					fz_stext_char* ch;
					for (ch = line->first_char; ch; ch = ch->next)
					{
						//获取字符
						SStringW sstrChar;
						if (ch->c == 65533)
							sstrData += L" ";
						else
						{
							sstrChar.Format(L"%c", ch->c);
							sstrData += sstrChar;
						}
					}
					sstrData += L"\n";
				}
			}
		}

		fz_drop_device(ctx, device);
		fz_drop_page(ctx, page);

		pContent->SetWindowTextW(sstrData);

		SStatic* pCurPage = FindChildByName2<SStatic>(L"txt_curpage");
		SStringW sstrPage;
		sstrPage.Format(L"当前页码：%d", m_nCurIndex + 1);
		pCurPage->SetWindowTextW(sstrPage);
	}
	else
		m_nCurIndex = nCount - 1;

	fz_drop_document(ctx, doc);
	fz_drop_context(ctx);
}