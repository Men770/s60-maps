/*
 ============================================================================
 Name		: S60MapsAppUi.cpp
 Author	  : artem78
 Copyright   : 
 Description : CS60MapsAppUi implementation
 ============================================================================
 */

// INCLUDE FILES
#include <avkon.hrh>
#include <aknmessagequerydialog.h>
#include <aknnotewrappers.h>
#include <stringloader.h>
#include <s32file.h>
#include <hlplch.h>

#include <S60Maps_0xED689B88.rsg>

#ifdef _HELP_AVAILABLE_
#include "S60Maps_0xED689B88.hlp.hrh"
#endif
#include "S60Maps.hrh"
#include "S60Maps.pan"
#include "S60MapsApplication.h"
#include "S60MapsAppUi.h"
#include "S60MapsAppView.h"
#include "Defs.h"
#ifdef _DEBUG
#include "GitInfo.h"
#endif
#include "FileUtils.h"


// ============================ MEMBER FUNCTIONS ===============================


// -----------------------------------------------------------------------------
// CS60MapsAppUi::ConstructL()
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CS60MapsAppUi::ConstructL()
	{
	// Initialise app UI with standard value.
	BaseConstructL(CAknAppUi::EAknEnableSkin);
	
	iFileMan = CFileMan::NewL(CCoeEnv::Static()->FsSession(), this);

	// Set initial map position
	TCoordinate position = TCoordinate(47.100, 5.361); // Center of Europe
	TZoom zoom = 2;	
	
	// Create view object
	iAppView = CS60MapsAppView::NewL(ClientRect(), position, zoom);
	AddToStackL(iAppView);
	
	// Position requestor
	_LIT(KPosRequestorName, "S60 Maps"); // ToDo: Move to global const
	iPosRequestor = CPositionRequestor::NewL(this, KPosRequestorName);
	iPosRequestor->Start(); // Must be started after view created
	
	// Media keys catching
	iInterfaceSelector = CRemConInterfaceSelector::NewL();
	iCoreTarget = CRemConCoreApiTarget::NewL(*iInterfaceSelector, *this);
	iInterfaceSelector->OpenTargetL();
	
	// Make fullscreen
	SetFullScreenApp(ETrue); // Seems no effect
	StatusPane()->MakeVisible(EFalse); // ToDo: Why if call it before creating
									   // app view panic KERN-EXEC 3 happens?
	//Cba()->MakeVisible(EFalse); // Softkeys not work after this
	iAppView->SetRect(ApplicationRect()); // Need to resize the view to fullscreen
	}
// -----------------------------------------------------------------------------
// CS60MapsAppUi::CS60MapsAppUi()
// C++ default constructor can NOT contain any code, that might leave.
// -----------------------------------------------------------------------------
//
CS60MapsAppUi::CS60MapsAppUi()
	{
	// No implementation required
	}

// -----------------------------------------------------------------------------
// CS60MapsAppUi::~CS60MapsAppUi()
// Destructor.
// -----------------------------------------------------------------------------
//
CS60MapsAppUi::~CS60MapsAppUi()
	{
	//delete iCoreTarget;
	/* Panic KERN-EXEC 3 - Seems that there are no need to manually destroy core target,
	 because interface selector brings ownership and will delete target by itself. */
	
	delete iInterfaceSelector;
	
	delete iPosRequestor;
	
	if (iAppView)
		{
		//if (IsControlOnStack(iAppView))
			RemoveFromStack(iAppView);
		delete iAppView;
		iAppView = NULL;
		}
	
	delete iFileMan;
	}

