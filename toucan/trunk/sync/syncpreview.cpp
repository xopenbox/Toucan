/////////////////////////////////////////////////////////////////////////////////
// Author:      Steven Lamerton
// Copyright:   Copyright (C) 2009 Steven Lamerton
// License:     GNU GPL 2 (See readme for more info)
/////////////////////////////////////////////////////////////////////////////////

#include "syncbase.h"
#include "syncpreview.h"
#include "../toucan.h"
#include "../forms/frmprogress.h"
#include <list>
#include <map>
#include <wx/string.h>
#include <wx/wfstream.h>

SyncPreview::SyncPreview(wxString syncsource, wxString syncdest, SyncData* syncdata, Rules syncrules, bool issource) : SyncFiles(syncsource, syncdest, syncdata, syncrules){
	this->sourcetree = issource;
	this->preview = true;
}

VdtcTreeItemBaseArray SyncPreview::Execute(){
	std::list<wxString> sourcepaths = FolderContentsToList(sourceroot);
	std::list<wxString> destpaths = FolderContentsToList(destroot);
	std::map<wxString, short> mergeresult = MergeListsToMap(sourcepaths, destpaths);
	OperationCaller(mergeresult);
	if(sourcetree){
		return sourceitems;
	}
	else{
		return destitems;		
	}

}

bool SyncPreview::OperationCaller(std::map<wxString, short> paths){
	for(std::map<wxString, short>::iterator iter = paths.begin(); iter != paths.end(); ++iter){
		if(wxDirExists(sourceroot + wxFILE_SEP_PATH + (*iter).first) || wxDirExists(destroot + wxFILE_SEP_PATH + (*iter).first)){
			if((*iter).second == 1){
				sourceitems.Add(new VdtcTreeItemBase(VDTC_TI_DIR, (*iter).first));
				OnSourceNotDestFolder((*iter).first);
			}
			else if((*iter).second == 2){
				destitems.Add(new VdtcTreeItemBase(VDTC_TI_DIR, (*iter).first));
				OnNotSourceDestFolder((*iter).first);		
			}
			else if((*iter).second == 3){
				sourceitems.Add(new VdtcTreeItemBase(VDTC_TI_DIR, (*iter).first));
				destitems.Add(new VdtcTreeItemBase(VDTC_TI_DIR, (*iter).first));
				OnSourceAndDestFolder((*iter).first);
			}
		}
		//We have a file
		else{
			if((*iter).second == 1){
				sourceitems.Add(new VdtcTreeItemBase(VDTC_TI_FILE, (*iter).first));
				OnSourceNotDestFile((*iter).first);
			}
			else if((*iter).second == 2){
				destitems.Add(new VdtcTreeItemBase(VDTC_TI_FILE, (*iter).first));
				OnNotSourceDestFile((*iter).first);
			}
			else if((*iter).second == 3){
				sourceitems.Add(new VdtcTreeItemBase(VDTC_TI_FILE, (*iter).first));
				destitems.Add(new VdtcTreeItemBase(VDTC_TI_FILE, (*iter).first));
				OnSourceAndDestFile((*iter).first);
			}
		}
	}
	return true;
}

bool SyncPreview::OnSourceNotDestFile(wxString path){
	if(data->GetFunction() != _("Clean")){
		wxString source = sourceroot + wxFILE_SEP_PATH + path;
		wxString dest = destroot + wxFILE_SEP_PATH + path;
		VdtcTreeItemBase* item = new VdtcTreeItemBase(VDTC_TI_FILE, path);
		if(!rules.ShouldExclude(source, false)){
			item->SetColour(wxT("Blue"));
			destitems.Add(item);
			if(data->GetFunction() == _("Move")){
				int pos = GetItemLocation(path, &sourceitems);
				if(pos != -1){
					sourceitems.Item(pos)->SetColour(wxT("Grey"));						
				}
			}
		}
		else{
			delete item;
		}	
	}
	return true;
}

bool SyncPreview::OnNotSourceDestFile(wxString path){
	wxString source = sourceroot + wxFILE_SEP_PATH + path;
	wxString dest = destroot + wxFILE_SEP_PATH + path;
	if(!rules.ShouldExclude(dest, false)){
		if(data->GetFunction() == _("Mirror") || data->GetFunction() == _("Clean")){
			int pos = GetItemLocation(path, &destitems);
			if(pos != -1){
				if(!rules.ShouldExclude(dest, false)){
					destitems.Item(pos)->SetColour(wxT("Grey"));						
				}
			}
		}
		else if(data->GetFunction() == _("Equalise")){
			VdtcTreeItemBase* item = new VdtcTreeItemBase(VDTC_TI_FILE, path);
			if(!rules.ShouldExclude(dest, false)){
				item->SetColour(wxT("Blue"));
				sourceitems.Add(item);
			}
			else{
				delete item;
			}
		}
	}
	return true;
}

