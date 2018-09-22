#pragma once
#include "json/json.h"
using namespace std;

bool LoadAssetsFile(wstring gameFolderPath, wstring assetsFileName);

AssetTypeValueField * GetAssetTypeValueFieldFromJsonRecursive(AssetTypeTemplateField * assetTypeTemplateField, Json::Value json);

AssetTypeValueField * GetAssetTypeValueFieldArrayFromJson(AssetTypeTemplateField * assetTypeTemplateField, Json::Value json);

void ReplaceMaterial(string assetsName, AssetsFileTable * assetsFileTable, AssetFileInfoEx * assetFileInfoEx, AssetTypeInstance * assetTypeInstance, float _TextureHeight, float _TextureWidth);

void ReplaceAtlas(string assetsname, AssetsFileTable * assetsFileTable, AssetFileInfoEx * assetFileInfoEx, AssetTypeInstance * assetTypeInstance, int m_CompleteImageSize, string atlasPath, int m_Width, int m_Height);