// -----------------------------------------------------------------------------
// CS60MapsAppUi::HandleCommandL()
// Takes care of command handling.
// -----------------------------------------------------------------------------
//
void CS60MapsAppUi::HandleCommandL(TInt aCommand)
	{
	switch (aCommand)
		{
		case EEikCmdExit:
		case EAknSoftkeyExit:
			{
			CAknMessageQueryDialog* dlg = new (ELeave) CAknMessageQueryDialog();
			dlg->PrepareLC(R_CONFIRM_EXIT_QUERY_DIALOG);
			HBufC* title = iEikonEnv->AllocReadResourceLC(R_CONFIRM_EXIT_DIALOG_TITLE);
			dlg->QueryHeading()->SetTextL(*title);
			CleanupStack::PopAndDestroy(); //title
			HBufC* msg = iEikonEnv->AllocReadResourceLC(R_CONFIRM_EXIT_DIALOG_TEXT);
			dlg->SetMessageTextL(*msg);
			CleanupStack::PopAndDestroy(); //msg
			TInt res = dlg->RunLD();
			if (res == 3005 /*Yes*/) // ToDo: Replace by constant name
				{
				SaveL();
				Exit();
				}
			}
			break;
		case EFindMe:
			{
			iAppView->SetFollowUser(ETrue);
			}
			break;
		case ETilesCacheStats:
			{
			ShowMapCacheStatsDialogL();
			}
			break;
		case EResetTilesCache:
			{
			CAknMessageQueryDialog* dlg = new (ELeave) CAknMessageQueryDialog();
			dlg->PrepareLC(R_CONFIRM_RESET_TILES_CACHE_DIALOG);
			HBufC* title = iEikonEnv->AllocReadResourceLC(R_CONFIRM_RESET_TILES_CACHE_DIALOG_TITLE);
			dlg->QueryHeading()->SetTextL(*title);
			CleanupStack::PopAndDestroy(); //title
			HBufC* msg = iEikonEnv->AllocReadResourceLC(R_CONFIRM_RESET_TILES_CACHE_DIALOG_TEXT);
			dlg->SetMessageTextL(*msg);
			CleanupStack::PopAndDestroy(); //msg
			TInt res = dlg->RunLD();
			if (res == 3005 /*Yes*/) // ToDo: Replace by constant name
				{
				ClearTilesCache();
				}
			}
			break;
		case EHelp:
			{

			CArrayFix<TCoeHelpContext>* buf = CCoeAppUi::AppHelpContextL();
			HlpLauncher::LaunchHelpApplicationL(iEikonEnv->WsSession(), buf);
			}
			break;
		case EAbout:
			{

			CAknMessageQueryDialog* dlg = new (ELeave) CAknMessageQueryDialog();
			dlg->PrepareLC(R_ABOUT_QUERY_DIALOG);
			HBufC* title = iEikonEnv->AllocReadResourceLC(R_ABOUT_DIALOG_TITLE);
			dlg->QueryHeading()->SetTextL(*title);
			CleanupStack::PopAndDestroy(); //title
#ifdef _DEBUG
			HBufC* msg = iEikonEnv->AllocReadResourceLC(R_ABOUT_DIALOG_TEXT);
			HBufC* gitMsg = iEikonEnv->AllocReadResourceLC(R_ABOUT_DIALOG_GIT_TEXT);
			RBuf buff;
			buff.CreateL(msg->Length() + gitMsg->Length() + 100);
			buff.CleanupClosePushL();
			buff.Zero();
			buff.Append(*msg);
			buff.AppendFormat(*gitMsg, &KGITBranch, &KGITCommit);
			dlg->SetMessageTextL(buff);
			CleanupStack::PopAndDestroy(3, msg);
			dlg->RunLD();
#else
			HBufC* msg = iEikonEnv->AllocReadResourceLC(R_ABOUT_DIALOG_TEXT);
			dlg->SetMessageTextL(*msg);
			CleanupStack::PopAndDestroy(); //msg
			dlg->RunLD();
#endif
			}
			break;
		default:
			Panic( ES60MapsUi);
			break;
		}
	}
// -----------------------------------------------------------------------------
//  Called by the framework when the application status pane
//  size is changed.  Passes the new client rectangle to the
//  AppView
// -----------------------------------------------------------------------------
//
void CS60MapsAppUi::HandleStatusPaneSizeChange()
	{
	//iAppView->SetRect(ClientRect());
	iAppView->SetRect(ApplicationRect());
	}