bool SyncPreview::OnSourceAndDestFile(wxString path){
	wxString source = sourceroot + wxFILE_SEP_PATH + path;
	wxString dest = destroot + wxFILE_SEP_PATH + path;
	if(!rules.ShouldExclude(dest, false)){
		if(data->GetFunction() == _("Copy") || data->GetFunction() == _("Mirror") || data->GetFunction() == _("Move")){
			if(ShouldCopy(source, dest)){
				int pos = GetItemLocation(path, &destitems);
				if(pos != -1){
					destitems.Item(pos)->SetColour(wxT("Green"));		
					if(data->GetFunction() == _("Move")){
						int pos = GetItemLocation(path, &sourceitems);
						if(pos != -1){
							sourceitems.Item(pos)->SetColour(wxT("Grey"));						
						}
					}
				}
			}		
		}
		else if(data->GetFunction() == _("Update")){
			wxDateTime tmTo, tmFrom;
			wxFileName flTo(dest);
			wxFileName flFrom(source);
			flTo.GetTimes(NULL, &tmTo, NULL);
			flFrom.GetTimes(NULL, &tmFrom, NULL);		

			if(data->GetIgnoreDLS()){
				tmFrom.MakeTimezone(wxDateTime::Local, true);
			}

			if(tmFrom.IsLaterThan(tmTo)){
				if(ShouldCopy(source, dest)){
					int pos = GetItemLocation(path, &destitems);
					if(pos != -1){
						destitems.Item(pos)->SetColour(wxT("Green"));			
					}
				}	
			}
		}
		else if(data->GetFunction() == _("Equalise")){
			wxDateTime tmTo, tmFrom;
			wxFileName flTo(dest);
			wxFileName flFrom(source);
			flTo.GetTimes(NULL, &tmTo, NULL);
			flFrom.GetTimes(NULL, &tmFrom, NULL);		

			if(data->GetIgnoreDLS()){
				tmFrom.MakeTimezone(wxDateTime::Local, true);
			}

			if(tmFrom.IsLaterThan(tmTo)){
				if(ShouldCopy(source, dest)){
					int pos = GetItemLocation(path, &destitems);
					if(pos != -1){
						destitems.Item(pos)->SetColour(wxT("Green"));			
					}
				}	
			}
			else if(tmTo.IsLaterThan(tmFrom)){
				if(ShouldCopy(dest, source)){
					int pos = GetItemLocation(path, &sourceitems);
					if(pos != -1){
						sourceitems.Item(pos)->SetColour(wxT("Green"));			
					}
				}					
			}
		}
	}
	return true;
}
bool SyncPreview::OnSourceNotDestFolder(wxString path){
	if(data->GetFunction() != _("Clean")){
		wxString source = sourceroot + wxFILE_SEP_PATH + path;
		wxString dest = destroot + wxFILE_SEP_PATH + path;
		VdtcTreeItemBase* item = new VdtcTreeItemBase(VDTC_TI_DIR, path);
		if(!rules.ShouldExclude(source, true)){
			item->SetColour(wxT("Blue"));
		}
		else{
			item->SetColour(wxT("Red"));
		}
		destitems.Add(item);
		if(data->GetFunction() == _("Move")){
			int pos = GetItemLocation(path, &sourceitems);
			if(pos != -1){
				sourceitems.Item(pos)->SetColour(wxT("Red"));						
			}
		}
	}
	return true;
}

bool SyncPreview::OnNotSourceDestFolder(wxString path){
	wxString source = sourceroot + wxFILE_SEP_PATH + path;
	wxString dest = destroot + wxFILE_SEP_PATH + path;
	if(data->GetFunction() == _("Mirror") || data->GetFunction() == _("Clean")){
		int pos = GetItemLocation(path, &destitems);
		if(pos != -1){
			if(!rules.ShouldExclude(dest, true)){
				destitems.Item(pos)->SetColour(wxT("Grey"));				
			}		
		}
	}
	else if(data->GetFunction() == _("Equalise")){
		VdtcTreeItemBase* item = new VdtcTreeItemBase(VDTC_TI_DIR, path);
		if(!rules.ShouldExclude(dest, true)){
			item->SetColour(wxT("Blue"));

		}
		else{
			item->SetColour(wxT("Red"));
		}
		sourceitems.Add(item);
	}
	return true;
}
bool SyncPreview::OnSourceAndDestFolder(wxString path){
	wxString source = sourceroot + wxFILE_SEP_PATH + path;
	wxString dest = destroot + wxFILE_SEP_PATH + path;
	if(data->GetFunction() == _("Move")){
		int pos = GetItemLocation(path, &sourceitems);
		if(pos != -1){
			if(!rules.ShouldExclude(source, true)){
				sourceitems.Item(pos)->SetColour(wxT("Red"));				
			}		
		}
	}
	return true;
}

int SyncPreview::GetItemLocation(wxString path, VdtcTreeItemBaseArray* array){
	for(unsigned int i = 0; i < array->GetCount(); i++){
		if(array->Item(i)->GetName() == path){
			return i;
		}
	}
	return -1;
}

bool SyncPreview::ShouldCopy(wxString source, wxString dest){
	if(data->GetDisableHash()){
		return true;
	}
	//See the real CopyFileHash for more info
	wxFileInputStream sourcestream(source);
	wxFileInputStream deststream(dest);
	if(sourcestream.GetLength() != deststream.GetLength()){
		return true;	
	}
	//Something is wrong with out streams, return error
	if(!sourcestream.IsOk() || !deststream.IsOk()){
		return false;
	}
	//Large files take forever to read (I think the boundary is 2GB), better off just to copy
	wxFileOffset size = sourcestream.GetLength();
	if(size > 2000000000){
		return true;
	}
	//We read in 1MB chunks
	char sourcebuf[1000000];
	char destbuf[1000000];
	wxFileOffset bytesLeft=size;
	while(bytesLeft > 0){
		wxGetApp().Yield();
		wxFileOffset bytesToRead=wxMin((wxFileOffset) sizeof(sourcebuf),bytesLeft);
		sourcestream.Read((void*)sourcebuf,bytesToRead);
		deststream.Read((void*)destbuf,bytesToRead);
		if(sourcestream.GetLastError() != wxSTREAM_NO_ERROR || deststream.GetLastError() != wxSTREAM_NO_ERROR){
			return false;
		}
		if(strncmp(sourcebuf, destbuf, bytesToRead)){
			return true;
		}
		bytesLeft-=bytesToRead;
	}
	//The two files are actually the same
	return false;
}
