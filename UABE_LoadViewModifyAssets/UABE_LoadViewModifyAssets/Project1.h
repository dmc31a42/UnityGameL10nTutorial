#pragma once
#include "json/json.h"

#include "AssetsTools/AssetsFileReader.h"
#include "AssetsTools/AssetsFileFormat.h"
#include "AssetsTools/ClassDatabaseFile.h"
#include "AssetsTools/AssetsFileTable.h"
#include "AssetsTools/ResourceManagerFile.h"
#include "AssetsTools/AssetTypeClass.h"

using namespace std;

struct AssetLogicalPath {
	string AssetsName;
	string AssetName;
	string ContainerPath;
	bool hasContainerPath;
};

bool LoadAssetsFile(wstring gameFolderPath, string assetsFileName);

AssetTypeValueField * GetAssetTypeValueFieldFromJsonRecursive(AssetTypeTemplateField * assetTypeTemplateField, Json::Value json);

AssetTypeValueField * GetAssetTypeValueFieldArrayFromJson(AssetTypeTemplateField * assetTypeTemplateField, Json::Value json);

void GetClassIdFromAssetFileInfoEx(AssetsFileTable * assetsFileTable, AssetFileInfoEx * assetFileInfoEx, int & classId, UINT16 & monoClassId);

void ReplaceMaterial(string assetsName, AssetsFileTable * assetsFileTable, AssetFileInfoEx * assetFileInfoEx, AssetTypeInstance * assetTypeInstance, float _TextureHeight, float _TextureWidth);

void ReplaceAtlas(string assetsname, AssetsFileTable * assetsFileTable, AssetFileInfoEx * assetFileInfoEx, AssetTypeInstance * assetTypeInstance, int m_CompleteImageSize, string atlasPath, int m_Width, int m_Height);

bool ProcessResourceAndMonoManger(AssetsFileTable * globalgamemanagersTable, wstring gameFolderPath, string globalgamemanagersName);

bool LoadMonoClassDatabase(wstring gameFolderPath, vector<string> AssemblyNames);

bool LoadFindMonoClassNameFromMonoScriptPathId(AssetsFileTable * globalgamemanagersAssetsTable);