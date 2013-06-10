/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  MergeEditView.cpp
 *
 * @brief Implementation of the CMergeEditView class
 */
// ID line follows -- this is updated by SVN
// $Id: MergeEditView.cpp 7142 2010-04-28 17:05:50Z kimmov $

#include "StdAfx.h"
#include <vector>
#include "BCMenu.h"
#include "Merge.h"
#include "LocationView.h"
#include "MergeEditView.h"
#include "MergeDoc.h"
#include "MainFrm.h"
#include "OptionsMgr.h"
#include "OptionsDiffColors.h"
#include "WaitStatusCursor.h"
#include "FileTransform.h"
#include "Plugins.h"
#include "lwdisp.h"
#include "WMGotoDlg.h"
#include "OptionsDef.h"
#include "SyntaxColors.h"
#include "ChildFrm.h"
#include "unicoder.h"
#include "MergeLineFlags.h"
#include "PluginsListDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using std::vector;

/** @brief Timer ID for delayed rescan. */
const UINT IDT_RESCAN = 2;
/** @brief Timer timeout for delayed rescan. */
const UINT RESCAN_TIMEOUT = 1000;

/** @brief Location for file compare specific help to open. */
static TCHAR MergeViewHelpLocation[] = _T("::/htmlhelp/Compare_files.html");

/////////////////////////////////////////////////////////////////////////////
// CMergeEditView

IMPLEMENT_DYNCREATE(CMergeEditView, CCrystalEditViewEx)

CMergeEditView::CMergeEditView()
: m_bCurrentLineIsDiff(false)
, m_pLocationView(NULL)
, m_nThisPane(0)
, m_piMergeEditStatus(0)
, m_bAutomaticRescan(false)
, fTimerWaitingForIdle(0)
{
	SetParser(&m_xParser);
	
	Options::DiffColors::Load(m_cachedColors);
}

CMergeEditView::~CMergeEditView()
{
}


BEGIN_MESSAGE_MAP(CMergeEditView, CCrystalEditViewEx)
	//{{AFX_MSG_MAP(CMergeEditView)
	ON_COMMAND(ID_CURDIFF, OnCurdiff)
	ON_UPDATE_COMMAND_UI(ID_CURDIFF, OnUpdateCurdiff)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_COMMAND(ID_FIRSTDIFF, OnFirstdiff)
	ON_UPDATE_COMMAND_UI(ID_FIRSTDIFF, OnUpdateFirstdiff)
	ON_COMMAND(ID_LASTDIFF, OnLastdiff)
	ON_UPDATE_COMMAND_UI(ID_LASTDIFF, OnUpdateLastdiff)
	ON_COMMAND(ID_NEXTDIFF, OnNextdiff)
	ON_UPDATE_COMMAND_UI(ID_NEXTDIFF, OnUpdateNextdiff)
	ON_COMMAND(ID_PREVDIFF, OnPrevdiff)
	ON_UPDATE_COMMAND_UI(ID_PREVDIFF, OnUpdatePrevdiff)
	ON_COMMAND(ID_NEXTDIFFLM, OnNextdiffLM)
	ON_UPDATE_COMMAND_UI(ID_NEXTDIFFLM, OnUpdateNextdiffLM)
	ON_COMMAND(ID_PREVDIFFLM, OnPrevdiffLM)
	ON_UPDATE_COMMAND_UI(ID_PREVDIFFLM, OnUpdatePrevdiffLM)
	ON_COMMAND(ID_NEXTDIFFLR, OnNextdiffLR)
	ON_UPDATE_COMMAND_UI(ID_NEXTDIFFLR, OnUpdateNextdiffLR)
	ON_COMMAND(ID_PREVDIFFLR, OnPrevdiffLR)
	ON_UPDATE_COMMAND_UI(ID_PREVDIFFLR, OnUpdatePrevdiffLR)
	ON_COMMAND(ID_NEXTDIFFMR, OnNextdiffMR)
	ON_UPDATE_COMMAND_UI(ID_NEXTDIFFMR, OnUpdateNextdiffMR)
	ON_COMMAND(ID_PREVDIFFMR, OnPrevdiffMR)
	ON_UPDATE_COMMAND_UI(ID_PREVDIFFMR, OnUpdatePrevdiffMR)
	ON_COMMAND(ID_NEXTDIFFLO, OnNextdiffLO)
	ON_UPDATE_COMMAND_UI(ID_NEXTDIFFLO, OnUpdateNextdiffLO)
	ON_COMMAND(ID_PREVDIFFLO, OnPrevdiffLO)
	ON_UPDATE_COMMAND_UI(ID_PREVDIFFLO, OnUpdatePrevdiffLO)
	ON_COMMAND(ID_NEXTDIFFMO, OnNextdiffMO)
	ON_UPDATE_COMMAND_UI(ID_NEXTDIFFMO, OnUpdateNextdiffMO)
	ON_COMMAND(ID_PREVDIFFMO, OnPrevdiffMO)
	ON_UPDATE_COMMAND_UI(ID_PREVDIFFMO, OnUpdatePrevdiffMO)
	ON_COMMAND(ID_NEXTDIFFRO, OnNextdiffRO)
	ON_UPDATE_COMMAND_UI(ID_NEXTDIFFRO, OnUpdateNextdiffRO)
	ON_COMMAND(ID_PREVDIFFRO, OnPrevdiffRO)
	ON_UPDATE_COMMAND_UI(ID_PREVDIFFRO, OnUpdatePrevdiffRO)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_COMMAND(ID_ALL_LEFT, OnAllLeft)
	ON_UPDATE_COMMAND_UI(ID_ALL_LEFT, OnUpdateAllLeft)
	ON_COMMAND(ID_ALL_RIGHT, OnAllRight)
	ON_UPDATE_COMMAND_UI(ID_ALL_RIGHT, OnUpdateAllRight)
	ON_COMMAND(ID_L2R, OnL2r)
	ON_UPDATE_COMMAND_UI(ID_L2R, OnUpdateL2r)
	ON_COMMAND(ID_R2L, OnR2l)
	ON_UPDATE_COMMAND_UI(ID_R2L, OnUpdateR2l)
	ON_COMMAND(ID_L2M, OnL2m)
	ON_UPDATE_COMMAND_UI(ID_L2M, OnUpdateL2m)
	ON_COMMAND(ID_R2M, OnR2m)
	ON_UPDATE_COMMAND_UI(ID_R2M, OnUpdateR2m)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_WM_TIMER()
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_LEFT, OnUpdateFileSaveLeft)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_MIDDLE, OnUpdateFileSaveMiddle)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_RIGHT, OnUpdateFileSaveRight)
	ON_COMMAND(ID_REFRESH, OnRefresh)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_COMMAND(ID_SELECTLINEDIFF, OnSelectLineDiff)
	ON_UPDATE_COMMAND_UI(ID_SELECTLINEDIFF, OnUpdateSelectLineDiff)
	ON_WM_CONTEXTMENU()
	ON_UPDATE_COMMAND_UI(ID_EDIT_REPLACE, OnUpdateEditReplace)
	ON_COMMAND(ID_FILE_LEFT_READONLY, OnLeftReadOnly)
	ON_UPDATE_COMMAND_UI(ID_FILE_LEFT_READONLY, OnUpdateLeftReadOnly)
	ON_COMMAND(ID_FILE_MIDDLE_READONLY, OnMiddleReadOnly)
	ON_UPDATE_COMMAND_UI(ID_FILE_MIDDLE_READONLY, OnUpdateMiddleReadOnly)
	ON_COMMAND(ID_FILE_RIGHT_READONLY, OnRightReadOnly)
	ON_UPDATE_COMMAND_UI(ID_FILE_RIGHT_READONLY, OnUpdateRightReadOnly)
	ON_UPDATE_COMMAND_UI(ID_STATUS_PANE0FILE_RO, OnUpdateStatusRO)
	ON_UPDATE_COMMAND_UI(ID_STATUS_PANE1FILE_RO, OnUpdateStatusRO)
	ON_UPDATE_COMMAND_UI(ID_STATUS_PANE2FILE_RO, OnUpdateStatusRO)
	ON_COMMAND_RANGE(ID_EOL_TO_DOS, ID_EOL_TO_MAC, OnConvertEolTo)
	ON_UPDATE_COMMAND_UI_RANGE(ID_EOL_TO_DOS, ID_EOL_TO_MAC, OnUpdateConvertEolTo)
	ON_UPDATE_COMMAND_UI(ID_STATUS_PANE0FILE_EOL, OnUpdateStatusEOL)
	ON_UPDATE_COMMAND_UI(ID_STATUS_PANE1FILE_EOL, OnUpdateStatusEOL)
	ON_UPDATE_COMMAND_UI(ID_STATUS_PANE2FILE_EOL, OnUpdateStatusEOL)
	ON_COMMAND(ID_L2RNEXT, OnL2RNext)
	ON_UPDATE_COMMAND_UI(ID_L2RNEXT, OnUpdateL2RNext)
	ON_COMMAND(ID_R2LNEXT, OnR2LNext)
	ON_UPDATE_COMMAND_UI(ID_R2LNEXT, OnUpdateR2LNext)
	ON_COMMAND(ID_WINDOW_CHANGE_PANE, OnChangePane)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_CHANGE_PANE, OnUpdateChangePane)
	ON_COMMAND(ID_NEXT_PANE, OnChangePane)
	ON_COMMAND(ID_EDIT_WMGOTO, OnWMGoto)
	ON_UPDATE_COMMAND_UI(ID_EDIT_WMGOTO, OnUpdateWMGoto)
	ON_COMMAND_RANGE(ID_SCRIPT_FIRST, ID_SCRIPT_LAST, OnScripts)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SCRIPT_FIRST, ID_SCRIPT_LAST, OnUpdateScripts)
	ON_COMMAND(ID_NO_PREDIFFER, OnNoPrediffer)
	ON_UPDATE_COMMAND_UI(ID_NO_PREDIFFER, OnUpdateNoPrediffer)
	ON_COMMAND_RANGE(ID_PREDIFFERS_FIRST, ID_PREDIFFERS_LAST, OnPrediffer)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PREDIFFERS_FIRST, ID_PREDIFFERS_LAST, OnUpdatePrediffer)
	ON_COMMAND(ID_FILE_MERGINGMODE, OnMergingMode)
	ON_UPDATE_COMMAND_UI(ID_FILE_MERGINGMODE, OnUpdateMergingMode)
	ON_UPDATE_COMMAND_UI(ID_STATUS_MERGINGMODE, OnUpdateMergingStatus)
	ON_WM_VSCROLL ()
	ON_WM_HSCROLL ()
	ON_COMMAND(ID_EDIT_COPY_LINENUMBERS, OnEditCopyLineNumbers)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY_LINENUMBERS, OnUpdateEditCopyLinenumbers)
	ON_COMMAND(ID_VIEW_LINEDIFFS, OnViewLineDiffs)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LINEDIFFS, OnUpdateViewLineDiffs)
	ON_COMMAND(ID_VIEW_WORDWRAP, OnViewWordWrap)
	ON_UPDATE_COMMAND_UI(ID_VIEW_WORDWRAP, OnUpdateViewWordWrap)
	ON_COMMAND(ID_VIEW_LINENUMBERS, OnViewLineNumbers)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LINENUMBERS, OnUpdateViewLineNumbers)
	ON_COMMAND(ID_FILE_OPEN_REGISTERED, OnOpenFile)
	ON_COMMAND(ID_FILE_OPEN_WITHEDITOR, OnOpenFileWithEditor)
	ON_COMMAND(ID_FILE_OPEN_WITH, OnOpenFileWith)
	ON_COMMAND(ID_VIEW_SWAPPANES, OnViewSwapPanes)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LINEDIFFS, OnUpdateViewSwapPanes)
	ON_UPDATE_COMMAND_UI(ID_NO_EDIT_SCRIPTS, OnUpdateNoEditScripts)
	ON_WM_SIZE()
	ON_WM_MOVE()
	ON_COMMAND(ID_HELP, OnHelp)
	ON_COMMAND(ID_VIEW_FILEMARGIN, OnViewMargin)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FILEMARGIN, OnUpdateViewMargin)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CHANGESCHEME, OnUpdateViewChangeScheme)
	ON_COMMAND_RANGE(ID_COLORSCHEME_FIRST, ID_COLORSCHEME_LAST, OnChangeScheme)
	ON_UPDATE_COMMAND_UI_RANGE(ID_COLORSCHEME_FIRST, ID_COLORSCHEME_LAST, OnUpdateChangeScheme)
	ON_WM_MOUSEWHEEL()
	ON_COMMAND(ID_VIEW_ZOOMIN, OnViewZoomIn)
	ON_COMMAND(ID_VIEW_ZOOMOUT, OnViewZoomOut)
	ON_COMMAND(ID_VIEW_ZOOMNORMAL, OnViewZoomNormal)
	ON_COMMAND(ID_PLUGINS_LIST, OnPluginsList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMergeEditView diagnostics

#ifdef _DEBUG
CMergeDoc* CMergeEditView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMergeDoc)));
	return (CMergeDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMergeEditView message handlers

/**
 * @brief Return text buffer for file in view
 */
CCrystalTextBuffer *CMergeEditView::LocateTextBuffer()
{
	return GetDocument()->m_ptBuf[m_nThisPane].get();
}

/**
 * @brief Update any resources necessary after a GUI language change
 */
void CMergeEditView::UpdateResources()
{
}

void CMergeEditView::PrimeListWithFile()
{
	// Set the tab size now, just in case the options change...
	// We don't update it at the end of OnOptions,
	// we can update it safely now
	SetTabSize(GetOptionsMgr()->GetInt(OPT_TAB_SIZE));
}
/**
 * @brief Return text from line given
 */
CString CMergeEditView::GetLineText(int idx)
{
	return GetLineChars(idx);
}

/**
 * @brief Return text from selection
 */
CString CMergeEditView::GetSelectedText()
{
	CPoint ptStart, ptEnd;
	CString strText;
	GetSelection(ptStart, ptEnd);
	if (ptStart != ptEnd)
		GetTextWithoutEmptys(ptStart.y, ptStart.x, ptEnd.y, ptEnd.x, strText);
	return strText;
}

/**
 * @brief Get diffs inside selection.
 * @param [out] firstDiff First diff inside selection
 * @param [out] lastDiff Last diff inside selection
 * @note -1 is returned in parameters if diffs cannot be determined
 * @todo This shouldn't be called when there is no diffs, so replace
 * first 'if' with ASSERT()?
 */
void CMergeEditView::GetFullySelectedDiffs(int & firstDiff, int & lastDiff)
{
	firstDiff = -1;
	lastDiff = -1;

	CMergeDoc *pd = GetDocument();
	const int nDiffs = pd->m_diffList.GetSignificantDiffs();
	if (nDiffs == 0)
		return;

	int firstLine, lastLine;
	GetFullySelectedLines(firstLine, lastLine);
	if (lastLine < firstLine)
		return;

	firstDiff = pd->m_diffList.NextSignificantDiffFromLine(firstLine);
	lastDiff = pd->m_diffList.PrevSignificantDiffFromLine(lastLine);
	if (firstDiff != -1 && lastDiff != -1)
	{
		DIFFRANGE di;
		
		// Check that first selected line is first diff's first line or above it
		VERIFY(pd->m_diffList.GetDiff(firstDiff, di));
		if ((int)di.dbegin[0] < firstLine)
		{
			if (firstDiff < lastDiff)
				++firstDiff;
		}

		// Check that last selected line is last diff's last line or below it
		VERIFY(pd->m_diffList.GetDiff(lastDiff, di));
		if ((int)di.dend[0] > lastLine)
		{
			if (firstDiff < lastDiff)
				--lastDiff;
		}

		// Special case: one-line diff is not selected if cursor is in it
		if (firstLine == lastLine)
		{
			firstDiff = -1;
			lastDiff = -1;
		}
	}
}

void CMergeEditView::OnInitialUpdate()
{
	CCrystalEditViewEx::OnInitialUpdate();
	SetFont(dynamic_cast<CMainFrame*>(AfxGetMainWnd())->m_lfDiff);

	m_lineBegin = 0;
	m_lineEnd = -1;
}

void CMergeEditView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	CCrystalEditViewEx::OnActivateView(bActivate, pActivateView, pDeactiveView);

	CMergeDoc* pDoc = GetDocument();
	pDoc->UpdateHeaderActivity(m_nThisPane, bActivate != false);
}