CArrayFix<TCoeHelpContext>* CS60MapsAppUi::HelpContextL() const
	{
#warning "Please see comment about help and UID3..."
	// Note: Help will not work if the application uid3 is not in the
	// protected range.  The default uid3 range for projects created
	// from this template (0xE0000000 - 0xEFFFFFFF) are not in the protected range so that they
	// can be self signed and installed on the device during testing.
	// Once you get your official uid3 from Symbian Ltd. and find/replace
	// all occurrences of uid3 in your project, the context help will
	// work. Alternatively, a patch now exists for the versions of 
	// HTML help compiler in SDKs and can be found here along with an FAQ:
	// http://www3.symbian.com/faq.nsf/AllByDate/E9DF3257FD565A658025733900805EA2?OpenDocument
#ifdef _HELP_AVAILABLE_
	CArrayFixFlat<TCoeHelpContext>* array = new(ELeave)CArrayFixFlat<TCoeHelpContext>(1);
	CleanupStack::PushL(array);
	array->AppendL(TCoeHelpContext(KUidS60MapsApp, KGeneral_Information));
	CleanupStack::Pop(array);
	return array;
#else
	return NULL;
#endif
	}

TStreamId CS60MapsAppUi::StoreL(CStreamStore& aStore) const
	{
	RStoreWriteStream stream;
	TStreamId id = stream.CreateLC(aStore);
	stream << *this;
	stream.CommitL() ;
	CleanupStack::PopAndDestroy(&stream);
	return id;
	}

void CS60MapsAppUi::RestoreL(const CStreamStore& aStore,
		TStreamId aStreamId)
	{
	RStoreReadStream stream;
	stream.OpenLC(aStore, aStreamId);
	stream >> *this;
	CleanupStack::PopAndDestroy(&stream);
	}

void CS60MapsAppUi::ExternalizeL(RWriteStream& aStream) const
	{
	aStream << *iAppView;
	}

void CS60MapsAppUi::InternalizeL(RReadStream& aStream)
	{
	aStream >> *iAppView;
	}

MFileManObserver::TControl CS60MapsAppUi::NotifyFileManStarted()
	{
	return EContinue;
	}

MFileManObserver::TControl CS60MapsAppUi::NotifyFileManOperation()
	{
	return EContinue;
	}

MFileManObserver::TControl CS60MapsAppUi::NotifyFileManEnded()
	{
	return EContinue;
	}

void CS60MapsAppUi::ClearTilesCache()
	{
	TFileName cacheDir;
	static_cast<CS60MapsApplication *>(Application())->CacheDir(cacheDir);

	// ToDo: Show loading/progress bar during operation
	// ToDo: Do asynchronous
	iFileMan->RmDir(cacheDir);
	
	_LIT(KMsg, "Done!");
	CEikonEnv::Static()->AlertWin(KMsg);
	}

void CS60MapsAppUi::OnPositionUpdated()
	{
	const TPositionInfo* posInfo = iPosRequestor->LastKnownPositionInfo();
	TPosition pos;
	posInfo->GetPosition(pos);
	TCoordinateEx coord = pos;
	coord.SetCourse(KNaN);
	if (posInfo->PositionClassType() & EPositionCourseInfoClass)
		{
		const TPositionCourseInfo* courseInfo =
				static_cast<const TPositionCourseInfo*>(posInfo);
		TCourse course;
		courseInfo->GetCourse(course);
		
		coord.SetCourse(course.Heading());
		}
	iAppView->SetUserPosition(coord);
	}

void CS60MapsAppUi::OnPositionPartialUpdated()
	{
	
	}

void CS60MapsAppUi::OnPositionRestored()
	{
	iAppView->ShowUserPosition();
	}

void CS60MapsAppUi::OnPositionLost()
	{
	iAppView->HideUserPosition();
	}

void CS60MapsAppUi::OnPositionError(TInt /*aErrCode*/)
	{
	
	}

