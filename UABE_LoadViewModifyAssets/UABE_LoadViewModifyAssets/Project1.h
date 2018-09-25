#pragma once
#include "json/json.h"
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

void ReplaceMaterial(string assetsName, AssetsFileTable * assetsFileTable, AssetFileInfoEx * assetFileInfoEx, AssetTypeInstance * assetTypeInstance, float _TextureHeight, float _TextureWidth);

void ReplaceAtlas(string assetsname, AssetsFileTable * assetsFileTable, AssetFileInfoEx * assetFileInfoEx, AssetTypeInstance * assetTypeInstance, int m_CompleteImageSize, string atlasPath, int m_Width, int m_Height);

string readFile2(const string & fileName);

bool copyFile(const char * SRC, const char * DEST);

bool copyFile(const wchar_t * SRC, const wchar_t * DEST);

vector<wstring> get_all_files_names_within_folder(wstring filter);

bool CreateProcessCustom(wstring commandLine);

bool ProcessResourceAndMonoManger(AssetsFileTable * globalgamemanagersTable, wstring gameFolderPath, string globalgamemanagersName);

bool LoadFindMonoClassNameFromMonoScriptPathId(AssetsFileTable * globalgamemanagersAssetsTable);