int CMergeEditView::GetAdditionalTextBlocks (int nLineIndex, TEXTBLOCK *&pBuf)
{
	pBuf = NULL;

	if (IsDetailViewPane())
	{
		if (nLineIndex < m_lineBegin || nLineIndex > m_lineEnd)
			return 0;
	}

	DWORD dwLineFlags = GetLineFlags(nLineIndex);
	if ((dwLineFlags & LF_SNP) == LF_SNP || (dwLineFlags & LF_DIFF) != LF_DIFF || (dwLineFlags & LF_MOVED) == LF_MOVED)
		return 0;

	if (!GetOptionsMgr()->GetBool(OPT_WORDDIFF_HIGHLIGHT))
		return 0;

	CMergeDoc *pDoc = GetDocument();
	if (pDoc->IsEditedAfterRescan(m_nThisPane))
		return 0;
	
	vector<WordDiff> worddiffs;
	int nDiff = pDoc->m_diffList.LineToDiff(nLineIndex);
	if (nDiff == -1)
		return 0;

	DIFFRANGE cd;
	pDoc->m_diffList.GetDiff(nDiff, cd);
	int unemptyLineCount = 0;
	for (int nPane = 0; nPane < pDoc->m_nBuffers; nPane++)
	{
		if (cd.begin[nPane] != cd.end[nPane] + 1)
			unemptyLineCount++;
	}
	if (unemptyLineCount < 2)
		return 0;

	pDoc->GetWordDiffArray(nLineIndex, &worddiffs);
	int nWordDiffs = worddiffs.size();

	bool lineInCurrentDiff = IsLineInCurrentDiff(nLineIndex);

	pBuf = new TEXTBLOCK[nWordDiffs * 2 + 1];
	pBuf[0].m_nCharPos = 0;
	pBuf[0].m_nColorIndex = COLORINDEX_NONE;
	pBuf[0].m_nBgColorIndex = COLORINDEX_NONE;
	int i, j;
	for (i = 0, j = 1; i < nWordDiffs; i++)
	{
		if (worddiffs[i].beginline[m_nThisPane] > nLineIndex || worddiffs[i].endline[m_nThisPane] < nLineIndex )
			continue;
		int begin[3], end[3];
		bool deleted = false;
		for (int pane = 0; pane < pDoc->m_nBuffers; pane++)
		{
			begin[pane] = (worddiffs[i].beginline[pane] < nLineIndex) ? 0 : worddiffs[i].begin[pane];
			end[pane]   = (worddiffs[i].endline[pane]   > nLineIndex) ? GetGroupView(pane)->GetLineLength(nLineIndex) : worddiffs[i].end[pane];
			if (worddiffs[i].beginline[pane] == worddiffs[i].endline[pane] &&
				worddiffs[i].begin[pane] == worddiffs[i].end[pane])
				deleted = true;
		}
		pBuf[j].m_nCharPos = begin[m_nThisPane];
		if (lineInCurrentDiff)
		{
			pBuf[j].m_nColorIndex = COLORINDEX_HIGHLIGHTTEXT1 | COLORINDEX_APPLYFORCE;
			pBuf[j].m_nBgColorIndex = COLORINDEX_APPLYFORCE | 
				(deleted ? COLORINDEX_HIGHLIGHTBKGND4 : COLORINDEX_HIGHLIGHTBKGND1);
		}
		else
		{
			pBuf[j].m_nColorIndex = COLORINDEX_HIGHLIGHTTEXT2 | COLORINDEX_APPLYFORCE;
			pBuf[j].m_nBgColorIndex = COLORINDEX_APPLYFORCE |
				(deleted ? COLORINDEX_HIGHLIGHTBKGND3 : COLORINDEX_HIGHLIGHTBKGND2);
		}
		j++;
		pBuf[j].m_nCharPos = end[m_nThisPane];
		pBuf[j].m_nColorIndex = COLORINDEX_NONE;
		pBuf[j].m_nBgColorIndex = COLORINDEX_NONE;
		j++;
	}

	return j;
}

COLORREF CMergeEditView::GetColor(int nColorIndex)
{
	switch (nColorIndex & ~COLORINDEX_APPLYFORCE)
	{
	case COLORINDEX_HIGHLIGHTBKGND1:
		return m_cachedColors.clrSelWordDiff;
	case COLORINDEX_HIGHLIGHTTEXT1:
		return m_cachedColors.clrSelWordDiffText;
	case COLORINDEX_HIGHLIGHTBKGND2:
		return m_cachedColors.clrWordDiff;
	case COLORINDEX_HIGHLIGHTTEXT2:
		return m_cachedColors.clrWordDiffText;
	case COLORINDEX_HIGHLIGHTBKGND3:
		return m_cachedColors.clrWordDiffDeleted;
	case COLORINDEX_HIGHLIGHTBKGND4:
		return m_cachedColors.clrSelWordDiffDeleted;

	default:
		return CCrystalTextView::GetColor(nColorIndex);
	}
}

/**
 * @brief Determine text and background color for line
 * @param [in] nLineIndex Index of line in view (NOT line in file)
 * @param [out] crBkgnd Backround color for line
 * @param [out] crText Text color for line
 */
void CMergeEditView::GetLineColors(int nLineIndex, COLORREF & crBkgnd,
                                COLORREF & crText, bool & bDrawWhitespace)
{
	DWORD ignoreFlags = 0;
	GetLineColors2(nLineIndex, ignoreFlags, crBkgnd, crText, bDrawWhitespace);
}

/**
 * @brief Determine text and background color for line
 * @param [in] nLineIndex Index of line in view (NOT line in file)
 * @param [in] ignoreFlags Flags that caller wishes ignored
 * @param [out] crBkgnd Backround color for line
 * @param [out] crText Text color for line
 *
 * This version allows caller to suppress particular flags
 */
void CMergeEditView::GetLineColors2(int nLineIndex, DWORD ignoreFlags, COLORREF & crBkgnd,
                                COLORREF & crText, bool & bDrawWhitespace)
{
	if (GetLineCount() <= nLineIndex)
		return;

	DWORD dwLineFlags = GetLineFlags(nLineIndex);

	if (dwLineFlags & ignoreFlags)
		dwLineFlags &= (~ignoreFlags);

	if (IsDetailViewPane())
	{
		// Line with WinMerge flag, 
		// Lines with only the LF_DIFF/LF_TRIVIAL flags are not colored with Winmerge colors
		if (dwLineFlags & (LF_WINMERGE_FLAGS & ~LF_DIFF & ~LF_TRIVIAL & ~LF_MOVED & ~LF_SNP))
		{
			crText = m_cachedColors.clrDiffText;
			bDrawWhitespace = true;

			if (dwLineFlags & LF_GHOST)
			{
				crBkgnd = m_cachedColors.clrDiffDeleted;
			}
		}
		else
		{
			// If no syntax hilighting
			if (!GetOptionsMgr()->GetBool(OPT_SYNTAX_HIGHLIGHT))
			{
				crBkgnd = GetColor (COLORINDEX_BKGND);
				crText = GetColor (COLORINDEX_NORMALTEXT);
				bDrawWhitespace = false;
			}
			else
				// Line not inside diff, get colors from CrystalEditor
				CCrystalEditViewEx::GetLineColors(nLineIndex, crBkgnd,
					crText, bDrawWhitespace);
		}
		if (nLineIndex < m_lineBegin || nLineIndex > m_lineEnd)
		{
			crBkgnd = GetColor (COLORINDEX_WHITESPACE);
			crText = GetColor (COLORINDEX_WHITESPACE);
			bDrawWhitespace = false;
		}
		return;
	}

	// Line inside diff
	if (dwLineFlags & LF_WINMERGE_FLAGS)
	{
		crText = m_cachedColors.clrDiffText;
		bDrawWhitespace = true;
		bool lineInCurrentDiff = IsLineInCurrentDiff(nLineIndex);

		if (dwLineFlags & LF_SNP)
		{
			if (lineInCurrentDiff)
			{
				if (dwLineFlags & LF_GHOST)
					crBkgnd = m_cachedColors.clrSelSNPDeleted;
				else
					crBkgnd = m_cachedColors.clrSelSNP;
				crText = m_cachedColors.clrSelSNPText;
			}
			else
			{
				if (dwLineFlags & LF_GHOST)
					crBkgnd = m_cachedColors.clrSNPDeleted;
				else
					crBkgnd = m_cachedColors.clrSNP;
				crText = m_cachedColors.clrSNPText;
			}
			return;
		}
		else if (dwLineFlags & LF_DIFF)
		{
			if (lineInCurrentDiff)
			{
				if (dwLineFlags & LF_MOVED)
				{
					if (dwLineFlags & LF_GHOST)
						crBkgnd = m_cachedColors.clrSelMovedDeleted;
					else
						crBkgnd = m_cachedColors.clrSelMoved;
					crText = m_cachedColors.clrSelMovedText;
				}
				else
				{
					crBkgnd = m_cachedColors.clrSelDiff;
					crText = m_cachedColors.clrSelDiffText;
				}
			
			}
			else
			{
				if (dwLineFlags & LF_MOVED)
				{
					if (dwLineFlags & LF_GHOST)
						crBkgnd = m_cachedColors.clrMovedDeleted;
					else
						crBkgnd = m_cachedColors.clrMoved;
					crText = m_cachedColors.clrMovedText;
				}
				else
				{
					crBkgnd = m_cachedColors.clrDiff;
					crText = m_cachedColors.clrDiffText;
				}
			}
			return;
		}
		else if (dwLineFlags & LF_TRIVIAL)
		{
			// trivial diff can not be selected
			if (dwLineFlags & LF_GHOST)
				// ghost lines in trivial diff has their own color
				crBkgnd = m_cachedColors.clrTrivialDeleted;
			else
				crBkgnd = m_cachedColors.clrTrivial;
			crText = m_cachedColors.clrTrivialText;
			return;
		}
		else if (dwLineFlags & LF_GHOST)
		{
			if (lineInCurrentDiff)
				crBkgnd = m_cachedColors.clrSelDiffDeleted;
			else
				crBkgnd = m_cachedColors.clrDiffDeleted;
			return;
		}
	}
	else
	{
		// Line not inside diff,
		if (!GetOptionsMgr()->GetBool(OPT_SYNTAX_HIGHLIGHT))
		{
			// If no syntax hilighting, get windows default colors
			crBkgnd = GetColor (COLORINDEX_BKGND);
			crText = GetColor (COLORINDEX_NORMALTEXT);
			bDrawWhitespace = false;
		}
		else
			// Syntax highlighting, get colors from CrystalEditor
			CCrystalEditViewEx::GetLineColors(nLineIndex, crBkgnd,
				crText, bDrawWhitespace);
	}
}

/**
 * @brief Sync other pane position
 */
void CMergeEditView::UpdateSiblingScrollPos (bool bHorz)
{
	CSplitterWnd *pSplitterWnd = GetParentSplitter (this, false);
	if (pSplitterWnd != NULL)
	{
		//  See CSplitterWnd::IdFromRowCol() implementation for details
		int nCurrentRow = (GetDlgCtrlID () - AFX_IDW_PANE_FIRST) / 16;
		int nCurrentCol = (GetDlgCtrlID () - AFX_IDW_PANE_FIRST) % 16;
		ASSERT (nCurrentRow >= 0 && nCurrentRow < pSplitterWnd->GetRowCount ());
		ASSERT (nCurrentCol >= 0 && nCurrentCol < pSplitterWnd->GetColumnCount ());

		// limit the TopLine : must be smaller than GetLineCount for all the panels
		int newTopSubLine = m_nTopSubLine;
		int nRows = pSplitterWnd->GetRowCount ();
		int nCols = pSplitterWnd->GetColumnCount ();
		int nRow=0;
//		for (nRow = 0; nRow < nRows; nRow++)
//		{
//			for (int nCol = 0; nCol < nCols; nCol++)
//			{
//				CMergeEditView *pSiblingView = static_cast<CMergeEditView*>(GetSiblingView (nRow, nCol));
//				if (pSiblingView != NULL)
//					if (pSiblingView->GetSubLineCount() <= newTopSubLine)
//						newTopSubLine = pSiblingView->GetSubLineCount()-1;
//			}
//		}
		if (m_nTopSubLine != newTopSubLine)
			ScrollToSubLine(newTopSubLine);

		for (nRow = 0; nRow < nRows; nRow++)
		{
			for (int nCol = 0; nCol < nCols; nCol++)
			{
				if (!(nRow == nCurrentRow && nCol == nCurrentCol))  //  We don't need to update ourselves
				{
					CMergeEditView *pSiblingView = static_cast<CMergeEditView*>(GetSiblingView (nRow, nCol));
					if (pSiblingView != NULL)
						pSiblingView->OnUpdateSibling (this, bHorz);
				}
			}
		}
	}
}

/**
 * @brief Update other panes
 */
void CMergeEditView::OnUpdateSibling (CCrystalTextView * pUpdateSource, BOOL bHorz)
{
	if (pUpdateSource != this)
	{
		ASSERT (pUpdateSource != NULL);
		ASSERT_KINDOF (CCrystalTextView, pUpdateSource);
		CMergeEditView *pSrcView = static_cast<CMergeEditView*>(pUpdateSource);
		if (!bHorz)  // changed this so bHorz works right
		{
			ASSERT (pSrcView->m_nTopSubLine >= 0);

			// This ASSERT is wrong: panes have different files and
			// different linecounts
			// ASSERT (pSrcView->m_nTopLine < GetLineCount ());
			if (pSrcView->m_nTopSubLine != m_nTopSubLine)
			{
				ScrollToSubLine (pSrcView->m_nTopSubLine, true, false);
				UpdateCaret ();
				RecalcVertScrollBar(true);
			}
		}
		else
		{
			ASSERT (pSrcView->m_nOffsetChar >= 0);

			// This ASSERT is wrong: panes have different files and
			// different linelengths
			// ASSERT (pSrcView->m_nOffsetChar < GetMaxLineLength ());
			if (pSrcView->m_nOffsetChar != m_nOffsetChar)
			{
				ScrollToChar (pSrcView->m_nOffsetChar, true, false);
				UpdateCaret ();
				RecalcHorzScrollBar(true);
			}
		}
	}
}

void CMergeEditView::OnDisplayDiff(int nDiff /*=0*/)
{
	int newlineBegin, newlineEnd;
	CMergeDoc *pd = GetDocument();
	if (nDiff < 0 || nDiff >= pd->m_diffList.GetSize())
	{
		newlineBegin = 0;
		newlineEnd = -1;
	}
	else
	{
		DIFFRANGE curDiff;
		VERIFY(pd->m_diffList.GetDiff(nDiff, curDiff));

		newlineBegin = curDiff.dbegin[0];
		ASSERT (newlineBegin >= 0);
		newlineEnd = curDiff.dend[0];
	}

	if (newlineBegin == m_lineBegin && newlineEnd == m_lineEnd)
		return;
	m_lineBegin = newlineBegin;
	m_lineEnd = newlineEnd;

	// scroll to the first line of the diff
	ScrollToLine(m_lineBegin);

	// tell the others views about this diff (no need to call UpdateSiblingScrollPos)
	CSplitterWnd *pSplitterWnd = GetParentSplitter(this, false);

	// pSplitterWnd is NULL if WinMerge started minimized.
	if (pSplitterWnd != NULL)
	{
		int nRows = pSplitterWnd->GetRowCount ();
		int nCols = pSplitterWnd->GetColumnCount ();
		for (int nRow = 0; nRow < nRows; nRow++)
		{
			for (int nCol = 0; nCol < nCols; nCol++)
			{
				CMergeEditView *pSiblingView = static_cast<CMergeEditView*>(GetSiblingView (nRow, nCol));
				if (pSiblingView != NULL)
					pSiblingView->OnDisplayDiff(nDiff);
			}
		}
	}

	// update the width of the horizontal scrollbar
	RecalcHorzScrollBar();
}

/**
 * @brief Selects diff by number and syncs other file
 * @param [in] nDiff Diff to select, must be >= 0
 * @param [in] bScroll Scroll diff to view
 * @param [in] bSelectText Select diff text
 * @sa CMergeEditView::ShowDiff()
 * @sa CMergeDoc::SetCurrentDiff()
 * @todo Parameter bSelectText is never used?
 */
void CMergeEditView::SelectDiff(int nDiff, bool bScroll /*=true*/, bool bSelectText /*=true*/)
{
	CMergeDoc *pd = GetDocument();

	// Check that nDiff is valid
	if (nDiff < 0)
		_RPTF1(_CRT_ERROR, "Diffnumber negative (%d)", nDiff);
	if (nDiff >= pd->m_diffList.GetSize())
		_RPTF2(_CRT_ERROR, "Selected diff > diffcount (%d >= %d)",
			nDiff, pd->m_diffList.GetSize());

	SelectNone();
	pd->SetCurrentDiff(nDiff);
	ShowDiff(bScroll, bSelectText);
	pd->UpdateAllViews(this);
	UpdateSiblingScrollPos(false);

	// notify either side, as it will notify the other one
	if (!IsDetailViewPane())
		pd->GetDetailView(0)->OnDisplayDiff(nDiff);
}

/**
 * @brief Called when user selects "Current Difference".
 * Goes to active diff. If no active diff, selects diff under cursor
 * @sa CMergeEditView::SelectDiff()
 * @sa CMergeDoc::GetCurrentDiff()
 * @sa CMergeDoc::LineToDiff()
 */
void CMergeEditView::OnCurdiff()
{
	CMergeDoc *pd = GetDocument();

	// If no diffs, nothing to select
	if (!pd->m_diffList.HasSignificantDiffs())
		return;

	// GetCurrentDiff() returns -1 if no diff selected
	int nDiff = pd->GetCurrentDiff();
	if (nDiff != -1)
	{
		// Scroll to the first line of the currently selected diff
		SelectDiff(nDiff, true, false);
	}
	else
	{
		// If cursor is inside diff, select that diff
		CPoint pos = GetCursorPos();
		nDiff = pd->m_diffList.LineToDiff(pos.y);
		if (nDiff != -1 && pd->m_diffList.IsDiffSignificant(nDiff))
			SelectDiff(nDiff, true, false);
	}
}

/**
 * @brief Called when "Current diff" item is updated
 */
void CMergeEditView::OnUpdateCurdiff(CCmdUI* pCmdUI)
{
	CMergeDoc *pd = GetDocument();
	CPoint pos = GetCursorPos();
	int nCurrentDiff = pd->GetCurrentDiff();
	if (nCurrentDiff == -1)
	{
		int nNewDiff = pd->m_diffList.LineToDiff(pos.y);
		if (nNewDiff != -1 && pd->m_diffList.IsDiffSignificant(nNewDiff))
			pCmdUI->Enable(true);
		else
			pCmdUI->Enable(false);
	}
	else
		pCmdUI->Enable(true);
}

/**
 * @brief Copy selected text to clipboard
 */
void CMergeEditView::OnEditCopy()
{
	CMergeDoc * pDoc = GetDocument();
	CPoint ptSelStart, ptSelEnd;
	GetSelection(ptSelStart, ptSelEnd);

	// Nothing selected
	if (ptSelStart == ptSelEnd)
		return;

	CString text;

	if (!m_bColumnSelection)
	{
		CDiffTextBuffer * buffer = pDoc->m_ptBuf[m_nThisPane].get();

		buffer->GetTextWithoutEmptys(ptSelStart.y, ptSelStart.x,
			ptSelEnd.y, ptSelEnd.x, text);
	}
	else
		GetTextWithoutEmptysInColumnSelection(text);

	PutToClipboard(text, text.GetLength(), m_bColumnSelection);
}