void CS60MapsAppUi::MrccatoCommand(TRemConCoreApiOperationId aOperationId,
		TRemConCoreApiButtonAction aButtonAct)
	{
	//TRequestStatus status;
	
	if (aButtonAct == ERemConCoreApiButtonClick)
		{
		switch (aOperationId)
			{
			case ERemConCoreApiVolumeUp:
				{
				//LOG(_L8("VolumeUp pressed\n"));
				iAppView->ZoomIn();
				
				/*iCoreTarget->VolumeUpResponse(status, KErrNone);
				User::WaitForRequest(status);*/
				}
			break;
			
			case ERemConCoreApiVolumeDown:
				{
				//LOG(_L8("VolumeDown pressed\n"));
				iAppView->ZoomOut();
				
				/*iCoreTarget->VolumeDownResponse(status, KErrNone);
				User::WaitForRequest(status);*/
				}
			break;
			
			default:
			break;
			}
		}
	}

void CS60MapsAppUi::ShowMapCacheStatsDialogL()
	{
	CS60MapsApplication* app = static_cast<CS60MapsApplication *>(Application());
	RFs fs = iEikonEnv->FsSession();
	
	// Prepare information text
	RBuf msg;
	msg.CreateL(2048);
	msg.CleanupClosePushL();
	
	TInt filesTotal = 0, bytesTotal = 0;
	
	TFileName baseCacheDir;
	app->CacheDir(baseCacheDir);
	
	CDir* cacheSubDirs = NULL;
	TInt r = fs.GetDir(baseCacheDir, KEntryAttDir, ESortByName, cacheSubDirs);
	if (r == KErrNone && cacheSubDirs != NULL)
		{
		for (TInt i = 0; i < cacheSubDirs->Count(); i++)
			{
			const TEntry &cacheSubDir = (*cacheSubDirs)[i];
			
			// Seems that KEntryAttDir doesn`t work
			if (!cacheSubDir.IsDir())
				continue;
			
			TDirStats dirStats;
			RBuf subDirFullPath;
			subDirFullPath.Create(KMaxFileName);
			subDirFullPath.Copy(baseCacheDir);
			TParsePtr parser(subDirFullPath);
			parser.AddDir(cacheSubDir.iName);
			r = FileUtils::DirectoryStats(fs, parser.FullName(), dirStats);
			subDirFullPath.Close();
			if (r != KErrNone)
				{ // Something went wrong
				dirStats.iFilesCount = 0;
				dirStats.iSize = 0;
				}
			
			filesTotal += dirStats.iFilesCount;
			bytesTotal += dirStats.iSize;
			
			TBuf<16> sizeBuff;
			FileUtils::FileSizeToReadableString(dirStats.iSize, sizeBuff);
			msg.AppendFormat(_L("%S: %d files, %S\n"), &cacheSubDir.iName, dirStats.iFilesCount, &sizeBuff);
			}
		
		delete cacheSubDirs;
		}
	
	msg.Append(_L("------------\n"));
	TBuf<16> totalSizeBuff;
	FileUtils::FileSizeToReadableString(bytesTotal, totalSizeBuff);
	msg.AppendFormat(_L("Total: %d files, %S"), filesTotal, &totalSizeBuff);
	
	
	
	// Show information
	CAknMessageQueryDialog* dlg = new (ELeave) CAknMessageQueryDialog();
	dlg->PrepareLC(R_MAP_CACHE_STATS_DIALOG);
	HBufC* title = iEikonEnv->AllocReadResourceLC(R_MAP_CACHE_STATS_DIALOG_TITLE);
	dlg->QueryHeading()->SetTextL(*title);
	CleanupStack::PopAndDestroy(title);
	dlg->SetMessageTextL(msg);
	//CleanupStack::PopAndDestroy(&msg);
	dlg->RunLD();
	
	CleanupStack::PopAndDestroy(&msg);
	//CleanupStack::PopAndDestroy(/*3*/2, &msg);
	}


// End of File
