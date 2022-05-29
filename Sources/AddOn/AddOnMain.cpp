#include "APIEnvir.h"
#include "ACAPinc.h"

#include "ResourceIds.hpp"
#include "DGModule.hpp"

#include "Sight.hpp"
#include "AttributeReader.hpp"
#include "Model.hpp"

#include "exp.h"

#include "DotbimExporter.hpp"

static const GSResID AddOnInfoID			= ID_ADDON_INFO;
	static const Int32 AddOnNameID			= 1;
	static const Int32 AddOnDescriptionID	= 2;

static const GSResID AddOnMenuID			= ID_ADDON_MENU;
	static const Int32 AddOnCommandID		= 1;

static const GSResID AddOnStrsID			= ID_ADDON_STRS;
	static const Int32 FormatNameID			= 1;

static const GSType FileTypeId				= 1;

static GSErrCode ExportDotbimFile (const ModelerAPI::Model& model, const IO::Location& location)
{
	IO::File output (location, IO::File::OnNotFound::Create);
	if (output.GetStatus () != NoError) {
		return Error;
	}

	output.Open (IO::File::OpenMode::WriteEmptyMode);
	if (output.GetStatus () != NoError) {
		return Error;
	}

	std::string dotbimContent = ExportDotbim (model);
	output.WriteBin (dotbimContent.c_str (), (USize) dotbimContent.length ());

	output.Close ();
	return NoError;
}

static GSErrCode ExportDotbimFromSaveAs (const API_IOParams* ioParams, Modeler::SightPtr sight)
{
	AttributeReader attributeReader;
	ModelerAPI::Model model;
	if (EXPGetModel (sight, &model, &attributeReader) != NoError) {
		return Error;
	}

	return ExportDotbimFile (model, *ioParams->fileLoc);
}

static GSErrCode ExportDotbimFromMenu ()
{
	API_WindowInfo newWindowInfo = {};
	newWindowInfo.typeID = APIWind_3DModelID;
	ACAPI_Automate (APIDo_ChangeWindowID, &newWindowInfo);

	AttributeReader attributeReader;
	ModelerAPI::Model model;
	void* currentSight = nullptr;
	if (ACAPI_3D_GetCurrentWindowSight (&currentSight) != NoError) {
		return Error;
	}

	Modeler::SightPtr sightPtr ((Modeler::Sight*) currentSight);
	Modeler::ConstModel3DPtr modelPtr (sightPtr->GetMainModelPtr ());
	if (EXPGetModel (modelPtr, &model, &attributeReader) != NoError) {
		return Error;
	}

	DG::FileDialog saveFileDialog (DG::FileDialog::Type::Save);
	GS::UniString fileTypeString = RSGetIndString (AddOnStrsID, FormatNameID, ACAPI_GetOwnResModule ());
	FTM::FileTypeManager fileTypeManager (fileTypeString.ToCStr (0, MaxUSize, CC_UTF8));
	FTM::FileType fileType (nullptr, "bim", 0, 0, 0);
	FTM::TypeID fileTypeId = FTM::FileTypeManager::SearchForType (fileType);
	saveFileDialog.AddFilter (fileTypeId);

	if (!saveFileDialog.Invoke ()) {
		return Error;
	}

	const IO::Location& location = saveFileDialog.GetSelectedFile ();
	return ExportDotbimFile (model, location);
}

static GSErrCode MenuCommandHandler (const API_MenuParams *menuParams)
{
	switch (menuParams->menuItemRef.menuResID) {
		case AddOnMenuID:
			switch (menuParams->menuItemRef.itemIndex) {
				case AddOnCommandID:
					ExportDotbimFromMenu ();
					break;
			}
			break;
	}
	return NoError;
}

API_AddonType __ACDLL_CALL CheckEnvironment (API_EnvirParams* envir)
{
	RSGetIndString (&envir->addOnInfo.name, AddOnInfoID, AddOnNameID, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, AddOnInfoID, AddOnDescriptionID, ACAPI_GetOwnResModule ());

	return APIAddon_Normal;
}

GSErrCode __ACDLL_CALL RegisterInterface (void)
{
	GSErrCode err = NoError;

	err = ACAPI_Register_Menu (AddOnMenuID, 0, MenuCode_Interoperability, MenuFlag_Default);
	if (err != NoError) {
		return err;
	}

	err = ACAPI_Register_FileType (
		FileTypeId,
		'DBIM',
		'GSAC',
		"bim",
		ID_ADDON_ICON,
		AddOnStrsID,
		FormatNameID,
		SaveAs3DSupported
	);
	if (err != NoError) {
		return err;
	}

	return NoError;
}

GSErrCode __ACENV_CALL Initialize (void)
{
	GSErrCode err = NoError;

	err = ACAPI_Install_MenuHandler (AddOnMenuID, MenuCommandHandler);
	if (err != NoError) {
		return err;
	}

	err = ACAPI_Install_FileTypeHandler3D (FileTypeId, ExportDotbimFromSaveAs);
	if (err != NoError) {
		return err;
	}

	return NoError;
}

GSErrCode __ACENV_CALL FreeData (void)
{
	return NoError;
}