/**
 * @brief Called when "Copy" item is updated
 */
void CMergeEditView::OnUpdateEditCopy(CCmdUI* pCmdUI)
{
	CCrystalEditViewEx::OnUpdateEditCopy(pCmdUI);
}

/**
 * @brief Cut current selection to clipboard
 */
void CMergeEditView::OnEditCut()
{
	if (IsReadOnly(m_nThisPane))
		return;

	CPoint ptSelStart, ptSelEnd;
	CMergeDoc * pDoc = GetDocument();
	GetSelection(ptSelStart, ptSelEnd);

	// Nothing selected
	if (ptSelStart == ptSelEnd)
		return;

	CString text;
	if (!m_bColumnSelection)
		pDoc->m_ptBuf[m_nThisPane]->GetTextWithoutEmptys(ptSelStart.y, ptSelStart.x,
			ptSelEnd.y, ptSelEnd.x, text);
	else
		GetTextWithoutEmptysInColumnSelection(text);

	PutToClipboard(text, text.GetLength(), m_bColumnSelection);

	if (!m_bColumnSelection)
	{
		CPoint ptCursorPos = ptSelStart;
		ASSERT_VALIDTEXTPOS(ptCursorPos);
		SetAnchor(ptCursorPos);
		SetSelection(ptCursorPos, ptCursorPos);
		SetCursorPos(ptCursorPos);
		EnsureVisible(ptCursorPos);

		pDoc->m_ptBuf[m_nThisPane]->DeleteText(this, ptSelStart.y, ptSelStart.x, ptSelEnd.y,
			ptSelEnd.x, CE_ACTION_CUT);
	}
	else
		DeleteCurrentColumnSelection (CE_ACTION_CUT);

	m_pTextBuffer->SetModified(true);
}

/**
 * @brief Called when "Cut" item is updated
 */
void CMergeEditView::OnUpdateEditCut(CCmdUI* pCmdUI)
{
	if (!IsReadOnly(m_nThisPane))
		CCrystalEditViewEx::OnUpdateEditCut(pCmdUI);
	else
		pCmdUI->Enable(false);
}

/**
 * @brief Paste text from clipboard
 */
void CMergeEditView::OnEditPaste()
{
	if (IsReadOnly(m_nThisPane))
		return;

	CCrystalEditViewEx::Paste();
	m_pTextBuffer->SetModified(true);
}

/**
 * @brief Called when "Paste" item is updated
 */
void CMergeEditView::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
	if (!IsReadOnly(m_nThisPane))
		CCrystalEditViewEx::OnUpdateEditPaste(pCmdUI);
	else
		pCmdUI->Enable(false);
}

/**
 * @brief Undo last action
 */
void CMergeEditView::OnEditUndo()
{
	WaitStatusCursor waitstatus(_("Undoing the last operation..."));
	CMergeDoc* pDoc = GetDocument();
	CMergeEditView *tgt = *(pDoc->curUndo-1);
	if(tgt==this)
	{
		if (IsReadOnly(m_nThisPane))
			return;

		GetParentFrame()->SetActiveView(this, true);
		if(CCrystalEditViewEx::DoEditUndo())
		{
			--pDoc->curUndo;
			pDoc->UpdateHeaderPath(m_nThisPane);
			pDoc->FlushAndRescan();

			int nAction;
			m_pTextBuffer->GetRedoActionCode(nAction);
			if (nAction == CE_ACTION_MERGE)
				// select the diff so we may just merge it again
				OnCurdiff();
		}
	}
	else
	{
		tgt->SendMessage(WM_COMMAND, ID_EDIT_UNDO);
	}
}

/**
 * @brief Called when "Undo" item is updated
 */
void CMergeEditView::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
	CMergeDoc* pDoc = GetDocument();
	if (pDoc->curUndo!=pDoc->undoTgt.begin())
	{
		CMergeEditView *tgt = *(pDoc->curUndo-1);
		pCmdUI->Enable( !IsReadOnly(tgt->m_nThisPane));
	}
	else
		pCmdUI->Enable(false);
}

/**
 * @brief Go to first diff
 *
 * Called when user selects "First Difference"
 * @sa CMergeEditView::SelectDiff()
 */
void CMergeEditView::OnFirstdiff()
{
	CMergeDoc *pd = GetDocument();
	if (pd->m_diffList.HasSignificantDiffs())
	{
		int nDiff = pd->m_diffList.FirstSignificantDiff();
		SelectDiff(nDiff, true, false);
	}
}

/**
 * @brief Update "First diff" UI items
 */
void CMergeEditView::OnUpdateFirstdiff(CCmdUI* pCmdUI)
{
	OnUpdatePrevdiff(pCmdUI);
}

/**
 * @brief Go to last diff
 */
void CMergeEditView::OnLastdiff()
{
	CMergeDoc *pd = GetDocument();
	if (pd->m_diffList.HasSignificantDiffs())
	{
		int nDiff = pd->m_diffList.LastSignificantDiff();
		SelectDiff(nDiff, true, false);
	}
}

/**
 * @brief Update "Last diff" UI items
 */
void CMergeEditView::OnUpdateLastdiff(CCmdUI* pCmdUI)
{
	OnUpdateNextdiff(pCmdUI);
}

/**
 * @brief Go to next diff and select it.
 *
 * Finds and selects next difference. There are several cases:
 * - if there is selected difference, and that difference is visible
 * on screen, next found difference is selected.
 * - if there is selected difference but it is not visible, next
 * difference from cursor position is selected. This is what user
 * expects to happen and is natural thing to do. Also reduces
 * needless scrolling.
 * - if there is no selected difference, next difference from cursor
 * position is selected.
 */
void CMergeEditView::OnNextdiff()
{
	CMergeDoc *pd = GetDocument();
	int cnt = pd->m_ptBuf[0]->GetLineCount();
	if (cnt <= 0)
		return;

	// Returns -1 if no diff selected
	int curDiff = pd->GetCurrentDiff();
	if (curDiff != -1)
	{
		// We're on a diff
		int nextDiff = curDiff;
		if (!IsDiffVisible(curDiff))
		{
			// Selected difference not visible, select next from cursor
			int line = GetCursorPos().y;
			// Make sure we aren't in the first line of the diff
			++line;
			if (!IsValidTextPosY(CPoint(0, line)))
				line = m_nTopLine;
			nextDiff = pd->m_diffList.NextSignificantDiffFromLine(line);
		}
		else
		{
			// Find out if there is a following significant diff
			if (curDiff < pd->m_diffList.GetSize() - 1)
			{
				nextDiff = pd->m_diffList.NextSignificantDiff(curDiff);
			}
		}
		if (nextDiff == -1)
			nextDiff = curDiff;

		// nextDiff is the next one if there is one, else it is the one we're on
		SelectDiff(nextDiff, true, false);
	}
	else
	{
		// We don't have a selected difference,
		// but cursor can be inside inactive diff
		int line = GetCursorPos().y;
		if (!IsValidTextPosY(CPoint(0, line)))
			line = m_nTopLine;
		curDiff = pd->m_diffList.NextSignificantDiffFromLine(line);
		if (curDiff >= 0)
			SelectDiff(curDiff, true, false);
	}
}

/**
 * @brief Update "Next diff" UI items
 */
void CMergeEditView::OnUpdateNextdiff(CCmdUI* pCmdUI)
{
	CMergeDoc *pd = GetDocument();
	const DIFFRANGE * dfi = pd->m_diffList.LastSignificantDiffRange();

	if (!dfi)
	{
		// There aren't any significant differences
		pCmdUI->Enable(false);
	}
	else
	{
		// Enable if the beginning of the last significant difference is after caret
		CPoint pos = GetCursorPos();
		pCmdUI->Enable(pos.y < (long)dfi->dbegin[0]);
	}
}

/**
 * @brief Go to previous diff and select it.
 *
 * Finds and selects previous difference. There are several cases:
 * - if there is selected difference, and that difference is visible
 * on screen, previous found difference is selected.
 * - if there is selected difference but it is not visible, previous
 * difference from cursor position is selected. This is what user
 * expects to happen and is natural thing to do. Also reduces
 * needless scrolling.
 * - if there is no selected difference, previous difference from cursor
 * position is selected.
 */
void CMergeEditView::OnPrevdiff()
{
	CMergeDoc *pd = GetDocument();
	int cnt = pd->m_ptBuf[0]->GetLineCount();
	if (cnt <= 0)
		return;

	// GetCurrentDiff() returns -1 if no diff selected
	int curDiff = pd->GetCurrentDiff();
	if (curDiff != -1)
	{
		// We're on a diff
		int prevDiff = curDiff;
		if (!IsDiffVisible(curDiff))
		{
			// Selected difference not visible, select previous from cursor
			int line = GetCursorPos().y;
			// Make sure we aren't in the last line of the diff
			--line;
			if (!IsValidTextPosY(CPoint(0, line)))
				line = m_nTopLine;
			prevDiff = pd->m_diffList.PrevSignificantDiffFromLine(line);
		}
		else
		{
			// Find out if there is a preceding significant diff
			if (curDiff > 0)
			{
				prevDiff = pd->m_diffList.PrevSignificantDiff(curDiff);
			}
		}
		if (prevDiff == -1)
			prevDiff = curDiff;

		// prevDiff is the preceding one if there is one, else it is the one we're on
		SelectDiff(prevDiff, true, false);
	}
	else
	{
		// We don't have a selected difference,
		// but cursor can be inside inactive diff
		int line = GetCursorPos().y;
		if (!IsValidTextPosY(CPoint(0, line)))
			line = m_nTopLine;
		curDiff = pd->m_diffList.PrevSignificantDiffFromLine(line);
		if (curDiff >= 0)
			SelectDiff(curDiff, true, false);
	}
}

/**
 * @brief Update "Previous diff" UI items
 */
void CMergeEditView::OnUpdatePrevdiff(CCmdUI* pCmdUI)
{
	CMergeDoc *pd = GetDocument();
	const DIFFRANGE * dfi = pd->m_diffList.FirstSignificantDiffRange();

	if (!dfi)
	{
		// There aren't any significant differences
		pCmdUI->Enable(false);
	}
	else
	{
		// Enable if the end of the first significant difference is before caret
		CPoint pos = GetCursorPos();
		pCmdUI->Enable(pos.y > (long)dfi->dend[0]);
	}
}

/**
 * @brief Go to next 3-way diff and select it.
 */
void CMergeEditView::OnNext3wayDiff(int nDiffType)
{
	CMergeDoc *pd = GetDocument();
	int cnt = pd->m_ptBuf[0]->GetLineCount();
	if (cnt <= 0)
		return;

	// Returns -1 if no diff selected
	int curDiff = pd->GetCurrentDiff();
	if (curDiff != -1)
	{
		// We're on a diff
		int nextDiff = curDiff;
		if (!IsDiffVisible(curDiff))
		{
			// Selected difference not visible, select next from cursor
			int line = GetCursorPos().y;
			// Make sure we aren't in the first line of the diff
			++line;
			if (!IsValidTextPosY(CPoint(0, line)))
				line = m_nTopLine;
			nextDiff = pd->m_diffList.NextSignificant3wayDiffFromLine(line, nDiffType);
		}
		else
		{
			// Find out if there is a following significant diff
			if (curDiff < pd->m_diffList.GetSize() - 1)
			{
				nextDiff = pd->m_diffList.NextSignificant3wayDiff(curDiff, nDiffType);
			}
		}
		if (nextDiff == -1)
			nextDiff = curDiff;

		// nextDiff is the next one if there is one, else it is the one we're on
		SelectDiff(nextDiff, true, false);
	}
	else
	{
		// We don't have a selected difference,
		// but cursor can be inside inactive diff
		int line = GetCursorPos().y;
		if (!IsValidTextPosY(CPoint(0, line)))
			line = m_nTopLine;
		curDiff = pd->m_diffList.NextSignificant3wayDiffFromLine(line, nDiffType);
		if (curDiff >= 0)
			SelectDiff(curDiff, true, false);
	}
}

/**
 * @brief Update "Next 3-way diff" UI items
 */
void CMergeEditView::OnUpdateNext3wayDiff(CCmdUI* pCmdUI, int nDiffType)
{
	CMergeDoc *pd = GetDocument();

	if (pd->m_nBuffers < 3)
	{
		pCmdUI->Enable(false);
		return;
	}

	const DIFFRANGE * dfi = pd->m_diffList.LastSignificant3wayDiffRange(nDiffType);

	if (!dfi)
	{
		// There aren't any significant differences
		pCmdUI->Enable(false);
	}
	else
	{
		// Enable if the beginning of the last significant difference is after caret
		CPoint pos = GetCursorPos();
		pCmdUI->Enable(pos.y < (long)dfi->dbegin[0]);
	}
}

/**
 * @brief Go to previous 3-way diff and select it.
 */
void CMergeEditView::OnPrev3wayDiff(int nDiffType)
{
	CMergeDoc *pd = GetDocument();

	int cnt = pd->m_ptBuf[0]->GetLineCount();
	if (cnt <= 0)
		return;

	// GetCurrentDiff() returns -1 if no diff selected
	int curDiff = pd->GetCurrentDiff();
	if (curDiff != -1)
	{
		// We're on a diff
		int prevDiff = curDiff;
		if (!IsDiffVisible(curDiff))
		{
			// Selected difference not visible, select previous from cursor
			int line = GetCursorPos().y;
			// Make sure we aren't in the last line of the diff
			--line;
			if (!IsValidTextPosY(CPoint(0, line)))
				line = m_nTopLine;
			prevDiff = pd->m_diffList.PrevSignificant3wayDiffFromLine(line, nDiffType);
		}
		else
		{
			// Find out if there is a preceding significant diff
			if (curDiff > 0)
			{
				prevDiff = pd->m_diffList.PrevSignificant3wayDiff(curDiff, nDiffType);
			}
		}
		if (prevDiff == -1)
			prevDiff = curDiff;

		// prevDiff is the preceding one if there is one, else it is the one we're on
		SelectDiff(prevDiff, true, false);
	}
	else
	{
		// We don't have a selected difference,
		// but cursor can be inside inactive diff
		int line = GetCursorPos().y;
		if (!IsValidTextPosY(CPoint(0, line)))
			line = m_nTopLine;
		curDiff = pd->m_diffList.PrevSignificant3wayDiffFromLine(line, nDiffType);
		if (curDiff >= 0)
			SelectDiff(curDiff, true, false);
	}
}

/**
 * @brief Update "Previous diff X and Y" UI items
 */
void CMergeEditView::OnUpdatePrev3wayDiff(CCmdUI* pCmdUI, int nDiffType)
{
	CMergeDoc *pd = GetDocument();

	if (pd->m_nBuffers < 3)
	{
		pCmdUI->Enable(false);
		return;
	}

	const DIFFRANGE * dfi = pd->m_diffList.FirstSignificant3wayDiffRange(nDiffType);

	if (!dfi)
	{
		// There aren't any significant differences
		pCmdUI->Enable(false);
	}
	else
	{
		// Enable if the end of the first significant difference is before caret
		CPoint pos = GetCursorPos();
		pCmdUI->Enable(pos.y > (long)dfi->dend[0]);
	}
}

void CMergeEditView::OnNextdiffLM()
{
	OnNext3wayDiff(THREEWAYDIFFTYPE_LEFTMIDDLE);
}

void CMergeEditView::OnUpdateNextdiffLM(CCmdUI* pCmdUI)
{
	OnUpdateNext3wayDiff(pCmdUI, THREEWAYDIFFTYPE_LEFTMIDDLE);
}

void CMergeEditView::OnNextdiffLR()
{
	OnNext3wayDiff(THREEWAYDIFFTYPE_LEFTRIGHT);
}

void CMergeEditView::OnUpdateNextdiffLR(CCmdUI* pCmdUI)
{
	OnUpdateNext3wayDiff(pCmdUI, THREEWAYDIFFTYPE_LEFTRIGHT);
}

void CMergeEditView::OnNextdiffMR()
{
	OnNext3wayDiff(THREEWAYDIFFTYPE_MIDDLERIGHT);
}

void CMergeEditView::OnUpdateNextdiffMR(CCmdUI* pCmdUI)
{
	OnUpdateNext3wayDiff(pCmdUI, THREEWAYDIFFTYPE_MIDDLERIGHT);
}

void CMergeEditView::OnNextdiffLO()
{
	OnNext3wayDiff(THREEWAYDIFFTYPE_LEFTONLY);
}

void CMergeEditView::OnUpdateNextdiffLO(CCmdUI* pCmdUI)
{
	OnUpdateNext3wayDiff(pCmdUI, THREEWAYDIFFTYPE_LEFTONLY);
}

void CMergeEditView::OnNextdiffMO()
{
	OnNext3wayDiff(THREEWAYDIFFTYPE_MIDDLEONLY);
}

void CMergeEditView::OnUpdateNextdiffMO(CCmdUI* pCmdUI)
{
	OnUpdateNext3wayDiff(pCmdUI, THREEWAYDIFFTYPE_MIDDLEONLY);
}

void CMergeEditView::OnNextdiffRO()
{
	OnNext3wayDiff(THREEWAYDIFFTYPE_RIGHTONLY);
}

void CMergeEditView::OnUpdateNextdiffRO(CCmdUI* pCmdUI)
{
	OnUpdateNext3wayDiff(pCmdUI, THREEWAYDIFFTYPE_RIGHTONLY);
}

void CMergeEditView::OnPrevdiffLM()
{
	OnPrev3wayDiff(THREEWAYDIFFTYPE_LEFTMIDDLE);
}

