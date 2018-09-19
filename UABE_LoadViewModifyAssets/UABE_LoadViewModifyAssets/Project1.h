#pragma once
#include "json/json.h"
using namespace std;

bool LoadAssetsFile(wstring gameFolderPath, wstring assetsFileName);

AssetTypeValueField * GetAssetTypeValueFieldFromJsonRecursive(AssetTypeTemplateField * assetTypeTemplateField, Json::Value json);

AssetTypeValueField * GetAssetTypeValueFieldArrayFromJson(AssetTypeTemplateField * assetTypeTemplateField, Json::Value json);