void CMergeEditView::OnUpdatePrevdiffLM(CCmdUI* pCmdUI)
{
	OnUpdatePrev3wayDiff(pCmdUI, THREEWAYDIFFTYPE_LEFTMIDDLE);
}

void CMergeEditView::OnPrevdiffLR()
{
	OnPrev3wayDiff(THREEWAYDIFFTYPE_LEFTRIGHT);
}

void CMergeEditView::OnUpdatePrevdiffLR(CCmdUI* pCmdUI)
{
	OnUpdatePrev3wayDiff(pCmdUI, THREEWAYDIFFTYPE_LEFTRIGHT);
}

void CMergeEditView::OnPrevdiffMR()
{
	OnPrev3wayDiff(THREEWAYDIFFTYPE_MIDDLERIGHT);
}

void CMergeEditView::OnUpdatePrevdiffMR(CCmdUI* pCmdUI)
{
	OnUpdatePrev3wayDiff(pCmdUI, THREEWAYDIFFTYPE_MIDDLERIGHT);
}

void CMergeEditView::OnPrevdiffLO()
{
	OnPrev3wayDiff(THREEWAYDIFFTYPE_LEFTONLY);
}

void CMergeEditView::OnUpdatePrevdiffLO(CCmdUI* pCmdUI)
{
	OnUpdatePrev3wayDiff(pCmdUI, THREEWAYDIFFTYPE_LEFTONLY);
}

void CMergeEditView::OnPrevdiffMO()
{
	OnPrev3wayDiff(THREEWAYDIFFTYPE_MIDDLEONLY);
}

void CMergeEditView::OnUpdatePrevdiffMO(CCmdUI* pCmdUI)
{
	OnUpdatePrev3wayDiff(pCmdUI, THREEWAYDIFFTYPE_MIDDLEONLY);
}

void CMergeEditView::OnPrevdiffRO()
{
	OnPrev3wayDiff(THREEWAYDIFFTYPE_RIGHTONLY);
}

void CMergeEditView::OnUpdatePrevdiffRO(CCmdUI* pCmdUI)
{
	OnUpdatePrev3wayDiff(pCmdUI, THREEWAYDIFFTYPE_RIGHTONLY);
}

/**
 * @brief Clear selection
 */
void CMergeEditView::SelectNone()
{
	SetSelection (GetCursorPos(), GetCursorPos());
	UpdateCaret();
}

/**
 * @brief Check if line is inside currently selected diff
 * @param [in] nLine 0-based linenumber in view
 * @sa CMergeDoc::GetCurrentDiff()
 * @sa CMergeDoc::LineInDiff()
 */
bool CMergeEditView::IsLineInCurrentDiff(int nLine)
{
	// Check validity of nLine
#ifdef _DEBUG
	if (nLine < 0)
		_RPTF1(_CRT_ERROR, "Linenumber is negative (%d)!", nLine);
	int nLineCount = LocateTextBuffer()->GetLineCount();
	if (nLine >= nLineCount)
		_RPTF2(_CRT_ERROR, "Linenumber > linecount (%d>%d)!", nLine, nLineCount);
#endif

	CMergeDoc *pd = GetDocument();
	int curDiff = pd->GetCurrentDiff();
	if (curDiff == -1)
		return false;
	return pd->m_diffList.LineInDiff(nLine, curDiff);
}

/**
 * @brief Called when mouse left-button double-clicked
 *
 * Double-clicking mouse inside diff selects that diff
 */
void CMergeEditView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CMergeDoc *pd = GetDocument();
	CPoint pos = GetCursorPos();

	int diff = pd->m_diffList.LineToDiff(pos.y);
	if (diff != -1 && pd->m_diffList.IsDiffSignificant(diff))
		SelectDiff(diff, false, false);

	CCrystalEditViewEx::OnLButtonDblClk(nFlags, point);
}

/**
 * @brief Called when mouse left button is released.
 *
 * If button is released outside diffs, current diff
 * is deselected.
 */
void CMergeEditView::OnLButtonUp(UINT nFlags, CPoint point)
{
	CMergeDoc *pd = GetDocument();
	CCrystalEditViewEx::OnLButtonUp(nFlags, point);

	// If we have a selected diff, deselect it
	int nCurrentDiff = pd->GetCurrentDiff();
	if (nCurrentDiff != -1)
	{
		CPoint pos = GetCursorPos();
		if (!IsLineInCurrentDiff(pos.y))
		{
			pd->SetCurrentDiff(-1);
			Invalidate();
			pd->UpdateAllViews(this);
		}
	}
}

/**
 * @brief Finds longest line (needed for scrolling etc).
 * @sa CCrystalTextView::GetMaxLineLength()
 */
void CMergeEditView::UpdateLineLengths()
{
	GetMaxLineLength();
}

/**
 * @brief Copy diff from left pane to right pane
 *
 * Difference is copied from left to right when
 * - difference is selected
 * - difference is inside selection (allows merging multiple differences).
 * - cursor is inside diff
 *
 * If there is selected diff outside selection, we copy selected
 * difference only.
 */
void CMergeEditView::OnL2r()
{
	int dstPane = (m_nThisPane < GetDocument()->m_nBuffers - 1) ? m_nThisPane + 1 : GetDocument()->m_nBuffers - 1;
	int srcPane = dstPane - 1;

	// Check that right side is not readonly
	if (IsReadOnly(dstPane))
		return;

	CMergeDoc *pDoc = GetDocument();
	int currentDiff = pDoc->GetCurrentDiff();

	if (currentDiff == -1)
	{
		// No selected diff
		// If cursor is inside diff get number of that diff
		if (m_bCurrentLineIsDiff)
		{
			CPoint pt = GetCursorPos();
			currentDiff = pDoc->m_diffList.LineToDiff(pt.y);
		}
	}

	int firstDiff, lastDiff;
	GetFullySelectedDiffs(firstDiff, lastDiff);

	if (firstDiff != -1 && lastDiff != -1 && (lastDiff >= firstDiff))
	{
		WaitStatusCursor waitstatus(_("Copying Left to Right"));
		if (currentDiff != -1 && pDoc->m_diffList.IsDiffSignificant(currentDiff) && !IsSelection())
			pDoc->ListCopy(srcPane, dstPane, currentDiff);
		else
			pDoc->CopyMultipleList(srcPane, dstPane, firstDiff, lastDiff);
	}
	else if (currentDiff != -1 && pDoc->m_diffList.IsDiffSignificant(currentDiff))
	{
		WaitStatusCursor waitstatus(_("Copying Left to Right"));
		pDoc->ListCopy(srcPane, dstPane, currentDiff);
	}
}

/**
 * @brief Called when "Copy to left" item is updated
 */
void CMergeEditView::OnUpdateL2r(CCmdUI* pCmdUI)
{
	// Check that right side is not readonly
	if (!IsReadOnly(m_nThisPane < GetDocument()->m_nBuffers - 1 ? m_nThisPane + 1 : GetDocument()->m_nBuffers - 1))
	{
		int firstDiff, lastDiff;
		GetFullySelectedDiffs(firstDiff, lastDiff);

		// If one or more diffs inside selection OR
		// there is an active diff OR
		// cursor is inside diff
		if (firstDiff != -1 && lastDiff != -1 && (lastDiff >= firstDiff))
			pCmdUI->Enable(true);
		else
		{
			const int currDiff = GetDocument()->GetCurrentDiff();
			if (currDiff != -1 && GetDocument()->m_diffList.IsDiffSignificant(currDiff))
				pCmdUI->Enable(true);
			else
				pCmdUI->Enable(m_bCurrentLineIsDiff);
		}
	}
	else
		pCmdUI->Enable(false);
}

/**
 * @brief Copy diff from right pane to left pane
 *
 * Difference is copied from left to right when
 * - difference is selected
 * - difference is inside selection (allows merging multiple differences).
 * - cursor is inside diff
 *
 * If there is selected diff outside selection, we copy selected
 * difference only.
 */
void CMergeEditView::OnR2l()
{
	int dstPane = (m_nThisPane > 0) ? m_nThisPane - 1 : 0;
	int srcPane = dstPane + 1;

	// Check that left side is not readonly
	if (IsReadOnly(dstPane))
		return;

	CMergeDoc *pDoc = GetDocument();
	int currentDiff = pDoc->GetCurrentDiff();
	if (currentDiff == -1)
	{
		// No selected diff
		// If cursor is inside diff get number of that diff
		if (m_bCurrentLineIsDiff)
		{
			CPoint pt;
			pt = GetCursorPos();
			currentDiff = pDoc->m_diffList.LineToDiff(pt.y);
		}
	}

	int firstDiff, lastDiff;
	GetFullySelectedDiffs(firstDiff, lastDiff);

	if (firstDiff != -1 && lastDiff != -1 && (lastDiff >= firstDiff))
	{
		WaitStatusCursor waitstatus(_("Copying Right to Left"));
		if (currentDiff != -1 && pDoc->m_diffList.IsDiffSignificant(currentDiff) && !IsSelection())
			pDoc->ListCopy(srcPane, dstPane, currentDiff);
		else
			pDoc->CopyMultipleList(srcPane, dstPane, firstDiff, lastDiff);
	}
	else if (currentDiff != -1 && pDoc->m_diffList.IsDiffSignificant(currentDiff))
	{
		WaitStatusCursor waitstatus(_("Copying Right to Left"));
		pDoc->ListCopy(srcPane, dstPane, currentDiff);
	}
}

/**
 * @brief Called when "Copy to right" item is updated
 */
void CMergeEditView::OnUpdateR2l(CCmdUI* pCmdUI)
{
	// Check that left side is not readonly
	if (!IsReadOnly(m_nThisPane > 0 ? m_nThisPane - 1 : 0))
	{
		int firstDiff, lastDiff;
		GetFullySelectedDiffs(firstDiff, lastDiff);

		// If one or more diffs inside selection OR
		// there is an active diff OR
		// cursor is inside diff
		if (firstDiff != -1 && lastDiff != -1 && (lastDiff >= firstDiff))
			pCmdUI->Enable(true);
		else
		{
			const int currDiff = GetDocument()->GetCurrentDiff();
			if (currDiff != -1 && GetDocument()->m_diffList.IsDiffSignificant(currDiff))
				pCmdUI->Enable(true);
			else
				pCmdUI->Enable(m_bCurrentLineIsDiff);
		}
	}
	else
		pCmdUI->Enable(false);
}

/**
 * @brief Copy diff from left pane to middle pane
 *
 * Difference is copied from left to middle when
 * - difference is selected
 * - difference is inside selection (allows merging multiple differences).
 * - cursor is inside diff
 *
 * If there is selected diff outside selection, we copy selected
 * difference only.
 */
void CMergeEditView::OnL2m()
{
	int dstPane = 1;
	int srcPane = 0;

	CMergeDoc *pDoc = GetDocument();

	// Check that middle side is not readonly
	if (pDoc->m_nBuffers < 3 || IsReadOnly(dstPane))
		return;

	int currentDiff = pDoc->GetCurrentDiff();

	if (currentDiff == -1)
	{
		// No selected diff
		// If cursor is inside diff get number of that diff
		if (m_bCurrentLineIsDiff)
		{
			CPoint pt = GetCursorPos();
			currentDiff = pDoc->m_diffList.LineToDiff(pt.y);
		}
	}

	int firstDiff, lastDiff;
	GetFullySelectedDiffs(firstDiff, lastDiff);

	if (firstDiff != -1 && lastDiff != -1 && (lastDiff >= firstDiff))
	{
		WaitStatusCursor waitstatus(_("Copying Left to Middle"));
		if (currentDiff != -1 && pDoc->m_diffList.IsDiffSignificant(currentDiff) && !IsSelection())
			pDoc->ListCopy(srcPane, dstPane, currentDiff);
		else
			pDoc->CopyMultipleList(srcPane, dstPane, firstDiff, lastDiff);
	}
	else if (currentDiff != -1 && pDoc->m_diffList.IsDiffSignificant(currentDiff))
	{
		WaitStatusCursor waitstatus(_("Copying Left to Middle"));
		pDoc->ListCopy(srcPane, dstPane, currentDiff);
	}
}

/**
 * @brief Called when "Copy to middle" item is updated
 */
void CMergeEditView::OnUpdateL2m(CCmdUI* pCmdUI)
{
	CMergeDoc *pDoc = GetDocument();

	// Check that middle side is not readonly
	if (pDoc->m_nBuffers == 3 && !IsReadOnly(1))
	{
		int firstDiff, lastDiff;
		GetFullySelectedDiffs(firstDiff, lastDiff);

		// If one or more diffs inside selection OR
		// there is an active diff OR
		// cursor is inside diff
		if (firstDiff != -1 && lastDiff != -1 && (lastDiff >= firstDiff))
			pCmdUI->Enable(true);
		else
		{
			const int currDiff = GetDocument()->GetCurrentDiff();
			if (currDiff != -1 && GetDocument()->m_diffList.IsDiffSignificant(currDiff))
				pCmdUI->Enable(true);
			else
				pCmdUI->Enable(m_bCurrentLineIsDiff);
		}
	}
	else
		pCmdUI->Enable(false);
}

/**
 * @brief Copy diff from right pane to middle pane
 *
 * Difference is copied from right to middle when
 * - difference is selected
 * - difference is inside selection (allows merging multiple differences).
 * - cursor is inside diff
 *
 * If there is selected diff outside selection, we copy selected
 * difference only.
 */
void CMergeEditView::OnR2m()
{
	int dstPane = 1;
	int srcPane = 2;

	CMergeDoc *pDoc = GetDocument();

	// Check that left side is not readonly
	if (pDoc->m_nBuffers < 3 || IsReadOnly(dstPane))
		return;

	int currentDiff = pDoc->GetCurrentDiff();
	if (currentDiff == -1)
	{
		// No selected diff
		// If cursor is inside diff get number of that diff
		if (m_bCurrentLineIsDiff)
		{
			CPoint pt;
			pt = GetCursorPos();
			currentDiff = pDoc->m_diffList.LineToDiff(pt.y);
		}
	}

	int firstDiff, lastDiff;
	GetFullySelectedDiffs(firstDiff, lastDiff);

	if (firstDiff != -1 && lastDiff != -1 && (lastDiff >= firstDiff))
	{
		WaitStatusCursor waitstatus(_("Copying Right to Middle"));
		if (currentDiff != -1 && pDoc->m_diffList.IsDiffSignificant(currentDiff) && !IsSelection())
			pDoc->ListCopy(srcPane, dstPane, currentDiff);
		else
			pDoc->CopyMultipleList(srcPane, dstPane, firstDiff, lastDiff);
	}
	else if (currentDiff != -1 && pDoc->m_diffList.IsDiffSignificant(currentDiff))
	{
		WaitStatusCursor waitstatus(_("Copying Right to Middle"));
		pDoc->ListCopy(srcPane, dstPane, currentDiff);
	}
}

/**
 * @brief Called when "Copy to middle" item is updated
 */
void CMergeEditView::OnUpdateR2m(CCmdUI* pCmdUI)
{
	CMergeDoc *pDoc = GetDocument();

	// Check that middle side is not readonly
	if (pDoc->m_nBuffers == 3 && !IsReadOnly(1))
	{
		int firstDiff, lastDiff;
		GetFullySelectedDiffs(firstDiff, lastDiff);

		// If one or more diffs inside selection OR
		// there is an active diff OR
		// cursor is inside diff
		if (firstDiff != -1 && lastDiff != -1 && (lastDiff >= firstDiff))
			pCmdUI->Enable(true);
		else
		{
			const int currDiff = GetDocument()->GetCurrentDiff();
			if (currDiff != -1 && GetDocument()->m_diffList.IsDiffSignificant(currDiff))
				pCmdUI->Enable(true);
			else
				pCmdUI->Enable(m_bCurrentLineIsDiff);
		}
	}
	else
		pCmdUI->Enable(false);
}

/**
 * @brief Copy all diffs from right pane to left pane
 */
void CMergeEditView::OnAllLeft()
{
	// Check that left side is not readonly
	int srcPane = m_nThisPane > 0 ? m_nThisPane : 1;
	int dstPane = m_nThisPane > 0 ? m_nThisPane - 1 : 0;
	if (IsReadOnly(dstPane))
		return;
	WaitStatusCursor waitstatus(_("Copying All to Left"));

	GetDocument()->CopyAllList(srcPane, dstPane);
}

/**
 * @brief Called when "Copy all to left" item is updated
 */
void CMergeEditView::OnUpdateAllLeft(CCmdUI* pCmdUI)
{
	// Check that left side is not readonly
	int dstPane = m_nThisPane > 0 ? m_nThisPane - 1 : 0;
	if (!IsReadOnly(dstPane))
		pCmdUI->Enable(GetDocument()->m_diffList.HasSignificantDiffs());
	else
		pCmdUI->Enable(false);
}

/**
 * @brief Copy all diffs from left pane to right pane
 */
void CMergeEditView::OnAllRight()
{
	// Check that right side is not readonly
	int srcPane = m_nThisPane < GetDocument()->m_nBuffers - 1 ? m_nThisPane : m_nThisPane - 1;
	int dstPane = m_nThisPane < GetDocument()->m_nBuffers - 1 ? m_nThisPane + 1 : GetDocument()->m_nBuffers - 1;
	if (IsReadOnly(dstPane))
		return;

	WaitStatusCursor waitstatus(_("Copying All to Right"));

	GetDocument()->CopyAllList(srcPane, dstPane);
}

/**
 * @brief Called when "Copy all to right" item is updated
 */
void CMergeEditView::OnUpdateAllRight(CCmdUI* pCmdUI)
{
	// Check that right side is not readonly
	int dstPane = m_nThisPane < GetDocument()->m_nBuffers - 1 ? m_nThisPane + 1 : GetDocument()->m_nBuffers - 1;
	if (!IsReadOnly(dstPane))
		pCmdUI->Enable(GetDocument()->m_diffList.HasSignificantDiffs());
	else
		pCmdUI->Enable(false);
}

/**
 * @brief This function is called before other edit events.
 * @param [in] nAction Edit operation to do
 * @param [in] pszText Text to insert, delete etc
 * @sa CCrystalEditView::OnEditOperation()
 * @todo More edit-events for rescan delaying?
 */
void CMergeEditView::OnEditOperation(int nAction, LPCTSTR pszText, int cchText)
{
	if (IsReadOnly(m_nThisPane))
	{
		// We must not arrive here, and assert helps detect troubles
		ASSERT(0);
		return;
	}

	CMergeDoc* pDoc = GetDocument();
	pDoc->SetEditedAfterRescan(m_nThisPane);

	// simple hook for multiplex undo operations
	// deleted by jtuc 2003-06-28
	// now AddUndoRecords does it (so we don't create entry for OnEditOperation with no Undo data in m_pTextBuffer)
	/*if(dynamic_cast<CMergeDoc::CDiffTextBuffer*>(m_pTextBuffer)->curUndoGroup())
	{
		pDoc->undoTgt.erase(pDoc->curUndo, pDoc->undoTgt.end());
		pDoc->undoTgt.push_back(this);
		pDoc->curUndo = pDoc->undoTgt.end();
	}*/

	// perform original function
	CCrystalEditViewEx::OnEditOperation(nAction, pszText, cchText);

	// augment with additional operations

	// Change header to inform about changed doc
	pDoc->UpdateHeaderPath(m_nThisPane);

	// If automatic rescan enabled, rescan after edit events
	if (m_bAutomaticRescan)
	{
		// keep document up to date
		// (Re)start timer to rescan only when user edits text
		// If timer starting fails, rescan immediately
		if (nAction == CE_ACTION_TYPING ||
			nAction == CE_ACTION_REPLACE ||
			nAction == CE_ACTION_BACKSPACE ||
			nAction == CE_ACTION_INDENT ||
			nAction == CE_ACTION_PASTE ||
			nAction == CE_ACTION_DELSEL ||
			nAction == CE_ACTION_DELETE ||
			nAction == CE_ACTION_CUT)
		{
			if (!SetTimer(IDT_RESCAN, RESCAN_TIMEOUT, NULL))
				pDoc->FlushAndRescan();
		}
		else
			pDoc->FlushAndRescan();
	}
	else
	{
		if (m_bWordWrap)
		{
			// Update other pane for sync line.
			for (int nPane = 0; nPane < pDoc->m_nBuffers; nPane++)
			{
				if (nPane == m_nThisPane)
					continue;
				CCrystalEditView *pView = GetGroupView(nPane);
				if (pView)
					pView->Invalidate();
			}
		}
	}
}

/**
 * @brief Redo last action
 */
void CMergeEditView::OnEditRedo()
{
	WaitStatusCursor waitstatus(_("Redoing the previous operation..."));
	CMergeDoc* pDoc = GetDocument();
	CMergeEditView *tgt = *(pDoc->curUndo);
	if(tgt==this)
	{
		if (IsReadOnly(m_nThisPane))
			return;

		GetParentFrame()->SetActiveView(this, true);
		if(CCrystalEditViewEx::DoEditRedo())
		{
			++pDoc->curUndo;
			pDoc->UpdateHeaderPath(m_nThisPane);
			pDoc->FlushAndRescan();
		}
	}
	else
	{
		tgt->SendMessage(WM_COMMAND, ID_EDIT_REDO);
	}
}

/**
 * @brief Called when "Redo" item is updated
 */
void CMergeEditView::OnUpdateEditRedo(CCmdUI* pCmdUI)
{
	CMergeDoc* pDoc = GetDocument();
	if (pDoc->curUndo!=pDoc->undoTgt.end())
	{
		CMergeEditView *tgt = *(pDoc->curUndo);
		pCmdUI->Enable( !IsReadOnly(tgt->m_nThisPane));
	}
	else
		pCmdUI->Enable(false);
}

void CMergeEditView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CCrystalEditViewEx::OnUpdate(pSender, lHint, pHint);
}

/**
 * @brief Scrolls to current diff and/or selects diff text
 * @param [in] bScroll If true scroll diff to view
 * @param [in] bSelectText If true select diff text
 * @note If bScroll and bSelectText are false, this does nothing!
 * @todo This shouldn't be called when no diff is selected, so
 * somebody could try to ASSERT(nDiff > -1)...
 */
void CMergeEditView::ShowDiff(bool bScroll, bool bSelectText)
{
	CMergeDoc *pd = GetDocument();
	const int nDiff = pd->GetCurrentDiff();

	// Try to trap some errors
	if (nDiff >= pd->m_diffList.GetSize())
		_RPTF2(_CRT_ERROR, "Selected diff > diffcount (%d > %d)!",
			nDiff, pd->m_diffList.GetSize());

	if (nDiff >= 0 && nDiff < pd->m_diffList.GetSize())
	{
		CPoint ptStart, ptEnd;
		DIFFRANGE curDiff;
		pd->m_diffList.GetDiff(nDiff, curDiff);

		ptStart.x = 0;
		ptStart.y = curDiff.dbegin[0];
		ptEnd.x = 0;
		ptEnd.y = curDiff.dend[0];

		if (bScroll)
		{
			if (!IsDiffVisible(curDiff, CONTEXT_LINES_BELOW))
			{
				// Difference is not visible, scroll it so that max amount of
				// scrolling is done while keeping the diff in screen. So if
				// scrolling is downwards, scroll the diff to as up in screen
				// as possible. This usually brings next diff to the screen
				// and we don't need to scroll into it.
				int nLine = GetSubLineIndex(ptStart.y);
				if (nLine > CONTEXT_LINES_ABOVE)
				{
					nLine -= CONTEXT_LINES_ABOVE;
				}
				GetGroupView(m_nThisPane)->ScrollToSubLine(nLine);
				for (int nPane = 0; nPane < pd->m_nBuffers; nPane++)
				{
					if (nPane != m_nThisPane)
						GetGroupView(nPane)->ScrollToSubLine(nLine);
				}
			}
			GetGroupView(m_nThisPane)->SetCursorPos(ptStart);
			GetGroupView(m_nThisPane)->SetAnchor(ptStart);
			GetGroupView(m_nThisPane)->SetSelection(ptStart, ptStart);
			for (int nPane = 0; nPane < pd->m_nBuffers; nPane++)
			{
				if (nPane != m_nThisPane)
				{
					GetGroupView(nPane)->SetCursorPos(ptStart);
					GetGroupView(nPane)->SetAnchor(ptStart);
					GetGroupView(nPane)->SetSelection(ptStart, ptStart);
				}
			}
		}

		if (bSelectText)
		{
			ptEnd.x = GetLineLength(ptEnd.y);
			SetSelection(ptStart, ptEnd);
			UpdateCaret();
		}
		else
			Invalidate();
	}
}


void CMergeEditView::OnTimer(UINT_PTR nIDEvent)
{
	// Maybe we want theApp::OnIdle to proceed before processing a timer message
	// ...but for this the queue must be empty
	// The timer message is a low priority message but the queue is maybe not yet empty
	// So we set a flag, wait for OnIdle to proceed, then come back here...
	// We come back here with a IDLE_TIMER OnTimer message (send with SendMessage
	// not with SetTimer so there is no delay)

	// IDT_RESCAN was posted because the app wanted to do a flushAndRescan with some delay

	// IDLE_TIMER is the false timer used to come back here after OnIdle
	// fTimerWaitingForIdle is a bool to store the commands waiting for idle
	// (one normal timer = one flag = one command)

	if (nIDEvent == IDT_RESCAN)
	{
		KillTimer(IDT_RESCAN);
		fTimerWaitingForIdle |= FLAG_RESCAN_WAITS_FOR_IDLE;
		// notify the app to come back after OnIdle
		theApp.SetNeedIdleTimer();
	}

	if (nIDEvent == IDLE_TIMER)
	{
		// not a real timer, just come back after OnIdle
		// look to flags to know what to do
		if (fTimerWaitingForIdle & FLAG_RESCAN_WAITS_FOR_IDLE)
			GetDocument()->RescanIfNeeded(RESCAN_TIMEOUT/1000);
		fTimerWaitingForIdle = 0;
	}

	CCrystalEditViewEx::OnTimer(nIDEvent);
}

/**
 * @brief Returns if buffer is read-only
 * @note This has no any relation to file being read-only!
 */
bool CMergeEditView::IsReadOnly(int pane)
{
	return GetDocument()->m_ptBuf[pane]->GetReadOnly() != false;
}

/**
 * @brief Called when "Save left (as...)" item is updated
 */
void CMergeEditView::OnUpdateFileSaveLeft(CCmdUI* pCmdUI)
{
	CMergeDoc *pd = GetDocument();

	if (!IsReadOnly(0) && pd->m_ptBuf[0]->IsModified())
		pCmdUI->Enable(true);
	else
		pCmdUI->Enable(false);
}

/**
 * @brief Called when "Save middle (as...)" item is updated
 */
void CMergeEditView::OnUpdateFileSaveMiddle(CCmdUI* pCmdUI)
{
	CMergeDoc *pd = GetDocument();

	if (pd->m_nBuffers == 3 && !IsReadOnly(1) && pd->m_ptBuf[1]->IsModified())
		pCmdUI->Enable(true);
	else
		pCmdUI->Enable(false);
}

/**
 * @brief Called when "Save right (as...)" item is updated
 */
void CMergeEditView::OnUpdateFileSaveRight(CCmdUI* pCmdUI)
{
	CMergeDoc *pd = GetDocument();

	if (!IsReadOnly(pd->m_nBuffers - 1) && pd->m_ptBuf[pd->m_nBuffers - 1]->IsModified())
		pCmdUI->Enable(true);
	else
		pCmdUI->Enable(false);
}

/**
 * @brief Refresh display using text-buffers
 * @note This DOES NOT reload files!
 */
void CMergeEditView::OnRefresh()
{
	CMergeDoc *pd = GetDocument();
	ASSERT(pd);
	pd->FlushAndRescan(true);
}

/**
 * @brief Enable/Disable automatic rescanning
 */
bool CMergeEditView::EnableRescan(bool bEnable)
{
	bool bOldValue = m_bAutomaticRescan;
	m_bAutomaticRescan = bEnable;
	return bOldValue;
}

/**
 * @brief Handle some keys when in merging mode
 */
bool CMergeEditView::MergeModeKeyDown(MSG* pMsg)
{
	bool bHandled = false;

	// Allow default text selection when SHIFT pressed
	if (::GetAsyncKeyState(VK_SHIFT))
		return false;

	// Allow default editor functions when CTRL pressed
	if (::GetAsyncKeyState(VK_CONTROL))
		return false;

	// If we are in merging mode (merge with cursor keys)
	// handle some keys here
	switch (pMsg->wParam)
	{
	case VK_LEFT:
		OnR2l();
		bHandled = true;
		break;

	case VK_RIGHT:
		OnL2r();
		bHandled = true;
		break;

	case VK_UP:
		OnPrevdiff();
		bHandled = true;
		break;

	case VK_DOWN:
		OnNextdiff();
		bHandled = true;
		break;
	}

	return bHandled;
}

/**
 * @brief Called before messages are translated.
 *
 * Checks if ESC key was pressed, saves and closes doc.
 * Also if in merge mode traps cursor keys.
 */
BOOL CMergeEditView::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		bool bHandled = false;
		
		// If we are in merging mode (merge with cursor keys)
		// handle some keys here
		if (GetDocument()->GetMergingMode())
		{
			bHandled = MergeModeKeyDown(pMsg);
			if (bHandled)
				return false;
		}

		// Close window if user has allowed it from options
		if (pMsg->wParam == VK_ESCAPE)
		{
			bool bCloseWithEsc = GetOptionsMgr()->GetBool(OPT_CLOSE_WITH_ESC);
			if (bCloseWithEsc)
				GetParentFrame()->PostMessage(WM_CLOSE, 0, 0);
			return false;
		}
	}
	else if (pMsg->message == WM_SYSKEYDOWN)
	{
		if (::GetAsyncKeyState(VK_MENU))
		{
			UINT id = 0;
			if (::GetAsyncKeyState(VK_SHIFT))
			{
				switch (pMsg->wParam)
				{
				case '1': id = ID_PREVDIFFLM; break;
				case '2': id = ID_PREVDIFFLR; break;
				case '3': id = ID_PREVDIFFMR; break;
				case '7': id = ID_PREVDIFFLO; break;
				case '8': id = ID_PREVDIFFMO; break;
				case '9': id = ID_PREVDIFFRO; break;
				}
			}
			else
			{
				switch (pMsg->wParam)
				{
				case '1': id = ID_NEXTDIFFLM; break;
				case '2': id = ID_NEXTDIFFLR; break;
				case '3': id = ID_NEXTDIFFMR; break;
				case '4': id = ID_L2M; break;
				case '6': id = ID_R2M; break;
				case '7': id = ID_NEXTDIFFLO; break;
				case '8': id = ID_NEXTDIFFMO; break;
				case '9': id = ID_NEXTDIFFRO; break;
				}
			}
			if (id)
			{
				PostMessage(WM_COMMAND, id);
				return false;
			}
		}
	}

	return CCrystalEditViewEx::PreTranslateMessage(pMsg);
}

/**
 * @brief Called when "Save" item is updated
 */
void CMergeEditView::OnUpdateFileSave(CCmdUI* pCmdUI)
{
	CMergeDoc *pd = GetDocument();

	bool bModified = false;
	for (int nPane = 0; nPane < pd->m_nBuffers; nPane++)
	{
		if (pd->m_ptBuf[nPane]->IsModified())
			bModified = true;
	}
	if (bModified)
		pCmdUI->Enable(true);
	else
		pCmdUI->Enable(false);
}

/**
 * @brief Enable/disable left buffer read-only
 */
void CMergeEditView::OnLeftReadOnly()
{
	CMergeDoc *pd = GetDocument();
	bool bReadOnly = pd->m_ptBuf[0]->GetReadOnly();
	pd->m_ptBuf[0]->SetReadOnly(!bReadOnly);
}

/**
 * @brief Called when "Left read-only" item is updated
 */
void CMergeEditView::OnUpdateLeftReadOnly(CCmdUI* pCmdUI)
{
	CMergeDoc *pd = GetDocument();
	bool bReadOnly = pd->m_ptBuf[0]->GetReadOnly();
	pCmdUI->Enable(true);
	pCmdUI->SetCheck(bReadOnly);
}

/**
 * @brief Enable/disable middle buffer read-only
 */
void CMergeEditView::OnMiddleReadOnly()
{
	CMergeDoc *pd = GetDocument();
	if (pd->m_nBuffers == 3)
	{
		bool bReadOnly = pd->m_ptBuf[1]->GetReadOnly();
		pd->m_ptBuf[1]->SetReadOnly(!bReadOnly);
	}
}

/**
 * @brief Called when "Middle read-only" item is updated
 */
void CMergeEditView::OnUpdateMiddleReadOnly(CCmdUI* pCmdUI)
{
	CMergeDoc *pd = GetDocument();
	if (pd->m_nBuffers < 3)
	{
		pCmdUI->Enable(false);
	}
	else
	{
		bool bReadOnly = pd->m_ptBuf[1]->GetReadOnly();
		pCmdUI->Enable(true);
		pCmdUI->SetCheck(bReadOnly);
	}
}

/**
 * @brief Enable/disable right buffer read-only
 */
void CMergeEditView::OnRightReadOnly()
{
	CMergeDoc *pd = GetDocument();
	bool bReadOnly = pd->m_ptBuf[pd->m_nBuffers - 1]->GetReadOnly();
	pd->m_ptBuf[pd->m_nBuffers - 1]->SetReadOnly(!bReadOnly);
}

/**
 * @brief Called when "Left read-only" item is updated
 */
void CMergeEditView::OnUpdateRightReadOnly(CCmdUI* pCmdUI)
{
	CMergeDoc *pd = GetDocument();
	bool bReadOnly = pd->m_ptBuf[pd->m_nBuffers - 1]->GetReadOnly();
	pCmdUI->Enable(true);
	pCmdUI->SetCheck(bReadOnly);
}

/// Store interface we use to display status line info
void CMergeEditView::SetStatusInterface(IMergeEditStatus * piMergeEditStatus)
{
	ASSERT(!m_piMergeEditStatus);
	m_piMergeEditStatus = piMergeEditStatus;
}

/**
 * @brief Update status bar contents.
 */
void CMergeEditView::UpdateStatusbar()
{
	OnUpdateCaret();
}

/**
 * @brief Update statusbar info, Override from CCrystalTextView
 * @note we tab-expand column, but we don't tab-expand char count,
 * since we want to show how many chars there are and tab is just one
 * character although it expands to several spaces.
 */
void CMergeEditView::OnUpdateCaret()
{
	if (!m_piMergeEditStatus || !IsTextBufferInitialized())
		return;

	CPoint cursorPos = GetCursorPos();
	int nScreenLine = cursorPos.y;
	const int nRealLine = ComputeRealLine(nScreenLine);
	CString sLine;
	int chars = -1;
	CString sEol;
	int column = -1;
	int columns = -1;
	int curChar = -1;
	DWORD dwLineFlags = 0;

	dwLineFlags = m_pTextBuffer->GetLineFlags(nScreenLine);
	// Is this a ghost line ?
	if (dwLineFlags & LF_GHOST)
	{
		// Ghost lines display eg "Line 12-13"
		sLine.Format(_T("%d-%d"), nRealLine, nRealLine+1);
		sEol = _T("hidden");
	}
	else
	{
		// Regular lines display eg "Line 13 Characters: 25 EOL: CRLF"
		sLine.Format(_T("%d"), nRealLine+1);
		curChar = cursorPos.x + 1;
		chars = GetLineLength(nScreenLine);
		column = CalculateActualOffset(nScreenLine, cursorPos.x, true) + 1;
		columns = CalculateActualOffset(nScreenLine, chars, true) + 1;
		chars++;
		if (GetOptionsMgr()->GetBool(OPT_ALLOW_MIXED_EOL) ||
				GetDocument()->IsMixedEOL(m_nThisPane))
		{
			sEol = GetTextBufferEol(nScreenLine);
		}
		else
			sEol = _T("hidden");
	}
	m_piMergeEditStatus->SetLineInfo(sLine, column, columns,
		curChar, chars, sEol, GetDocument()->m_ptBuf[m_nThisPane]->getCodepage(), GetDocument()->m_ptBuf[m_nThisPane]->getHasBom());

	// Is cursor inside difference?
	if (dwLineFlags & LF_NONTRIVIAL_DIFF)
		m_bCurrentLineIsDiff = true;
	else
		m_bCurrentLineIsDiff = false;

	UpdateLocationViewPosition(m_nTopSubLine, m_nTopSubLine + GetScreenLines());
}
/**
 * @brief Select linedifference in the current line.
 *
 * Select line difference in current line. Selection type
 * is choosed by highlight type.
 */
void CMergeEditView::OnSelectLineDiff()
{
	// Pass this to the document, to compare this file to other
	GetDocument()->Showlinediff(this);
}

/// Enable select difference menuitem if current line is inside difference.
void CMergeEditView::OnUpdateSelectLineDiff(CCmdUI* pCmdUI)
{
	int line = GetCursorPos().y;
	bool enable = ((GetLineFlags(line) & (LF_DIFF | LF_GHOST)) != 0);
	if (GetDocument()->IsEditedAfterRescan(m_nThisPane))
		enable = false;
	pCmdUI->Enable(enable);
}

/**
 * @brief Enable/disable Replace-menuitem
 */
void CMergeEditView::OnUpdateEditReplace(CCmdUI* pCmdUI)
{
	CMergeDoc *pd = GetDocument();
	bool bReadOnly = pd->m_ptBuf[m_nThisPane]->GetReadOnly();

	pCmdUI->Enable(!bReadOnly);
}

/**
 * @brief Update readonly statusbaritem
 */
void CMergeEditView::OnUpdateStatusRO(CCmdUI* pCmdUI)
{
	bool bRO = GetDocument()->m_ptBuf[pCmdUI->m_nID - ID_STATUS_PANE0FILE_RO]->GetReadOnly();
	pCmdUI->Enable(bRO);
}

/**
 * @brief Call ::AppendMenu, and if it fails get error string into local variable
 *
 * This only provides functionality for debugging.
 */
static bool DoAppendMenu(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCTSTR lpNewItem)
{
	bool ok = ::AppendMenu(hMenu, uFlags, uIDNewItem, lpNewItem) != false;
	if (!ok)
	{
		int nerr = GetLastError();
		String syserr = GetSysError(nerr);
	}
	return ok;
}

/**
 * @brief Create the dynamic submenu for scripts
 */
HMENU CMergeEditView::createScriptsSubmenu(HMENU hMenu)
{
	// get scripts list
	std::vector<String> functionNamesList;
	GetFreeFunctionsInScripts(functionNamesList, L"EDITOR_SCRIPT");

	// empty the menu
	int i = GetMenuItemCount(hMenu);
	while (i --)
		DeleteMenu(hMenu, 0, MF_BYPOSITION);

	if (functionNamesList.size() == 0)
	{
		// no script : create a <empty> entry
		DoAppendMenu(hMenu, MF_STRING, ID_NO_EDIT_SCRIPTS, theApp.LoadString(ID_NO_EDIT_SCRIPTS).c_str());
	}
	else
	{
		// or fill in the submenu with the scripts names
		int ID = ID_SCRIPT_FIRST;	// first ID in menu
		for (i = 0 ; i < functionNamesList.size() ; i++, ID++)
			DoAppendMenu(hMenu, MF_STRING, ID, functionNamesList[i].c_str());

		functionNamesList.clear();
	}

	if (!IsWindowsScriptThere())
		DoAppendMenu(hMenu, MF_STRING, ID_NO_SCT_SCRIPTS, theApp.LoadString(ID_NO_SCT_SCRIPTS).c_str());

	return hMenu;
}

/**
 * @brief Create the dynamic submenu for prediffers
 *
 * @note The plugins are grouped in (suggested) and (not suggested)
 *       The IDs follow the order of GetAvailableScripts
 *       For example :
 *				suggested 0         ID_1ST + 0 
 *				suggested 1         ID_1ST + 2 
 *				suggested 2         ID_1ST + 5 
 *				not suggested 0     ID_1ST + 1 
 *				not suggested 1     ID_1ST + 3 
 *				not suggested 2     ID_1ST + 4 
 */
HMENU CMergeEditView::createPrediffersSubmenu(HMENU hMenu)
{
	// empty the menu
	int i = GetMenuItemCount(hMenu);
	while (i --)
		DeleteMenu(hMenu, 0, MF_BYPOSITION);

	CMergeDoc *pd = GetDocument();
	ASSERT(pd);

	// title
	DoAppendMenu(hMenu, MF_STRING, ID_NO_PREDIFFER, theApp.LoadString(ID_NO_PREDIFFER).c_str());

	// get the scriptlet files
	PluginArray * piScriptArray = 
		CAllThreadsScripts::GetActiveSet()->GetAvailableScripts(L"FILE_PREDIFF");
	PluginArray * piScriptArray2 = 
		CAllThreadsScripts::GetActiveSet()->GetAvailableScripts(L"BUFFER_PREDIFF");

	// build the menu : first part, suggested plugins
	// title
	DoAppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	DoAppendMenu(hMenu, MF_STRING, ID_SUGGESTED_PLUGINS, theApp.LoadString(ID_SUGGESTED_PLUGINS).c_str());

	int ID = ID_PREDIFFERS_FIRST;	// first ID in menu
	int iScript;
	for (iScript = 0 ; iScript < piScriptArray->size() ; iScript++, ID ++)
	{
		const PluginInfoPtr & plugin = piScriptArray->at(iScript);
		if (!plugin->TestAgainstRegList(pd->m_strBothFilenames))
			continue;

		DoAppendMenu(hMenu, MF_STRING, ID, plugin->m_name.c_str());
	}
	for (iScript = 0 ; iScript < piScriptArray2->size() ; iScript++, ID ++)
	{
		const PluginInfoPtr & plugin = piScriptArray2->at(iScript);
		if (!plugin->TestAgainstRegList(pd->m_strBothFilenames))
			continue;

		DoAppendMenu(hMenu, MF_STRING, ID, plugin->m_name.c_str());
	}

	// build the menu : second part, others plugins
	// title
	DoAppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	DoAppendMenu(hMenu, MF_STRING, ID_NOT_SUGGESTED_PLUGINS, theApp.LoadString(ID_NOT_SUGGESTED_PLUGINS).c_str());

	ID = ID_PREDIFFERS_FIRST;	// first ID in menu
	for (iScript = 0 ; iScript < piScriptArray->size() ; iScript++, ID ++)
	{
		const PluginInfoPtr & plugin = piScriptArray->at(iScript);
		if (plugin->TestAgainstRegList(pd->m_strBothFilenames) != false)
			continue;

		DoAppendMenu(hMenu, MF_STRING, ID, plugin->m_name.c_str());
	}
	for (iScript = 0 ; iScript < piScriptArray2->size() ; iScript++, ID ++)
	{
		const PluginInfoPtr & plugin = piScriptArray2->at(iScript);
		if (plugin->TestAgainstRegList(pd->m_strBothFilenames) != false)
			continue;

		DoAppendMenu(hMenu, MF_STRING, ID, plugin->m_name.c_str());
	}

	// compute the m_CurrentPredifferID (to set the radio button)
	PrediffingInfo prediffer;
	pd->GetPrediffer(&prediffer);

	if (prediffer.bToBeScanned)
		m_CurrentPredifferID = 0;
	else if (prediffer.pluginName.empty())
		m_CurrentPredifferID = ID_NO_PREDIFFER;
	else
	{
		ID = ID_PREDIFFERS_FIRST;	// first ID in menu
		for (iScript = 0 ; iScript < piScriptArray->size() ; iScript++, ID ++)
		{
			const PluginInfoPtr & plugin = piScriptArray->at(iScript);
			if (prediffer.pluginName == plugin->m_name)
				m_CurrentPredifferID = ID;

		}
		for (iScript = 0 ; iScript < piScriptArray2->size() ; iScript++, ID ++)
		{
			const PluginInfoPtr & plugin = piScriptArray2->at(iScript);
			if (prediffer.pluginName == plugin->m_name)
				m_CurrentPredifferID = ID;
		}
	}

	return hMenu;
}

/**
 * @brief Offer a context menu built with scriptlet/ActiveX functions
 */
void CMergeEditView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// Create the menu and populate it with the available functions
	BCMenu menu;
	VERIFY(menu.LoadMenu(IDR_POPUP_MERGEVIEW));

	// Remove copying item copying from active side
	if (m_nThisPane == 0) // left?
		menu.RemoveMenu(ID_R2L, MF_BYCOMMAND);
	else
		menu.RemoveMenu(ID_L2R, MF_BYCOMMAND);

	VERIFY(menu.LoadToolbar(IDR_MAINFRAME));
	theApp.TranslateMenu(menu.m_hMenu);

	BCMenu *pSub = (BCMenu *)menu.GetSubMenu(0);
	ASSERT(pSub != NULL);

	// Context menu opened using keyboard has no coordinates
	if (point.x == -1 && point.y == -1)
	{
		CRect rect;
		GetClientRect(rect);
		ClientToScreen(rect);

		point = rect.TopLeft();
		point.Offset(5, 5);
	}

	pSub->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		point.x, point.y, AfxGetMainWnd());

}

/**
 * @brief Update EOL mode in status bar
 */
void CMergeEditView::OnUpdateStatusEOL(CCmdUI* pCmdUI)
{
	GetGroupView(pCmdUI->m_nID - ID_STATUS_PANE0FILE_EOL)->OnUpdateIndicatorCRLF(pCmdUI);
}

/**
 * @brief Change EOL mode and unify all the lines EOL to this new mode
 */
void CMergeEditView::OnConvertEolTo(UINT nID )
{
	CRLFSTYLE nStyle = CRLF_STYLE_AUTOMATIC;;
	switch (nID)
	{
		case ID_EOL_TO_DOS:
			nStyle = CRLF_STYLE_DOS;
			break;
		case ID_EOL_TO_UNIX:
			nStyle = CRLF_STYLE_UNIX;
			break;
		case ID_EOL_TO_MAC:
			nStyle = CRLF_STYLE_MAC;
			break;
		default:
			// Catch errors
			_RPTF0(_CRT_ERROR, "Unhandled EOL type conversion!");
			break;
	}
	m_pTextBuffer->SetCRLFMode(nStyle);

	// we don't need a derived applyEOLMode for ghost lines as they have no EOL char
	if (m_pTextBuffer->applyEOLMode())
	{
		CMergeDoc *pd = GetDocument();
		ASSERT(pd);
		pd->UpdateHeaderPath(m_nThisPane);
		pd->FlushAndRescan(true);
	}
}

/**
 * @brief allow convert to entries in file submenu
 */
void CMergeEditView::OnUpdateConvertEolTo(CCmdUI* pCmdUI)
{
	int nStyle = CRLF_STYLE_AUTOMATIC;
	switch (pCmdUI->m_nID)
	{
		case ID_EOL_TO_DOS:
			nStyle = CRLF_STYLE_DOS;
			break;
		case ID_EOL_TO_UNIX:
			nStyle = CRLF_STYLE_UNIX;
			break;
		case ID_EOL_TO_MAC:
			nStyle = CRLF_STYLE_MAC;
			break;
		default:
			// Catch errors
			_RPTF0(_CRT_ERROR, "Missing menuitem handler for EOL convert menu!");
			break;
	}

	if (GetOptionsMgr()->GetBool(OPT_ALLOW_MIXED_EOL) ||
		GetDocument()->IsMixedEOL(m_nThisPane) ||
		nStyle != m_pTextBuffer->GetCRLFMode())
	{
		pCmdUI->SetRadio(false);

		// Don't allow selecting other EOL style for protected pane
		if (IsReadOnly(m_nThisPane))
			pCmdUI->Enable(false);
	}
	else
		pCmdUI->SetRadio(true);
}

/**
 * @brief Copy diff from left to right and advance to next diff
 */
void CMergeEditView::OnL2RNext()
{
	OnL2r();
	OnNextdiff();
}

/**
 * @brief Update "Copy right and advance" UI item
 */
void CMergeEditView::OnUpdateL2RNext(CCmdUI* pCmdUI)
{
	OnUpdateL2r(pCmdUI);
}

/**
 * @brief Copy diff from right to left and advance to next diff
 */
void CMergeEditView::OnR2LNext()
{
	OnR2l();
	OnNextdiff();
}

/**
 * @brief Update "Copy left and advance" UI item
 */
void CMergeEditView::OnUpdateR2LNext(CCmdUI* pCmdUI)
{
	OnUpdateR2l(pCmdUI);
}

/**
 * @brief Change active pane in MergeView.
 * Changes active pane and makes sure cursor position is kept in
 * screen. Currently we put cursor in same line than in original
 * active pane but we could be smarter too? Maybe update cursor
 * only when it is not visible in new pane?
 */
void CMergeEditView::OnChangePane()
{
	CSplitterWnd *pSplitterWnd = GetParentSplitter(this, false);
	CMergeEditView *pWnd = static_cast<CMergeEditView*>(pSplitterWnd->GetActivePane());
	CPoint ptCursor = pWnd->GetCursorPos();
	pSplitterWnd->ActivateNext();
	pWnd = static_cast<CMergeEditView*>(pSplitterWnd->GetActivePane());
	ptCursor.x = 0;
	if (ptCursor.y >= pWnd->GetLineCount())
		ptCursor.y = pWnd->GetLineCount() - 1;
	pWnd->SetCursorPos(ptCursor);
	pWnd->SetAnchor(ptCursor);
	pWnd->SetSelection(ptCursor, ptCursor);
}

/**
 * @brief Enable "Change Pane" menuitem when mergeview is active
 */
void CMergeEditView::OnUpdateChangePane(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(true);
}

/**
 * @brief Show "Go To" dialog and scroll views to line or diff.
 *
 * Before dialog is opened, current line and file is determined
 * and selected.
 * @note Conversions needed between apparent and real lines
 */
void CMergeEditView::OnWMGoto()
{
	WMGotoDlg dlg;
	CMergeDoc *pDoc = GetDocument();
	CPoint pos = GetCursorPos();
	int nRealLine = 0;
	int nLastLine = 0;

	nRealLine = pDoc->m_ptBuf[m_nThisPane]->ComputeRealLine(pos.y);
	int nLineCount = pDoc->m_ptBuf[m_nThisPane]->GetLineCount();
	nLastLine = pDoc->m_ptBuf[m_nThisPane]->ComputeRealLine(nLineCount - 1);

	// Set active file and current line selected in dialog
	dlg.m_strParam.Format(_T("%d"), nRealLine + 1);
	dlg.m_nFile = (pDoc->m_nBuffers < 3) ? (m_nThisPane == 1 ? 2 : 0) : m_nThisPane;
	dlg.m_nGotoWhat = 0;

	if (dlg.DoModal() == IDOK)
	{
		CMergeDoc * pDoc = GetDocument();
		CMergeEditView * pCurrentView = NULL;

		// Get views
		pCurrentView = GetGroupView(m_nThisPane);

		if (dlg.m_nGotoWhat == 0)
		{
			int nRealLine = _ttoi(dlg.m_strParam) - 1;
			if (nRealLine < 0)
				nRealLine = 0;
			if (nRealLine > nLastLine)
				nRealLine = nLastLine;

			GotoLine(nRealLine, true, (pDoc->m_nBuffers < 3) ? (dlg.m_nFile == 2 ? 1 : 0) : dlg.m_nFile);
		}
		else
		{
			int diff = _ttoi(dlg.m_strParam) - 1;
			if (diff < 0)
				diff = 0;
			if (diff >= pDoc->m_diffList.GetSize())
				diff = pDoc->m_diffList.GetSize();

			pCurrentView->SelectDiff(diff, true, false);
		}
	}
}

/**
 * @brief Enable "Go To" menuitem when mergeview is active
 */
void CMergeEditView::OnUpdateWMGoto(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(true);
}

/**
 * @brief Reload options.
 */
void CMergeEditView::RefreshOptions()
{ 
	m_bAutomaticRescan = GetOptionsMgr()->GetBool(OPT_AUTOMATIC_RESCAN);

	if (GetOptionsMgr()->GetInt(OPT_TAB_TYPE) == 0)
		SetInsertTabs(true);
	else
		SetInsertTabs(false);

	SetSelectionMargin(GetOptionsMgr()->GetBool(OPT_VIEW_FILEMARGIN));

	if (!GetOptionsMgr()->GetBool(OPT_SYNTAX_HIGHLIGHT))
		SetTextType(CCrystalTextView::SRC_PLAIN);

	SetWordWrapping(GetOptionsMgr()->GetBool(OPT_WORDWRAP));
	SetViewLineNumbers(GetOptionsMgr()->GetBool(OPT_VIEW_LINENUMBERS));

	Options::DiffColors::Load(m_cachedColors);
}

/**
 * @brief Called when an editor script item is updated
 */
void CMergeEditView::OnUpdateScripts(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(true);
}

void CMergeEditView::OnScripts(UINT nID )
{
	// text is CHAR if compiled without UNICODE, WCHAR with UNICODE
	String text = GetSelectedText();

	// transform the text with a script/ActiveX function, event=EDITOR_SCRIPT
	bool bChanged = TextTransform_Interactive(text, L"EDITOR_SCRIPT", nID - ID_SCRIPT_FIRST);
	if (bChanged)
		// now replace the text
		ReplaceSelection(text.c_str(), text.length(), 0);
}

/**
 * @brief Called when an editor script item is updated
 */
void CMergeEditView::OnUpdateNoEditScripts(CCmdUI* pCmdUI)
{
	// append the scripts submenu
	HMENU scriptsSubmenu = dynamic_cast<CMainFrame*>(AfxGetMainWnd())->GetScriptsSubmenu(AfxGetMainWnd()->GetMenu()->m_hMenu);
	if (scriptsSubmenu != NULL)
		createScriptsSubmenu(scriptsSubmenu);

	pCmdUI->Enable(true);
}

/**
 * @brief Called when an editor script item is updated
 */
void CMergeEditView::OnUpdatePrediffer(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(true);

	CMergeDoc *pd = GetDocument();
	ASSERT(pd);
	PrediffingInfo prediffer;
	pd->GetPrediffer(&prediffer);

	if (prediffer.bToBeScanned)
	{
		pCmdUI->SetRadio(false);
		return;
	}

	// Detect when CDiffWrapper::RunFileDiff has canceled a buggy prediffer
	if (prediffer.pluginName.empty())
		m_CurrentPredifferID = ID_NO_PREDIFFER;

	pCmdUI->SetRadio(pCmdUI->m_nID == m_CurrentPredifferID);
}

/**
 * @brief Update "Prediffer" menuitem
 */
void CMergeEditView::OnUpdateNoPrediffer(CCmdUI* pCmdUI)
{
	// recreate the sub menu (to fill the "selected prediffers")
	GetMainFrame()->UpdatePrediffersMenu();
	OnUpdatePrediffer(pCmdUI);
}

void CMergeEditView::OnNoPrediffer()
{
	OnPrediffer(ID_NO_PREDIFFER);
}
/**
 * @brief Handler for all prediffer choices, including ID_PREDIFF_MANUAL, ID_PREDIFF_AUTO, ID_NO_PREDIFFER, & specific prediffers
 */
void CMergeEditView::OnPrediffer(UINT nID )
{
	CMergeDoc *pd = GetDocument();
	ASSERT(pd);

	SetPredifferByMenu(nID);
	pd->FlushAndRescan(true);
}

/**
 * @brief Handler for all prediffer choices.
 * Prediffer choises include ID_PREDIFF_MANUAL, ID_PREDIFF_AUTO,
 * ID_NO_PREDIFFER, & specific prediffers.
 */
void CMergeEditView::SetPredifferByMenu(UINT nID )
{
	CMergeDoc *pd = GetDocument();
	ASSERT(pd);

	if (nID == ID_NO_PREDIFFER)
	{
		m_CurrentPredifferID = nID;
		// All flags are set correctly during the construction
		PrediffingInfo *infoPrediffer = new PrediffingInfo;
		infoPrediffer->bToBeScanned = 0;
		infoPrediffer->pluginName.clear();
		pd->SetPrediffer(infoPrediffer);
		pd->FlushAndRescan(true);
		return;
	}

	// get the scriptlet files
	PluginArray * piScriptArray = 
		CAllThreadsScripts::GetActiveSet()->GetAvailableScripts(L"FILE_PREDIFF");
	PluginArray * piScriptArray2 = 
		CAllThreadsScripts::GetActiveSet()->GetAvailableScripts(L"BUFFER_PREDIFF");

	// build a PrediffingInfo structure fom the ID
	PrediffingInfo prediffer;
	prediffer.bToBeScanned = false;

	int pluginNumber = nID - ID_PREDIFFERS_FIRST;
	if (pluginNumber < piScriptArray->size())
	{
		prediffer.bWithFile = true;
		const PluginInfoPtr & plugin = piScriptArray->at(pluginNumber);
		prediffer.pluginName = plugin->m_name;
	}
	else
	{
		pluginNumber -= piScriptArray->size();
		if (pluginNumber >= piScriptArray2->size())
			return;
		prediffer.bWithFile = false;
		const PluginInfoPtr & plugin = piScriptArray2->at(pluginNumber);
		prediffer.pluginName = plugin->m_name;
	}

	// update data for the radio button
	m_CurrentPredifferID = nID;

	// update the prediffer and rescan
	pd->SetPrediffer(&prediffer);
}

/**
 * @brief Look through available prediffers, and return ID of requested one, if found
 */
int CMergeEditView::FindPrediffer(LPCTSTR prediffer) const
{
	int i;
	int ID = ID_PREDIFFERS_FIRST;

	// Search file prediffers
	PluginArray * piScriptArray = 
		CAllThreadsScripts::GetActiveSet()->GetAvailableScripts(L"FILE_PREDIFF");
	for (i=0; i<piScriptArray->size(); ++i, ++ID)
	{
		const PluginInfoPtr & plugin = piScriptArray->at(i);
		if (plugin->m_name == prediffer)
			return ID;
	}

	// Search buffer prediffers
	PluginArray * piScriptArray2 = 
		CAllThreadsScripts::GetActiveSet()->GetAvailableScripts(L"BUFFER_PREDIFF");
	for (i=0; i<piScriptArray2->size(); ++i, ++ID)
	{
		const PluginInfoPtr & plugin = piScriptArray2->at(i);
		if (plugin->m_name == prediffer)
			return ID;
	}
	return -1;
}


/**
 * @brief Look through available prediffers, and return ID of requested one, if found
 */
bool CMergeEditView::SetPredifferByName(const CString & prediffer)
{
	int id = FindPrediffer(prediffer);
	if (id<0) return false;
	SetPredifferByMenu(id);
	return true;
}

/**
 * @brief Switch Merging/Editing mode and update
 * buffer read-only states accordingly
 */
void CMergeEditView::OnMergingMode()
{
	CMergeDoc *pDoc = GetDocument();
	bool bMergingMode = pDoc->GetMergingMode();

	if (!bMergingMode)
		LangMessageBox(IDS_MERGE_MODE, MB_ICONINFORMATION | MB_DONT_DISPLAY_AGAIN);
	pDoc->SetMergingMode(!bMergingMode);
}

/**
 * @brief Update Menuitem for Merging Mode
 */
void CMergeEditView::OnUpdateMergingMode(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(true);
	pCmdUI->SetCheck(GetDocument()->GetMergingMode());
}

/**
 * @brief Update MergingMode UI in statusbar
 */
void CMergeEditView::OnUpdateMergingStatus(CCmdUI *pCmdUI)
{
	String text = _("Merge");
	pCmdUI->SetText(text.c_str());
	pCmdUI->Enable(GetDocument()->GetMergingMode());
}

/** 
 * @brief Goto given line.
 * @param [in] nLine Destination linenumber
 * @param [in] bRealLine if true linenumber is real line, otherwise
 * it is apparent line (including deleted lines)
 * @param [in] pane Pane index of goto target pane (0 = left, 1 = right).
 */
void CMergeEditView::GotoLine(UINT nLine, bool bRealLine, int pane)
{
 	CMergeDoc *pDoc = GetDocument();
	CSplitterWnd *pSplitterWnd = GetParentSplitter(this, false);
	CMergeEditView *pCurrentView = NULL;
	if (pSplitterWnd)
		pCurrentView = static_cast<CMergeEditView*>
			(pSplitterWnd->GetActivePane());

	int nRealLine = nLine;
	int nApparentLine = nLine;

	// Compute apparent (shown linenumber) line
	if (bRealLine)
	{
		if (nRealLine > pDoc->m_ptBuf[pane]->GetLineCount() - 1)
			nRealLine = pDoc->m_ptBuf[pane]->GetLineCount() - 1;

		nApparentLine = pDoc->m_ptBuf[pane]->ComputeApparentLine(nRealLine);
	}
	CPoint ptPos;
	ptPos.x = 0;
	ptPos.y = nApparentLine;

	// Scroll line to center of view
	int nScrollLine = GetSubLineIndex(nApparentLine);
	nScrollLine -= GetScreenLines() / 2;
	if (nScrollLine < 0)
		nScrollLine = 0;

	for (int nPane = 0; nPane < pDoc->m_nBuffers; nPane++)
	{
		CMergeEditView *pView = GetGroupView(nPane);
		pView->ScrollToSubLine(nScrollLine);
		if (ptPos.y < pView->GetLineCount())
		{
			pView->SetCursorPos(ptPos);
			pView->SetAnchor(ptPos);
		}
		else
		{
			CPoint ptPos(0, pView->GetLineCount() - 1);
			pView->SetCursorPos(ptPos);
			pView->SetAnchor(ptPos);
		}
	}

	// If goto target is another view - activate another view.
	// This is done for user convenience as user probably wants to
	// work with goto target file.
	if (GetGroupView(pane) != pCurrentView)
	{
		if (pSplitterWnd)
		{
			if (pSplitterWnd->GetColumnCount() > 1)
				pSplitterWnd->SetActivePane(0, pane);
			else
				pSplitterWnd->SetActivePane(pane, 0);
		}
	}
}

/**
 * @brief Check for horizontal scroll. Re-route to CSplitterEx if not from
 * a scroll bar.
 */
void CMergeEditView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
{
	if (pScrollBar == NULL)
	{
		// Scroll did not come frome a scroll bar
		// Find the appropriate scroll bar
		// and send the message to the splitter window instead
		// The event should eventually come back here but with a valid scrollbar
		// Along the way it will be propagated to other windows that need it
		CSplitterWnd *pSplitterWnd = GetParentSplitter(this, false);
		CScrollBar* curBar = this->GetScrollBarCtrl(SB_HORZ);
		pSplitterWnd->SendMessage(WM_HSCROLL,
			MAKELONG(nSBCode, nPos), (LPARAM)curBar->m_hWnd);
		return;
	}
	CCrystalTextView::OnHScroll (nSBCode, nPos, pScrollBar);
}

/**
 * @brief When view is scrolled using scrollbars update location pane.
 */
void CMergeEditView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
{
	if (pScrollBar == NULL)
	{
		// Scroll did not come frome a scroll bar
		// Find the appropriate scroll bar
		// and send the message to the splitter window instead
		// The event should eventually come back here but with a valid scrollbar
		// Along the way it will be propagated to other windows that need it
		CSplitterWnd *pSplitterWnd = GetParentSplitter(this, false);
		CScrollBar* curBar = this->GetScrollBarCtrl(SB_VERT);
		pSplitterWnd->SendMessage(WM_VSCROLL,
			MAKELONG(nSBCode, nPos), (LPARAM)curBar->m_hWnd);
		return;
	}
	CCrystalTextView::OnVScroll (nSBCode, nPos, pScrollBar);

	if (nSBCode == SB_ENDSCROLL)
		return;

	// Note we cannot use nPos because of its 16-bit nature
	SCROLLINFO si = {0};
	si.cbSize = sizeof (si);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
	VERIFY (GetScrollInfo (SB_VERT, &si));

	// Get the current position of scroll	box.
	int nCurPos =	si.nPos;
	
	UpdateLocationViewPosition(nCurPos, nCurPos + GetScreenLines());
}

/**
 * @brief Copy selected lines adding linenumbers.
 */
void CMergeEditView::OnEditCopyLineNumbers()
{
	CPoint ptStart;
	CPoint ptEnd;
	CString strText;
	CString strLine;
	CString strNum;
	CString strNumLine;
	UINT line = 0;
	int nNumWidth = 0;

	CMergeDoc *pDoc = GetDocument();
	GetSelection(ptStart, ptEnd);

	// Get last selected line (having widest linenumber)
	line = pDoc->m_ptBuf[m_nThisPane]->ComputeRealLine(ptEnd.y);
	strNum.Format(_T("%d"), line + 1);
	nNumWidth = strNum.GetLength();
	
	for (int i = ptStart.y; i <= ptEnd.y; i++)
	{
		if (GetLineFlags(i) & LF_GHOST || (GetEnableHideLines() && (GetLineFlags(i) & LF_INVISIBLE)))
			continue;

		// We need to convert to real linenumbers
		line = pDoc->m_ptBuf[m_nThisPane]->ComputeRealLine(i);

		// Insert spaces to align different width linenumbers (99, 100)
		strLine = GetLineText(i);
		strNum.Format(_T("%d"), line + 1);
		CString sSpaces(' ', nNumWidth - strNum.GetLength());
		
		strText += sSpaces;
		strNumLine.Format(_T("%d: %s"), line + 1, strLine);
		strText += strNumLine;
 	}
	PutToClipboard(strText, strText.GetLength(), m_bColumnSelection);
}

void CMergeEditView::OnUpdateEditCopyLinenumbers(CCmdUI* pCmdUI)
{
	CCrystalEditViewEx::OnUpdateEditCopy(pCmdUI);
}

/**
 * @brief Open active file with associated application.
 *
 * First tries to open file using shell 'Edit' action, since that
 * action open scripts etc. to editor instead of running them. If
 * edit-action is not registered, 'Open' action is used.
 */
void CMergeEditView::OnOpenFile()
{
	CMergeDoc * pDoc = GetDocument();
	ASSERT(pDoc != NULL);

	String sFileName = pDoc->m_filePaths[m_nThisPane];
	if (sFileName.empty())
		return;
	int rtn = (int)ShellExecute(::GetDesktopWindow(), _T("edit"), sFileName.c_str(),
			0, 0, SW_SHOWNORMAL);
	if (rtn==SE_ERR_NOASSOC)
		rtn = (int)ShellExecute(::GetDesktopWindow(), _T("open"), sFileName.c_str(),
			 0, 0, SW_SHOWNORMAL);
	if (rtn==SE_ERR_NOASSOC)
		OnOpenFileWith();
}

/**
 * @brief Open active file with app selection dialog
 */
void CMergeEditView::OnOpenFileWith()
{
	CMergeDoc * pDoc = GetDocument();
	ASSERT(pDoc != NULL);

	String sFileName = pDoc->m_filePaths[m_nThisPane];
	if (sFileName.empty())
		return;

	CString sysdir;
	if (!GetSystemDirectory(sysdir.GetBuffer(MAX_PATH), MAX_PATH))
		return;
	sysdir.ReleaseBuffer();
	CString arg = (CString)_T("shell32.dll,OpenAs_RunDLL ") + sFileName.c_str();
	ShellExecute(::GetDesktopWindow(), 0, _T("RUNDLL32.EXE"), arg,
			sysdir, SW_SHOWNORMAL);
}

/**
 * @brief Open active file with external editor
 */
void CMergeEditView::OnOpenFileWithEditor()
{
	CMergeDoc * pDoc = GetDocument();
	ASSERT(pDoc != NULL);

	String sFileName = pDoc->m_filePaths[m_nThisPane];
	if (sFileName.empty())
		return;

	int nRealLine = ComputeRealLine(GetCursorPos().y) + 1;
	GetMainFrame()->OpenFileToExternalEditor(sFileName.c_str(), nRealLine);
}

/**
 * @brief Force repaint of the location pane.
 */
void CMergeEditView::RepaintLocationPane()
{
	// Must force recalculation due to caching of data in location pane.
	if (m_pLocationView)
		m_pLocationView->ForceRecalculate();
}

/**
 * @brief Enables/disables linediff (different color for diffs)
 */
void CMergeEditView::OnViewLineDiffs()
{
	bool bWordDiffHighlight = GetOptionsMgr()->GetBool(OPT_WORDDIFF_HIGHLIGHT);
	GetOptionsMgr()->SaveOption(OPT_WORDDIFF_HIGHLIGHT, !bWordDiffHighlight);

	// Call CMergeDoc RefreshOptions() to refresh *both* views
	CMergeDoc *pDoc = GetDocument();
	pDoc->RefreshOptions();
	pDoc->FlushAndRescan(true);
}

void CMergeEditView::OnUpdateViewLineDiffs(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(true);
	pCmdUI->SetCheck(GetOptionsMgr()->GetBool(OPT_WORDDIFF_HIGHLIGHT));
}

/**
 * @brief Enables/disables line number
 */
void CMergeEditView::OnViewLineNumbers()
{
	GetOptionsMgr()->SaveOption(OPT_VIEW_LINENUMBERS, !GetViewLineNumbers());

	// Call CMergeDoc RefreshOptions() to refresh *both* views
	CMergeDoc *pDoc = GetDocument();
	pDoc->RefreshOptions();
}

void CMergeEditView::OnUpdateViewLineNumbers(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(true);
	pCmdUI->SetCheck(GetViewLineNumbers());
}

/**
 * @brief Enables/disables word wrap
 */
void CMergeEditView::OnViewWordWrap()
{
	GetOptionsMgr()->SaveOption(OPT_WORDWRAP, !m_bWordWrap);

	// Call CMergeDoc RefreshOptions() to refresh *both* views
	CMergeDoc *pDoc = GetDocument();
	pDoc->RefreshOptions();
	pDoc->UpdateAllViews(this);

	UpdateCaret();
}

void CMergeEditView::OnUpdateViewWordWrap(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(true);
	pCmdUI->SetCheck(m_bWordWrap);
}

void CMergeEditView::OnSize(UINT nType, int cx, int cy) 
{
	if (!IsInitialized())
		return;

	CMergeDoc * pDoc = GetDocument();
	if (m_nThisPane < pDoc->m_nBuffers - 1)
	{
		// To calculate subline index correctly
		// we have to invalidate line cache in all pane before calling the function related the subline.
		for (int nPane = 0; nPane < pDoc->m_nBuffers; nPane++) 
		{
			CMergeEditView *pView = GetGroupView(nPane);
			if (pView)
				pView->InvalidateScreenRect(false);
		}
	}
	else
	{
		for (int nPane = 0; nPane < pDoc->m_nBuffers; nPane++) 
		{
			CMergeEditView *pView = GetGroupView(nPane);
			if (pView)
				pView->Invalidate();
		}
	}
	// recalculate m_nTopSubLine
	m_nTopSubLine = GetSubLineIndex(m_nTopLine);

	UpdateCaret();
	
	RecalcVertScrollBar (false, false);
	RecalcHorzScrollBar (false, false);
	
	((CChildFrame *)GetParentFrame())->PostMessage(WM_TIMER);
}

/**
* @brief allocates GDI resources for printing
* @param pDC [in] points to the printer device context
* @param pInfo [in] points to a CPrintInfo structure that describes the current print job
*/
void CMergeEditView::OnBeginPrinting(CDC * pDC, CPrintInfo * pInfo)
{
	((CChildFrame *)GetParentFrame())->PostMessage(WM_TIMER);

	for (int pane = 0; pane < GetDocument()->m_nBuffers; pane++)
	{
		CMergeEditView *pView = GetDocument()->GetView(pane);
		pView->m_bPrintHeader = true;
		pView->m_bPrintFooter = true;
		pView->CGhostTextView::OnBeginPrinting(pDC, pInfo);
	}
}

/**
* @brief frees GDI resources for printing
* @param pDC [in] points to the printer device context
* @param pInfo [in] points to a CPrintInfo structure that describes the current print job
*/
void CMergeEditView::OnEndPrinting(CDC * pDC, CPrintInfo * pInfo)
{
	for (int pane = 0; pane < GetDocument()->m_nBuffers; pane++)
		GetDocument()->GetView(pane)->CGhostTextView::OnEndPrinting(pDC, pInfo);

	((CChildFrame *)GetParentFrame())->PostMessage(WM_TIMER);
}

/**
* @brief Gets header text to print
* @param [in]  nPageNum the page number to print
* @param [out] header text to print
*/
void CMergeEditView::GetPrintHeaderText(int nPageNum, CString & text)
{
	text = GetDocument()->GetTitle();
}

/**
* @brief Prints header
* @param [in] nPageNum the page number to print
*/
void CMergeEditView::PrintHeader(CDC * pdc, int nPageNum)
{
	if (m_nThisPane > 0)
		return;
	int oldRight = m_rcPrintArea.right;
	m_rcPrintArea.right += m_rcPrintArea.Width() * (GetDocument()->m_nBuffers - 1);
	CGhostTextView::PrintHeader(pdc, nPageNum);
	m_rcPrintArea.right = oldRight;
}

/**
* @brief Prints footer
* @param [in] nPageNum the page number to print
*/
void CMergeEditView::PrintFooter(CDC * pdc, int nPageNum)
{
	if (m_nThisPane > 0)
		return;
	int oldRight = m_rcPrintArea.right;
	m_rcPrintArea.right += m_rcPrintArea.Width() * (GetDocument()->m_nBuffers - 1);
	CGhostTextView::PrintFooter(pdc, nPageNum);
	m_rcPrintArea.right = oldRight;
}

void CMergeEditView::RecalcPageLayouts (CDC * pDC, CPrintInfo * pInfo)
{
	for (int pane = 0; pane < GetDocument()->m_nBuffers; pane++)
		GetDocument()->GetView(pane)->CGhostTextView::RecalcPageLayouts(pDC, pInfo);
}

/**
* @brief Prints or previews both panes.
* @param pDC [in] points to the printer device context
* @param pInfo [in] points to a CPrintInfo structure that describes the current print job
*/
void CMergeEditView::OnPrint(CDC* pDC, CPrintInfo* pInfo) 
{
	CRect rDraw = pInfo->m_rectDraw;
	CSize sz = rDraw.Size();
	CMergeDoc *pDoc = GetDocument();

	SIZE szLeftTop, szRightBottom;
	GetPrintMargins(szLeftTop.cx, szLeftTop.cy, szRightBottom.cx, szRightBottom.cy);
	pDC->HIMETRICtoLP(&szLeftTop);
	pDC->HIMETRICtoLP(&szRightBottom);
	
	int midX = (sz.cx - szLeftTop.cx - szRightBottom.cx) / pDoc->m_nBuffers;

	// print pane
	for (int pane = 0; pane < pDoc->m_nBuffers; pane++)
	{
		pInfo->m_rectDraw.left = rDraw.left + midX * pane;
		pInfo->m_rectDraw.right	= pInfo->m_rectDraw.left + midX + szLeftTop.cx + szRightBottom.cx;
		CMergeEditView* pPane = pDoc->GetView(pane);
		pPane->CGhostTextView::OnPrint(pDC, pInfo);
	}
}

bool CMergeEditView::IsInitialized() const
{
	CMergeEditView * pThis = const_cast<CMergeEditView *>(this);
	CDiffTextBuffer * pBuffer = dynamic_cast<CDiffTextBuffer *>(pThis->LocateTextBuffer());
	return pBuffer->IsInitialized();
}

/**
 * @brief returns the number of empty lines which are added for synchronizing the line in two/three panes.
 */
int CMergeEditView::GetEmptySubLines( int nLineIndex )
{
	int	nBreaks[3] = {0};
	int nMaxBreaks = -1;
	CMergeDoc * pDoc = GetDocument();
	for (int nPane = 0; nPane < pDoc->m_nBuffers; nPane++) 
	{
		CMergeEditView *pView = GetGroupView(nPane);
		if (pView)
		{
			if (nLineIndex >= pView->GetLineCount())
				return 0;
			pView->WrapLineCached( nLineIndex, pView->GetScreenChars(), NULL, nBreaks[nPane] );
		}
		nMaxBreaks = max(nMaxBreaks, nBreaks[nPane]);
	}

	if (nBreaks[m_nThisPane] < nMaxBreaks)
		return nMaxBreaks - nBreaks[m_nThisPane];
	else
		return 0;
}

/**
 * @brief Invalidate sub line index cache from the specified index to the end of file.
 * @param [in] nLineIndex Index of the first line to invalidate 
 */
void CMergeEditView::InvalidateSubLineIndexCache( int nLineIndex )
{
	CMergeDoc * pDoc = GetDocument();
	ASSERT(pDoc != NULL);

    // We have to invalidate sub line index cache on both panes.
	for (int nPane = 0; nPane < pDoc->m_nBuffers; nPane++) 
	{
		CMergeEditView *pView = GetGroupView(nPane);
		if (pView)
			pView->CCrystalTextView::InvalidateSubLineIndexCache( nLineIndex );
	}
}

void CMergeEditView::SetWordWrapping( bool bWordWrap )
{
	for (int pane = 0; pane < GetDocument()->m_nBuffers; pane++)
		GetGroupView(pane)->m_bWordWrap = bWordWrap;
	CCrystalTextView::SetWordWrapping(bWordWrap);
}

/**
 * @brief Swap the positions of the two panes
 */
void CMergeEditView::OnViewSwapPanes()
{
	GetDocument()->SwapFiles();
}

/**
 * @brief Enable Swap Panes -gui.
 */
void CMergeEditView::OnUpdateViewSwapPanes(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(true);
}

/**
 * @brief Check if cursor is inside difference.
 * @return true if cursor is inside difference.
 */
bool CMergeEditView::IsCursorInDiff() const
{
	return m_bCurrentLineIsDiff;
}

/**
* @brief Determine if difference is visible on screen.
* @param [in] nDiff Number of diff to check.
* @return true if difference is visible.
*/
bool CMergeEditView::IsDiffVisible(int nDiff)
{
	CMergeDoc *pd = GetDocument();

	DIFFRANGE diff;
	pd->m_diffList.GetDiff(nDiff, diff);

	return IsDiffVisible(diff);
}

/**
 * @brief Determine if difference is visible on screen.
 * @param [in] diff diff to check.
 * @param [in] nLinesBelow Allow "minimizing" the number of visible lines.
 * @return true if difference is visible, false otherwise.
 */
bool CMergeEditView::IsDiffVisible(const DIFFRANGE& diff, int nLinesBelow /*=0*/)
{
	const int nDiffStart = GetSubLineIndex(diff.dbegin[0]);
	const int nDiffEnd = GetSubLineIndex(diff.dend[0]);
	// Diff's height is last line - first line + last line's line count
	const int nDiffHeight = nDiffEnd - nDiffStart + GetSubLines(diff.dend[0]) + 1;

	// If diff first line outside current view - context OR
	// if diff last line outside current view - context OR
	// if diff is bigger than screen
	if ((nDiffStart < m_nTopSubLine) ||
		(nDiffEnd >= m_nTopSubLine + GetScreenLines() - nLinesBelow) ||
		(nDiffHeight >= GetScreenLines()))
	{
		return false;
	}
	else
	{
		return true;
	}
}

/** @brief Open help from mainframe when user presses F1*/
void CMergeEditView::OnHelp()
{
	GetMainFrame()->ShowHelp(MergeViewHelpLocation);
}

/**
 * @brief Called after document is loaded.
 * This function is called from CMergeDoc::OpenDocs() after documents are
 * loaded. So this is good place to set View's options etc.
 */
void CMergeEditView::DocumentsLoaded()
{
	// Enable/disable automatic rescan (rescanning after edit)
	EnableRescan(GetOptionsMgr()->GetBool(OPT_AUTOMATIC_RESCAN));

	// SetTextType will revert to language dependent defaults for tab
	SetTabSize(GetOptionsMgr()->GetInt(OPT_TAB_SIZE));
	SetViewTabs(GetOptionsMgr()->GetBool(OPT_VIEW_WHITESPACE));
	const bool mixedEOLs = GetOptionsMgr()->GetBool(OPT_ALLOW_MIXED_EOL) ||
		GetDocument()->IsMixedEOL(m_nThisPane);
	SetViewEols(GetOptionsMgr()->GetBool(OPT_VIEW_WHITESPACE), mixedEOLs);
	SetWordWrapping(GetOptionsMgr()->GetBool(OPT_WORDWRAP));
	SetViewLineNumbers(GetOptionsMgr()->GetBool(OPT_VIEW_LINENUMBERS));
	SetSelectionMargin(GetOptionsMgr()->GetBool(OPT_VIEW_FILEMARGIN));

	// Enable Backspace at beginning of line
	SetDisableBSAtSOL(false);

	// Set tab type (tabs/spaces)
	bool bInsertTabs = (GetOptionsMgr()->GetInt(OPT_TAB_TYPE) == 0);
	SetInsertTabs(bInsertTabs);

	// Sometimes WinMerge doesn't update scrollbars correctly (they remain
	// disabled) after docs are open in screen. So lets make sure they are
	// really updated, even though this is unnecessary in most cases.
	RecalcHorzScrollBar();
	RecalcVertScrollBar();
}

/**
 * @brief Set LocationView pointer.
 * CLocationView calls this function to set pointer to itself,
 * so we can call locationview to update it.
 * @param [in] pView Pointer to CLocationView.
 */
void CMergeEditView::SetLocationView(const CLocationView * pView /*=NULL*/)
{
	m_pLocationView = const_cast<CLocationView *>(pView);
}

/**
 * @brief Update LocationView position.
 * This function updates LocationView position to given lines.
 * Usually we want to lines in file compare view and area in
 * LocationView to match. Be extra carefull to not call non-existing
 * LocationView.
 * @param [in] nTopLine Top line of current view.
 * @param [in] nBottomLine Bottom line of current view.
 */
void CMergeEditView::UpdateLocationViewPosition(int nTopLine /*=-1*/,
		int nBottomLine /*= -1*/)
{
	if (m_pDocument == NULL)
		return;

	if (m_pLocationView != NULL && IsWindow(m_pLocationView->GetSafeHwnd()))
	{
		m_pLocationView->UpdateVisiblePos(nTopLine, nBottomLine);
	}
}

/**
 * @brief Enable/Disable view's selection margins.
 * Selection margins show bookmarks and word-wrap symbols, so they are pretty
 * useful. But it appears many users don't use/need those features and for them
 * selection margins are just wasted screen estate.
 */
void CMergeEditView::OnViewMargin()
{
	bool bViewMargin = GetOptionsMgr()->GetBool(OPT_VIEW_FILEMARGIN);
	GetOptionsMgr()->SaveOption(OPT_VIEW_FILEMARGIN, !bViewMargin);

	SetSelectionMargin(!bViewMargin);
	CMergeDoc *pDoc = GetDocument();
	pDoc->RefreshOptions();
	pDoc->UpdateAllViews(this);
}

/**
 * @brief Update GUI for Enable/Disable view's selection margin.
 * @param [in] pCmdUI Pointer to UI item to update.
 */
void CMergeEditView::OnUpdateViewMargin(CCmdUI* pCmdUI)
{
	bool bViewMargin = GetOptionsMgr()->GetBool(OPT_VIEW_FILEMARGIN);
	pCmdUI->Enable(true);
	pCmdUI->SetCheck(bViewMargin);
}

/**
* @brief Create the "Change Scheme" sub menu.
* @param [in] pCmdUI Pointer to UI item to update.
*/
void CMergeEditView::OnUpdateViewChangeScheme(CCmdUI *pCmdUI)
{
	// Delete the place holder menu.
	pCmdUI->m_pSubMenu->DeleteMenu(0, MF_BYPOSITION);

	const HMENU hSubMenu = pCmdUI->m_pSubMenu->m_hMenu;

	String name = theApp.LoadString(ID_COLORSCHEME_FIRST);
	DoAppendMenu(hSubMenu, MF_STRING, ID_COLORSCHEME_FIRST, name.c_str());
	DoAppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);

	for (int i = ID_COLORSCHEME_FIRST + 1; i <= ID_COLORSCHEME_LAST; ++i)
	{
		name = theApp.LoadString(i);
		DoAppendMenu(hSubMenu, MF_STRING, i, name.c_str());
	}

	pCmdUI->Enable(true);
}

/**
* @brief Change the editor's syntax highlighting scheme.
* @param [in] nID Selected color scheme sub menu id.
*/
void CMergeEditView::OnChangeScheme(UINT nID)
{
	CMergeDoc *pDoc = GetDocument();
	ASSERT(pDoc != NULL);

	for (int nPane = 0; nPane < pDoc->m_nBuffers; nPane++) 
	{
		CMergeEditView *pView = GetGroupView(nPane);
		ASSERT(pView != NULL);

		if (pView != NULL)
		{
			pView->SetTextType(CCrystalTextView::TextType(nID - ID_COLORSCHEME_FIRST));
			pView->SetDisableBSAtSOL(false);
		}
	}

	pDoc->UpdateAllViews(NULL);
}

/**
* @brief Enable all color schemes sub menu items.
* @param [in] pCmdUI Pointer to UI item to update.
*/
void CMergeEditView::OnUpdateChangeScheme(CCmdUI* pCmdUI)
{
	const bool bIsCurrentScheme = (m_CurSourceDef->type == (pCmdUI->m_nID - ID_COLORSCHEME_FIRST));
	pCmdUI->SetRadio(bIsCurrentScheme);

	bool syntaxHLEnabled = GetOptionsMgr()->GetBool(OPT_SYNTAX_HIGHLIGHT);
	if (syntaxHLEnabled)
		pCmdUI->Enable(true);
	else
		pCmdUI->Enable(false);
}

/**
 * @brief Called when mouse's wheel is scrolled.
 */
BOOL CMergeEditView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if ( nFlags == MK_CONTROL )
	{
		short amount = zDelta < 0 ? -1: 1;
		ZoomText(amount);

		// no default CCrystalTextView
		return CView::OnMouseWheel(nFlags, zDelta, pt);
	}

	if (nFlags == MK_SHIFT)
	{
		SCROLLINFO si = {0};
		si.cbSize = sizeof(si);
		si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;

		VERIFY(GetScrollInfo(SB_HORZ, &si));

		// new horz pos
		si.nPos -= zDelta / 40;
		if (si.nPos > si.nMax) si.nPos = si.nMax;
		if (si.nPos < si.nMin) si.nPos = si.nMin;

		SetScrollInfo(SB_HORZ, &si);

		// for update
		SendMessage(WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, si.nPos) , NULL );

		// no default CCrystalTextView
		return CView::OnMouseWheel(nFlags, zDelta, pt);
	}

	return CGhostTextView::OnMouseWheel(nFlags, zDelta, pt);
}

/**
 * @brief Change font size (zoom) in views.
 * @param [in] amount Amount of change/zoom, negative number makes
 *  font smaller, positive number bigger and 0 reset the font size.
 */
void CMergeEditView::ZoomText(short amount)
{
	LOGFONT lf = { 0 };
	GetFont(lf);

	CDC* pDC = GetDC();
	ASSERT_VALID(pDC);
	
	if (pDC) 
	{
		const int nLogPixelsY = pDC->GetDeviceCaps(LOGPIXELSY);

		int nPointSize = -MulDiv(lf.lfHeight, 72, nLogPixelsY);

		if ( amount == 0)
		{
			nPointSize = -MulDiv(GetOptionsMgr()->GetInt(String(OPT_FONT_FILECMP) + OPT_FONT_HEIGHT), 72, nLogPixelsY);
		}

		nPointSize += amount;
		if (nPointSize < 2)
			nPointSize = 2;

		lf.lfHeight = -MulDiv(nPointSize, nLogPixelsY, 72);

		CMergeDoc *pDoc = GetDocument();
		ASSERT(pDoc != NULL);

		if (pDoc != NULL )
		{
			for (int nPane = 0; nPane < pDoc->m_nBuffers; nPane++) 
			{
				CMergeEditView *pView = GetGroupView(nPane);
				ASSERT(pView != NULL);
				
				if (pView != NULL)
				{
					pView->SetFont(lf);
				}
			}
		}
	}
}

/**
 * @brief Called when user selects View/Zoom In from menu.
 */
void CMergeEditView::OnViewZoomIn()
{
	ZoomText(1);
}

/**
 * @brief Called when user selects View/Zoom Out from menu.
 */
void CMergeEditView::OnViewZoomOut()
{
	ZoomText(-1);
}

/**
 * @brief Called when user selects View/Zoom Normal from menu.
 */
void CMergeEditView::OnViewZoomNormal()
{
	ZoomText(0);
}

/**
 * @brief Show the plugins list dialog.
 */
void CMergeEditView::OnPluginsList()
{
	PluginsListDlg dlg;
	dlg.DoModal();
}
