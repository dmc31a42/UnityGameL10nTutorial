// Project1.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include <iostream>
#include <map>
#include <codecvt> // for std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
#include <algorithm>    // std::find

#include "AssetsTools\AssetsFileReader.h"
#include "AssetsTools\AssetsFileFormat.h"
#include "AssetsTools/ClassDatabaseFile.h"
#include "AssetsTools/AssetsFileTable.h"
#include "AssetsTools/ResourceManagerFile.h"
#include "AssetsTools/AssetTypeClass.h"
#include "Project1.h"
#include "json/json.h"


using namespace std;

ClassDatabaseFile* classDatabaseFile;
ClassDatabaseFile* monoDatabaseFile;
ResourceManagerFile* resourceManagerFile;
map <wstring, AssetsFile*> mapAssetsFiles;
map <wstring, AssetsFileTable*> mapAssetsFileTables;
vector<wstring> vectorAssetsFileNames;
map<unsigned __int64, string> findMonoScriptFullNameByPathID;
map <int, unsigned int> findByClassID;
map<string, unsigned int> findMonoByName;
AssetsFileTable* globalgamemanagersAssetsTable;
map<string, vector<AssetsReplacer*>> AssetsReplacerForEachAssets;

// https://stackoverflow.com/questions/116038/what-is-the-best-way-to-read-an-entire-file-into-a-stdstring-in-c#
string readFile2(const string &fileName)
{
	ifstream ifs(fileName.c_str(), ios::in | ios::binary | ios::ate);

	ifstream::pos_type fileSize = ifs.tellg();
	if (fileSize < 0)
		return std::string();

		ifs.seekg(0, ios::beg);

	vector<char> bytes(fileSize);
	ifs.read(&bytes[0], fileSize);

	return string(&bytes[0], fileSize);
}

bool getResourceManagerAndAssembly(
	AssetsFileTable* globalgamemanagersTable,
	wstring gameFolderPath,
	wstring globalgamemanagersName) {
	AssetsFile* globalgamemanagersFile = globalgamemanagersTable->getAssetsFile();
	int ResourceManagerClassId;
	int MonoManagerClassId;
	vector<string> AssemblyNames;

	int ClassIdSearchCount = 2;
	// findBy로 통합할 것
	for (vector<ClassDatabaseType>::iterator iterator = classDatabaseFile->classes.begin();
		(iterator != classDatabaseFile->classes.end()) && ClassIdSearchCount >0; iterator++) {
		const char* classDatabaseTypeName = iterator->name.GetString(classDatabaseFile);
		if (strcmp(classDatabaseTypeName, "ResourceManager") == 0) {
			ResourceManagerClassId = iterator->classId;
			ClassIdSearchCount--;
			continue;
		}
		else if (strcmp(classDatabaseTypeName, "MonoManager") == 0) {
			MonoManagerClassId = iterator->classId;
			ClassIdSearchCount--;
			continue;
		}
	}

	int AssetSearchCount = 2;
	for (unsigned int i = 0;
		(i < globalgamemanagersTable->assetFileInfoCount) && AssetSearchCount>0;
		i++) {
		AssetFileInfoEx assetFileInfoEx = globalgamemanagersTable->pAssetFileInfo[i];
		DWORD classId;
		//함수로 통합
		if (globalgamemanagersFile->header.format < 0x10) {
			classId = assetFileInfoEx.curFileType;
		}
		else {
			classId = globalgamemanagersFile->typeTree.pTypes_Unity5[assetFileInfoEx.curFileTypeOrIndex].classId;
		}
		if (classId == ResourceManagerClassId) {
			//assetFileInfoEx.absolutePos
			resourceManagerFile = new ResourceManagerFile();
			/*AssetFile* resourceManagerAssetFile = new AssetFile;
			globalgamemanagersFile->GetAssetFile(assetFileInfoEx.absolutePos, globalgamemanagersTable->getReader(), resourceManagerAssetFile);*/
			std::ifstream ifsGlobalgamemanagers(gameFolderPath+globalgamemanagersName, std::ios::binary | std::ios::ate);
			ifsGlobalgamemanagers.seekg(assetFileInfoEx.absolutePos, std::ios::beg);

			std::vector<char> resourceManagerBuffer(assetFileInfoEx.curFileSize);
			if (ifsGlobalgamemanagers.read(resourceManagerBuffer.data(), assetFileInfoEx.curFileSize))
			{
				/* worked! */
			}
			int* resourceManagerFilePos = new int(0);
			resourceManagerFile->Read(
				(void*)resourceManagerBuffer.data(),
				assetFileInfoEx.curFileSize,
				resourceManagerFilePos,
				globalgamemanagersFile->header.format,
				globalgamemanagersFile->header.endianness ? true : false);
			AssetSearchCount--;
		}
		else if (classId == MonoManagerClassId) {
			AssetTypeTemplateField* baseAssetTypeTemplateField = new AssetTypeTemplateField;
			baseAssetTypeTemplateField->FromClassDatabase(classDatabaseFile, &classDatabaseFile->classes[findByClassID[classId]], (DWORD)0, false);
			AssetTypeInstance baseAssetTypeInstance(
				(DWORD)1, 
				&baseAssetTypeTemplateField, 
				assetFileInfoEx.curFileSize, 
				globalgamemanagersTable->getReader(), 
				globalgamemanagersFile->header.endianness ? true : false, 
				assetFileInfoEx.absolutePos);
			AssetTypeValueField* baseAssetTypeValueField = baseAssetTypeInstance.GetBaseField();
			if (baseAssetTypeValueField) {
				AssetTypeValueField* m_AssemblyNamesArrayATVF =
					baseAssetTypeValueField->Get("m_AssemblyNames")->Get("Array");
				if (m_AssemblyNamesArrayATVF) {
					AssetTypeValueField** m_AssemblyNamesChildrenListATVF = m_AssemblyNamesArrayATVF->GetChildrenList();
					for (DWORD i = 0; i < m_AssemblyNamesArrayATVF->GetChildrenCount(); i++) {
						AssemblyNames.push_back(m_AssemblyNamesChildrenListATVF[i]->GetValue()->AsString());
					}
				}
			}
			AssetSearchCount--;
		}
	}
	string TypeTreeGeneratorParams;
	string gameFolderPath_a;
	gameFolderPath_a.assign(gameFolderPath.begin(), gameFolderPath.end());
	for (vector<string>::iterator iterator = AssemblyNames.begin(); iterator != AssemblyNames.end(); iterator++) {
		if (!(iterator->empty())) {
			TypeTreeGeneratorParams += "-f \"" + gameFolderPath_a + "Managed\\" + *iterator + "\" ";
		}
	}
	TypeTreeGeneratorParams += " 2>&1 > baseList.txt";
	int TypeTreeGeneratorResult = system((".\\Resource\\TypeTreeGenerator.exe " + TypeTreeGeneratorParams).c_str());
	// behaviourdb.dat
	wstring classDatabaseFileName = L"behaviourdb.dat";
	IAssetsReader* classDatabaseReader = Create_AssetsReaderFromFile((classDatabaseFileName).c_str(), true, RWOpenFlags_None);
	monoDatabaseFile = new ClassDatabaseFile();
	monoDatabaseFile->Read(classDatabaseReader);
	for (size_t i = 0; i < monoDatabaseFile->classes.size(); i++)
	{
		string monoClassDatabaseTypeName = string(monoDatabaseFile->classes[i].name.GetString(monoDatabaseFile));
		findMonoByName.insert(map<string, unsigned int>::value_type(monoClassDatabaseTypeName, (unsigned int)i));
	}
	return true;
}

bool getMonoScriptMap(AssetsFileTable* globalgamemanagersAssetsTable) {
	AssetsFile* globalgamemanagersAssetsFile = globalgamemanagersAssetsTable->getAssetsFile();
	int MonoScriptClassId;
	// findBy로 통합할것
	for (vector<ClassDatabaseType>::iterator iterator = classDatabaseFile->classes.begin();
		(iterator != classDatabaseFile->classes.end()); iterator++) {
		const char* classDatabaseTypeName = iterator->name.GetString(classDatabaseFile);
		if (strcmp(classDatabaseTypeName, "MonoScript") == 0) {
			MonoScriptClassId = iterator->classId;
			break;;
		}
	}
	for (unsigned int i = 0; i < globalgamemanagersAssetsTable->assetFileInfoCount; i++) {
		int classId;
		AssetFileInfoEx assetFileInfoEx = globalgamemanagersAssetsTable->pAssetFileInfo[i];
		if (globalgamemanagersAssetsFile->header.format < 0x10) {
			classId = assetFileInfoEx.curFileType;
		}
		else {
			classId = globalgamemanagersAssetsFile->typeTree.pTypes_Unity5[assetFileInfoEx.curFileTypeOrIndex].classId;
		}
		if (classId == MonoScriptClassId) {
			AssetTypeTemplateField* baseAssetTypeTemplateField = new AssetTypeTemplateField;
			baseAssetTypeTemplateField->FromClassDatabase(classDatabaseFile, &classDatabaseFile->classes[findByClassID[classId]], (DWORD)0, false);
			AssetTypeInstance baseAssetTypeInstance(
				(DWORD)1,
				&baseAssetTypeTemplateField,
				assetFileInfoEx.curFileSize,
				globalgamemanagersAssetsTable->getReader(),
				globalgamemanagersAssetsFile->header.endianness ? true : false,
				assetFileInfoEx.absolutePos);
			AssetTypeValueField* baseAssetTypeValueField = baseAssetTypeInstance.GetBaseField();
			if (baseAssetTypeValueField) {
				AssetTypeValueField* m_ClassNameATVF = baseAssetTypeValueField->Get("m_ClassName");
				AssetTypeValueField* m_NamespaceATVF = baseAssetTypeValueField->Get("m_Namespace");
				if (m_ClassNameATVF && m_NamespaceATVF) {
					string monoScriptFullName = string(m_NamespaceATVF->GetValue()->AsString()) + "." + m_ClassNameATVF->GetValue()->AsString();
					findMonoScriptFullNameByPathID.insert(pair<unsigned __int64, string>(assetFileInfoEx.index, monoScriptFullName));
				}
			}
		}
	}
	return true;
}

/* gameFolderPath should end by \ */
bool LoadAssetsFile(wstring gameFolderPath, wstring assetsFileName) {
	if (gameFolderPath.back() != '\\') {
		gameFolderPath += '\\';
	}
	map<wstring, AssetsFile*>::iterator iterator = mapAssetsFiles.find(assetsFileName);
	if (iterator == mapAssetsFiles.end()) {
		IAssetsReader* assetsReader = Create_AssetsReaderFromFile((gameFolderPath + assetsFileName).c_str(), true, RWOpenFlags_None);
		AssetsFile* assetsFile = new AssetsFile(assetsReader);
		AssetsFileTable* assetsFileTable = new AssetsFileTable(assetsFile);
		assetsFileTable->GenerateQuickLookupTree();
		mapAssetsFiles.insert(pair<wstring, AssetsFile*>(assetsFileName, assetsFile));
		mapAssetsFileTables.insert(pair<wstring, AssetsFileTable*>(assetsFileName, assetsFileTable));
		vectorAssetsFileNames.push_back(assetsFileName);
		if (assetsFileName == L"globalgamemanagers") {
			globalgamemanagersAssetsTable = assetsFileTable;
			getResourceManagerAndAssembly(assetsFileTable, gameFolderPath, assetsFileName);
		}
		else if (assetsFileName == L"globalgamemanagers.assets") {
			getMonoScriptMap(assetsFileTable);
		}
		DWORD dependencyCount = assetsFile->dependencies.dependencyCount;
		if (dependencyCount > 0) {
			for (DWORD i = 0; i < dependencyCount; i++) {
				static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
				wstring newAssetsFileName = converter.from_bytes(assetsFile->dependencies.pDependencies[i].assetPath);
				LoadAssetsFile(gameFolderPath, newAssetsFileName);
			}
			return true;
		}
		else {
			return true;
		}
	}
	else {
		return true;
	}
}

std::string ReplaceAll(std::string &str, const std::string& from, const std::string& to) {
	size_t start_pos = 0; //string처음부터 검사
	while ((start_pos = str.find(from, start_pos)) != std::string::npos)  //from을 찾을 수 없을 때까지
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // 중복검사를 피하고 from.length() > to.length()인 경우를 위해서
	}
	return str;
}

string GetJsonFromAssetTypeValueFieldRecursive(AssetTypeValueField *field) {
	AssetTypeTemplateField* templateField = field->GetTemplateField();
	AssetTypeValueField** fieldChildren = field->GetChildrenList();
	DWORD childrenCount = field->GetChildrenCount();
	string str;
	if (templateField->isArray) {
		if (childrenCount == 0) {
			str = "[";
		}
		else {
			str = "[\r\n";
		}
	}
	else {
		str = "{\r\n";
	}
	
	for (DWORD i = 0; i < childrenCount; i++) {
		AssetTypeValueField* fieldChild = fieldChildren[i];
		AssetTypeTemplateField* templateFieldChild = fieldChild->GetTemplateField();
		string align;
		if (templateFieldChild->align || templateFieldChild->valueType == EnumValueTypes::ValueType_String) {
			align = "1";
		}
		else {
			align = "0";
		}
		string key = align + " " + string(templateFieldChild->type) + " " + string(templateFieldChild->name);
		string value;
		switch (templateFieldChild->valueType) {
		case EnumValueTypes::ValueType_None:
			if (templateFieldChild->isArray) {
				value = GetJsonFromAssetTypeValueFieldRecursive(fieldChild);
			} else {
				value = "\r\n" + GetJsonFromAssetTypeValueFieldRecursive(fieldChild);
			}
			break;
		case EnumValueTypes::ValueType_Int8:
		case EnumValueTypes::ValueType_Int16:
		case EnumValueTypes::ValueType_Int32:
		case EnumValueTypes::ValueType_Int64:
			value = to_string((long long)fieldChild->GetValue()->AsInt());
			break;
		case EnumValueTypes::ValueType_UInt8:
		case EnumValueTypes::ValueType_UInt16:
		case EnumValueTypes::ValueType_UInt32:
		case EnumValueTypes::ValueType_UInt64:
			value = to_string(fieldChild->GetValue()->AsUInt64());
			break;
		case EnumValueTypes::ValueType_Float:
			value = to_string((long double)fieldChild->GetValue()->AsFloat());
			break;
		case EnumValueTypes::ValueType_Double:
			value = to_string((long double)fieldChild->GetValue()->AsDouble());
			break;
		case EnumValueTypes::ValueType_Bool:
			if (fieldChild->GetValue()->AsBool()) {
				value = "true";
			}
			else {
				value = "false";
			}
			break;
		case EnumValueTypes::ValueType_String:
			value = "\"" + string(fieldChild->GetValue()->AsString()) + "\"";
			break;
		}
		if (templateField->isArray) {
			str += "    {\"" + key + "\": ";
			str += ReplaceAll(value, "\r\n", "\r\n    ");
			str += "}";
			if ((i + 1) < childrenCount) {
				str += ",";
				str += "\r\n";
			}
		}
		else {
			str += "    \"" + key + "\": ";
			str += ReplaceAll(value, "\r\n", "\r\n    ");
			if ((i + 1) < childrenCount) {
				str += ",";
				str += "\r\n";
			};
		}
	}
	if (templateField->isArray) {
		if (childrenCount == 0) {
			str += "]";
		}
		else {
			str += "\r\n]";
		}
	}
	else {
		str += "\r\n}";
	}
	return str;
}

string GetJsonFromAssetTypeValueField(AssetTypeValueField *field) {
	string str = "{\r\n";
	AssetTypeTemplateField* templateField = field->GetTemplateField();
	string key = string(templateField->align ? "1" : "0") + " " + string(templateField->type) + " " + string(templateField->name);
	str += "    \"" + key + "\": \r\n    ";
	string value = GetJsonFromAssetTypeValueFieldRecursive(field);
	str += ReplaceAll(value, "\r\n", "\r\n    ");
	str += "\r\n}";
	return str;
}

AssetTypeValueField* GetAssetTypeValueFieldFromJsonRecursive(AssetTypeTemplateField* assetTypeTemplateField, Json::Value json) {
	vector<AssetTypeValueField*>* assetTypeValueFieldArray = new vector<AssetTypeValueField*>();
	AssetTypeValue* assetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_None, 0);
	AssetTypeValueField* assetTypeValueField = new AssetTypeValueField();
	string align;
	if (assetTypeTemplateField->align || assetTypeTemplateField->valueType == EnumValueTypes::ValueType_String) {
		align = "1";
	}
	else {
		align = "0";
	}
	string key = align + " " + string(assetTypeTemplateField->type) + " " + string(assetTypeTemplateField->name);
	//Json::Value thisJson = json[key];
	Json::Value thisJson = json;
	// 이전코드가 잘못되 수정하는도중 임시로 재할당
	vector<string> testStrs1 = thisJson.getMemberNames();
	/*Json::Value thisJson = */
	for (unsigned int i = 0; i < assetTypeTemplateField->childrenCount; i++) {
		AssetTypeTemplateField* childAssetTypeTemplateField = &assetTypeTemplateField->children[i];
		string alignChild;
		if (childAssetTypeTemplateField->align || childAssetTypeTemplateField->valueType == EnumValueTypes::ValueType_String) {
			alignChild = "1";
		}
		else {
			alignChild = "0";
		}
		string keyChild = alignChild + " " + string(childAssetTypeTemplateField->type) + " " + string(childAssetTypeTemplateField->name);
		//void* container;
		AssetTypeValue* childAssetTypeValue;
		AssetTypeValueField* childAssetTypeValueField = new AssetTypeValueField();
		AssetTypeByteArray* assetTypeByteArray;
		string* tempStr;

		//only test
		INT32 testInt = 0;
		switch (childAssetTypeTemplateField->valueType) {
		case EnumValueTypes::ValueType_Int8:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_Int8, new INT8((INT8)thisJson[keyChild].asInt()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;
		case EnumValueTypes::ValueType_Int16:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_Int16, new INT16((INT16)thisJson[keyChild].asInt()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;
		case EnumValueTypes::ValueType_Int32:
			testInt = thisJson[keyChild].asInt();
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_Int32, new INT32((INT32)thisJson[keyChild].asInt()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;
		case EnumValueTypes::ValueType_Int64:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_Int64, new INT64((INT64)thisJson[keyChild].asInt64()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;

		case EnumValueTypes::ValueType_UInt8:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_UInt8, new UINT8((UINT8)thisJson[keyChild].asUInt()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;
		case EnumValueTypes::ValueType_UInt16:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_UInt16, new UINT16((UINT16)thisJson[keyChild].asUInt()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;
		case EnumValueTypes::ValueType_UInt32:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_UInt32, new UINT32((UINT32)thisJson[keyChild].asUInt()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;
		case EnumValueTypes::ValueType_UInt64:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_UInt64, new UINT64((UINT64)thisJson[keyChild].asUInt64()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;

		case EnumValueTypes::ValueType_Float:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_Float, new FLOAT((FLOAT)thisJson[keyChild].asFloat()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;

		case EnumValueTypes::ValueType_Double:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_Double, new DOUBLE((DOUBLE)thisJson[keyChild].asFloat()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;

		case EnumValueTypes::ValueType_Bool:
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_Bool, new BOOL((BOOL)thisJson[keyChild].asBool()));
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;

		case EnumValueTypes::ValueType_String:
			tempStr = new string(thisJson[keyChild].asString());
			childAssetTypeValue = new AssetTypeValue(EnumValueTypes::ValueType_String, (void*)tempStr->c_str());
			childAssetTypeValueField->Read(childAssetTypeValue, childAssetTypeTemplateField, 0, 0);
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;

		case EnumValueTypes::ValueType_None:
			if (childAssetTypeTemplateField->isArray) {
				childAssetTypeValueField = GetAssetTypeValueFieldArrayFromJson(childAssetTypeTemplateField, thisJson[keyChild]);
			}
			else {
				childAssetTypeValueField = GetAssetTypeValueFieldFromJsonRecursive(childAssetTypeTemplateField, thisJson[keyChild]);
			}
			assetTypeValueFieldArray->push_back(childAssetTypeValueField);
			break;
		case EnumValueTypes::ValueType_Array:
			throw new exception("No implement");
			break;
		}
	}
	assetTypeValueField->Read(assetTypeValue, assetTypeTemplateField, assetTypeValueFieldArray->size(), assetTypeValueFieldArray->data());
	return assetTypeValueField;
}

AssetTypeValueField* GetAssetTypeValueFieldFromJson(AssetTypeTemplateField* assetTypeTemplateField, Json::Value json) {
	return GetAssetTypeValueFieldFromJsonRecursive(assetTypeTemplateField, json[json.getMemberNames()[0]]);
}

AssetTypeValueField* GetAssetTypeValueFieldArrayFromJson(AssetTypeTemplateField* assetTypeTemplateField, Json::Value json) {
	Json::StyledWriter writer;
	string testStr = writer.write(json);
	vector<AssetTypeValueField*>* assetTypeValueFieldArray = new vector<AssetTypeValueField*>();
	for (Json::ArrayIndex i = 0; i < json.size(); i++) {
		Json::Value childJson = json[i];
		string key = childJson.getMemberNames()[0];
		assetTypeValueFieldArray->push_back(GetAssetTypeValueFieldFromJsonRecursive(&assetTypeTemplateField->children[1], childJson[key]));
	}
	
	AssetTypeArray* assetTypeArray = new AssetTypeArray();
	assetTypeArray->size = assetTypeValueFieldArray->size();
	AssetTypeValueField* assetTypeValueField = new AssetTypeValueField();
	assetTypeValueField->Read(new AssetTypeValue(EnumValueTypes::ValueType_Array, assetTypeArray), assetTypeTemplateField, assetTypeValueFieldArray->size(), assetTypeValueFieldArray->data());

	return assetTypeValueField;

}



void test2() {
	/*AssetsFileTable* shared0AssetsFileTable = mapAssetsFileTables.find(L"sharedassets0.assets")->second;
	AssetsFile* shared0AssetsFile = shared0AssetsFileTable->getAssetsFile();

	AssetTypeValueField* TMPGlyphATVF = assetTypeValueField->Get("m_glyphInfoList");
	TMPGlyphATVF->Get("id")->GetValue();
	AssetTypeTemplateField* TMPGlyphTemplate = TMPGlyphATVF->GetTemplateField();
	Json::Value testJson;
	Json::Value testJson2;
	testJson2["0 int id"] = 32;
	testJson2["0 float x"] = 6;
	testJson2["0 float y"] = 1029;
	testJson2["0 float width"] = 26;
	testJson2["0 float height"] = 136.1875;
	testJson2["0 float xOffset"] = 0;
	testJson2["0 float yOffset"] = 106.875;
	testJson2["0 float xAdvance"] = 26;
	testJson2["0 float scale"] = 1;
	testJson["0 TMPro.TMP_Glyph data"] = testJson2;
	GetAssetTypeValueFieldFromJsonRecursive(TMPGlyphTemplate, testJson);*/
}


AssetTypeTemplateField* GetMonoAssetTypeTemplateFieldFromClassName(string MonoClassName) {
	int indexOfMonoclass = findMonoByName.find(MonoClassName)->second;

	AssetTypeTemplateField* baseAssetTypeTemplateField = new AssetTypeTemplateField;
	baseAssetTypeTemplateField->FromClassDatabase(classDatabaseFile, &classDatabaseFile->classes[findByClassID[0x72]], (DWORD)0, false);
	AssetTypeTemplateField* baseMonoTypeTemplateField = new AssetTypeTemplateField;
	baseMonoTypeTemplateField->FromClassDatabase(monoDatabaseFile, &monoDatabaseFile->classes[indexOfMonoclass], (DWORD)0, true);
	int prevBaseAssetTypeTemplateFieldChildrenCount = baseAssetTypeTemplateField->childrenCount;
	int prevBaseMonoTypeTemplateFieldChildrenCount = baseMonoTypeTemplateField->childrenCount;
	baseAssetTypeTemplateField->AddChildren(prevBaseMonoTypeTemplateFieldChildrenCount);
	for (int i = 0; i < prevBaseMonoTypeTemplateFieldChildrenCount; i++) {
		baseAssetTypeTemplateField->children[prevBaseAssetTypeTemplateFieldChildrenCount + i] =
			baseMonoTypeTemplateField->children[i];
	}
	return baseAssetTypeTemplateField;
}

string GetClassNameFromBaseAssetTypeValueField(AssetTypeValueField* baseAssetTypeValueField) {
	if (baseAssetTypeValueField) {
		string m_Name = baseAssetTypeValueField->Get("m_Name")->GetValue()->AsString();
		AssetTypeValueField* m_ScriptATVF = baseAssetTypeValueField->Get("m_Script");
		if (m_ScriptATVF) {
			int m_FileId = m_ScriptATVF->Get("m_FileID")->GetValue()->AsInt();
			unsigned __int64 m_PathID = m_ScriptATVF->Get("m_PathID")->GetValue()->AsUInt64();
			return findMonoScriptFullNameByPathID.find(m_PathID)->second;
		}
		else {
			throw new exception("GetClassNameFromBaseAssetTypeValueField: m_ScriptATVF not exist");
		}
	}
	else {
		throw new exception("GetClassNameFromBaseAssetTypeValueField: baseAssetTypeValueField not exist");
	}
}

void GetClassIdFromAssetFileInfoEx(AssetsFileTable* assetsFileTable, AssetFileInfoEx* assetFileInfoEx, int& classId, WORD& monoClassId) {
	if (assetsFileTable->getAssetsFile()->header.format <= 0x10) {
		classId = assetFileInfoEx->curFileType;
	}
	else {
		classId = assetsFileTable->getAssetsFile()->typeTree.pTypes_Unity5[assetFileInfoEx->curFileTypeOrIndex].classId;
		if (classId == 0x72) {
			monoClassId = (WORD)(0xFFFFFFFF - assetFileInfoEx->curFileType); // same as monoScriptIndex in AssetsReplacer
		}
	}
}

AssetTypeInstance* GetBasicAssetTypeInstanceFromAssetFileInfoEx(AssetsFileTable* assetsFileTable, AssetFileInfoEx* assetFileInfoEx) {
	int classId;
	WORD monoClassId;
	GetClassIdFromAssetFileInfoEx(assetsFileTable, assetFileInfoEx, classId, monoClassId);
	AssetTypeTemplateField* baseAssetTypeTemplateField = new AssetTypeTemplateField;
	baseAssetTypeTemplateField->FromClassDatabase(classDatabaseFile, &classDatabaseFile->classes[findByClassID[classId]], (DWORD)0, false);
	AssetTypeInstance* baseAssetTypeInstance = new AssetTypeInstance(
		(DWORD)1,
		&baseAssetTypeTemplateField,
		assetFileInfoEx->curFileSize,
		assetsFileTable->getReader(),
		assetsFileTable->getAssetsFile()->header.endianness ? true : false,
		assetFileInfoEx->absolutePos);
	return baseAssetTypeInstance;
}

AssetTypeInstance* GetDetailAssetTypeInstanceFromAssetFileInfoEx(AssetsFileTable* assetsFileTable, AssetFileInfoEx* assetFileInfoEx) {
	int classId;
	WORD monoClassId;
	GetClassIdFromAssetFileInfoEx(assetsFileTable, assetFileInfoEx, classId, monoClassId);
	AssetTypeInstance* baseAssetTypeInstance = GetBasicAssetTypeInstanceFromAssetFileInfoEx(assetsFileTable, assetFileInfoEx);
	if (classId == 0x72) {
		AssetTypeValueField* baseAssetTypeValueField = baseAssetTypeInstance->GetBaseField();
		string monoScriptFullName = GetClassNameFromBaseAssetTypeValueField(baseAssetTypeValueField);
		AssetTypeTemplateField* baseMonoTypeTemplateField = GetMonoAssetTypeTemplateFieldFromClassName(monoScriptFullName);
		AssetTypeInstance* baseMonoTypeInstance = new AssetTypeInstance(
			(DWORD)1,
			&baseMonoTypeTemplateField,
			assetFileInfoEx->curFileSize,
			assetsFileTable->getReader(),
			assetsFileTable->getAssetsFile()->header.endianness ? true : false,
			assetFileInfoEx->absolutePos);
		return baseMonoTypeInstance;
	}
	else {
		return baseAssetTypeInstance;
	}
}

string GetKeyFromAssetTypeTemplateField(AssetTypeTemplateField* assetTypeTemplateField) {
	string align;
	if (assetTypeTemplateField->align || assetTypeTemplateField->valueType == EnumValueTypes::ValueType_String) {
		align = "1";
	}
	else {
		align = "0";
	}
	string key = align + " " + string(assetTypeTemplateField->type) + " " + string(assetTypeTemplateField->name);
	return key;
}

string GetKeyFromAssetTypeValueField(AssetTypeValueField* assetTypeValueField) {
	return GetKeyFromAssetTypeTemplateField(assetTypeValueField->GetTemplateField());
}

bool ModifyAssetTypeValueFieldFromJSONRecursive(AssetTypeValueField* assetTypeValueField, Json::Value json) {
	string key = GetKeyFromAssetTypeValueField(assetTypeValueField);
	vector<string> jsonKeyList = json.getMemberNames();
	for (unsigned int i = 0; i < assetTypeValueField->GetChildrenCount(); i++) {
		AssetTypeValueField* childAssetTypeValueField = assetTypeValueField->GetChildrenList()[i];
		string keyChild = GetKeyFromAssetTypeValueField(childAssetTypeValueField);
		vector<string>::iterator iterator = find(jsonKeyList.begin(), jsonKeyList.end(), keyChild);

		if (iterator != jsonKeyList.end()) {
			switch (childAssetTypeValueField->GetTemplateField()->valueType) {
				case EnumValueTypes::ValueType_Int8:
					childAssetTypeValueField->GetValue()->Set(new INT8((INT8)json[keyChild].asInt()));
					break;
				case EnumValueTypes::ValueType_Int16:
					childAssetTypeValueField->GetValue()->Set(new INT16((INT16)json[keyChild].asInt()));
					break;
				case EnumValueTypes::ValueType_Int32:
					childAssetTypeValueField->GetValue()->Set(new INT32((INT32)json[keyChild].asInt()));
					break;
				case EnumValueTypes::ValueType_Int64:
					childAssetTypeValueField->GetValue()->Set(new INT64((INT64)json[keyChild].asInt64()));
					break;

				case EnumValueTypes::ValueType_UInt8:
					childAssetTypeValueField->GetValue()->Set(new UINT8((UINT8)json[keyChild].asUInt()));
					break;
				case EnumValueTypes::ValueType_UInt16:
					childAssetTypeValueField->GetValue()->Set(new UINT16((UINT16)json[keyChild].asUInt()));
					break;
				case EnumValueTypes::ValueType_UInt32:
					childAssetTypeValueField->GetValue()->Set(new UINT32((UINT32)json[keyChild].asUInt()));
					break;
				case EnumValueTypes::ValueType_UInt64:
					childAssetTypeValueField->GetValue()->Set(new UINT64((UINT64)json[keyChild].asUInt64()));
					break;

				case EnumValueTypes::ValueType_Float:
					childAssetTypeValueField->GetValue()->Set(new FLOAT((FLOAT)json[keyChild].asFloat()));
					break;
				case EnumValueTypes::ValueType_Double:
					childAssetTypeValueField->GetValue()->Set(new DOUBLE((DOUBLE)json[keyChild].asDouble()));
					break;

				case EnumValueTypes::ValueType_Bool:
					childAssetTypeValueField->GetValue()->Set(new BOOL((BOOL)json[keyChild].asBool()));
					break;

				case EnumValueTypes::ValueType_String:
					childAssetTypeValueField->GetValue()->Set(new string(json[keyChild].asString()));
					break;

				case EnumValueTypes::ValueType_None:
					if (childAssetTypeValueField->GetTemplateField()->isArray) {
						//ClearAssetTypeValueField(childAssetTypeValueField); // 해야할지 안해도 될지 모르겠
						assetTypeValueField->GetChildrenList()[i] = GetAssetTypeValueFieldArrayFromJson(childAssetTypeValueField->GetTemplateField(), json[keyChild]);
					}
					else {
						ModifyAssetTypeValueFieldFromJSONRecursive(childAssetTypeValueField, json[keyChild]);
					}
					break;

				case EnumValueTypes::ValueType_Array:
					throw new exception("No implement");
					break;
			}
		}
	}
	return true;
}

bool ModifyAssetTypeValueFieldFromJSON(AssetTypeValueField* assetTypeValueField, Json::Value json) {
	return ModifyAssetTypeValueFieldFromJSONRecursive(assetTypeValueField, json[json.getMemberNames()[0]]);
}

struct AssetLogicalPath {
	string AssetsName;
	string AssetName;
	string ContainerPath;
	bool hasContainerPath;
};

struct AssetLogicalReplacer {
	AssetLogicalPath assetLogicalPath;
	string replaceAssetPath;
	bool UseContainerPath;
};

int GetIndexFromPathId(int PathId) {
	string dependencyAssetsPath = string(globalgamemanagersAssetsTable->getAssetsFile()->dependencies.pDependencies[PathId-1].assetPath);
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	wstring newAssetsFileName = converter.from_bytes(dependencyAssetsPath);
	return distance(vectorAssetsFileNames.begin(), find(vectorAssetsFileNames.begin(), vectorAssetsFileNames.end(), newAssetsFileName));
}

vector<AssetLogicalPath> GetFontAssetListAsNameAndContainerInternal(AssetsFileTable* assetsFileTable, string AssetsName, int AssetsPathId) {
	vector<AssetLogicalPath> assetLogicalPaths;
	string MonoClassNameToFind = "TMPro.TMP_FontAsset";
	AssetsFile* assetsFile = assetsFileTable->getAssetsFile();
	ResourceManager_ContainerData* resourceManagerFileContainerArray = resourceManagerFile->containerArray;

	for (unsigned int i = 0; i < assetsFileTable->assetFileInfoCount; i++)
	{
		AssetFileInfoEx* assetFileInfoEx = &assetsFileTable->pAssetFileInfo[i];
		int classId;
		WORD monoClassId;
		GetClassIdFromAssetFileInfoEx(assetsFileTable, assetFileInfoEx, classId, monoClassId);
		if (classId == 0x72) {
			AssetTypeInstance* baseAssetTypeInstance = GetBasicAssetTypeInstanceFromAssetFileInfoEx(assetsFileTable, assetFileInfoEx);
			AssetTypeValueField* baseAssetTypeValueField = baseAssetTypeInstance->GetBaseField();
			if (baseAssetTypeValueField) {
				string monoClassName = GetClassNameFromBaseAssetTypeValueField(baseAssetTypeValueField);
				if (monoClassName == MonoClassNameToFind) {
					string m_Name = baseAssetTypeValueField->Get("m_Name")->GetValue()->AsString();
					int j = 0;
					for (; j < resourceManagerFile->containerArrayLen; j++) {
						if (AssetsPathId == GetIndexFromPathId(resourceManagerFileContainerArray[j].ids.fileId) &&
							assetFileInfoEx->index == resourceManagerFileContainerArray[j].ids.pathId) {
							AssetLogicalPath assetLogicalPath = { AssetsName, m_Name, resourceManagerFileContainerArray[j].name, true };
							assetLogicalPaths.push_back(assetLogicalPath);
							break;
						}
					}
					if (j == resourceManagerFile->containerArrayLen) {
						AssetLogicalPath assetLogicalPath = { AssetsName, m_Name, "", false };
						assetLogicalPaths.push_back(assetLogicalPath);
					}
				}
			}
		}
	}
	return assetLogicalPaths;
}

vector<AssetLogicalPath> GetFontAssetListAsNameAndContainer() {
	vector<AssetLogicalPath> FontAssetList;
	ptrdiff_t ptrdiffResource = distance(vectorAssetsFileNames.begin(), find(vectorAssetsFileNames.begin(), vectorAssetsFileNames.end(), wstring(L"resources.assets")));
	AssetsFileTable* resourcesAssetsFileTable = mapAssetsFileTables[L"resources.assets"];
	vector<AssetLogicalPath> resourcesFontAssetList = GetFontAssetListAsNameAndContainerInternal(resourcesAssetsFileTable, "resources.assets", ptrdiffResource);

	ptrdiff_t ptrdiffShared0 = distance(vectorAssetsFileNames.begin(), find(vectorAssetsFileNames.begin(), vectorAssetsFileNames.end(), L"sharedassets0.assets"));
	AssetsFileTable* shared0AssetsFileTable = mapAssetsFileTables[L"sharedassets0.assets"];
	vector<AssetLogicalPath> shared0FontAssetList = GetFontAssetListAsNameAndContainerInternal(shared0AssetsFileTable, "sharedassets0.assets", ptrdiffShared0);

	FontAssetList.insert(FontAssetList.end(), resourcesFontAssetList.begin(), resourcesFontAssetList.end());
	FontAssetList.insert(FontAssetList.end(), shared0FontAssetList.begin(), shared0FontAssetList.end());
	return FontAssetList;
}





void test() {
	map<unsigned __int64, string> TMPro_TMP_FontAssetList;
	AssetsFileTable* shared0AssetsFileTable = mapAssetsFileTables.find(L"sharedassets0.assets")->second;
	AssetsFile* shared0AssetsFile = shared0AssetsFileTable->getAssetsFile();
	
	for (unsigned int i = 0; i < shared0AssetsFileTable->assetFileInfoCount; i++) {
		AssetFileInfoEx* assetFileInfoEx = &shared0AssetsFileTable->pAssetFileInfo[i];
		int classId;
		WORD monoClassId; // same as monoScriptIndex in AssetsReplacer
		GetClassIdFromAssetFileInfoEx(shared0AssetsFileTable, assetFileInfoEx, classId, monoClassId);
		if(classId == 0x72) {
			AssetTypeInstance* baseAssetTypeInstance = GetBasicAssetTypeInstanceFromAssetFileInfoEx(shared0AssetsFileTable, assetFileInfoEx);
			AssetTypeValueField* baseAssetTypeValueField = baseAssetTypeInstance->GetBaseField();
			if (baseAssetTypeValueField) {
				string m_Name = baseAssetTypeValueField->Get("m_Name")->GetValue()->AsString();
				string monoClassName = GetClassNameFromBaseAssetTypeValueField(baseAssetTypeValueField);
				if(monoClassName == "TMPro.TMP_FontAsset") {
					TMPro_TMP_FontAssetList.insert(pair<unsigned __int64, string>(assetFileInfoEx->index, m_Name));
				}
			}
		}
	}

	for (
		map<unsigned __int64, string>::iterator iterator = TMPro_TMP_FontAssetList.begin();
		iterator != TMPro_TMP_FontAssetList.end();
		iterator++
		) {
		AssetFileInfoEx* assetFileInfoEx = shared0AssetsFileTable->getAssetInfo(iterator->first);
		AssetTypeInstance* baseMonoTypeInstance = GetDetailAssetTypeInstanceFromAssetFileInfoEx(shared0AssetsFileTable, assetFileInfoEx);
		AssetTypeValueField* baseMonoTypeValueField = baseMonoTypeInstance->GetBaseField();
		if (baseMonoTypeValueField) {
			string m_Name = baseMonoTypeValueField->Get("m_Name")->GetValue()->AsString();
			string testJson = GetJsonFromAssetTypeValueField(baseMonoTypeValueField);
			//test2(baseMonoTypeValueField);
			Json::Reader reader;
			Json::Value testRoot;
			reader.parse(testJson, testRoot);
			AssetTypeValueField* createdFromJson = GetAssetTypeValueFieldFromJson(baseMonoTypeValueField->GetTemplateField(), testRoot);
			string testJson2 = GetJsonFromAssetTypeValueField(createdFromJson);
			if (testJson == testJson2) {
				Json::Value modifiedJson;
				reader.parse(readFile2("Resource\\OpenSans SDF-sharedassets0.assets-10-MonoBehaviour.json"), modifiedJson);
				ModifyAssetTypeValueFieldFromJSON(baseMonoTypeValueField, modifiedJson);
			}
		}
	}
}



//bool MakeAndAddReplacer(vector<AssetLogicalReplacer> assetLogicalReplacers, AssetsFileTable* assetsFileTable, string AssetsName, int AssetsPathId) {
//	//AssetsReplacerForEachAssets[AssetsName]
//	for (vector<AssetLogicalReplacer>::iterator iterator = assetLogicalReplacers.begin();
//		iterator != assetLogicalReplacers.end(); iterator++) {
//
//		
//	}
//}



bool MakeAndAddReplacerForContainer(vector<AssetLogicalReplacer> assetLogicalReplacers) {

	std::wstring_convert<std::codecvt_utf8<wchar_t> , wchar_t> converter;
	ResourceManager_ContainerData* resourceManagerFileContainerArray = resourceManagerFile->containerArray;
	Json::Reader reader;

	for (vector<AssetLogicalReplacer>::iterator iterator = assetLogicalReplacers.begin();
		iterator != assetLogicalReplacers.end(); iterator++) {

		ResourceManager_PPtr resourceManager_PPtr;
		for (unsigned int i = 0; i < resourceManagerFile->containerArrayLen; i++) {
			if (resourceManagerFileContainerArray[i].name == iterator->assetLogicalPath.ContainerPath) {
				resourceManager_PPtr = resourceManagerFileContainerArray[i].ids;
				break;
			}
		}
		if (resourceManager_PPtr.fileId != 0 && resourceManager_PPtr.pathId != 0) {
			int FileIndex = GetIndexFromPathId(resourceManager_PPtr.fileId);
			DWORD PathId = resourceManager_PPtr.pathId;
			wstring assetsFileName = vectorAssetsFileNames[FileIndex];
			string assetsFilenameStr = converter.to_bytes(assetsFileName);
			AssetsFileTable* assetsFileTable = mapAssetsFileTables[assetsFileName];
			AssetFileInfoEx* assetFileInfoEx = assetsFileTable->getAssetInfo(PathId);
			AssetTypeInstance* assetTypeInstance = GetDetailAssetTypeInstanceFromAssetFileInfoEx(assetsFileTable, assetFileInfoEx);
			AssetTypeValueField* baseAssetTypeValueField = assetTypeInstance->GetBaseField();
			int classId;
			WORD monoClassId;
			GetClassIdFromAssetFileInfoEx(assetsFileTable, assetFileInfoEx, classId, monoClassId);

			Json::Value modifiedJson;
			reader.parse(readFile2(iterator->replaceAssetPath), modifiedJson);
			ModifyAssetTypeValueFieldFromJSON(baseAssetTypeValueField, modifiedJson);

			QWORD newByteSize = baseAssetTypeValueField->GetByteSize(0);
			void* newAssetBuffer = malloc((size_t)newByteSize);
			if (newAssetBuffer) {
				IAssetsWriter *pWriter = Create_AssetsWriterToMemory(newAssetBuffer, (size_t)newByteSize);
				if (pWriter) {
					newByteSize = baseAssetTypeValueField->Write(pWriter, 0, assetsFileTable->getAssetsFile()->header.endianness ? true : false);
					AssetsReplacer *pReplacer = MakeAssetModifierFromMemory(0, PathId, classId, monoClassId, newAssetBuffer, newByteSize, free);
					if (pReplacer) {
						AssetsReplacerForEachAssets[assetsFilenameStr].push_back(pReplacer);
					}
				}
			}

		}
	}
	return true;
}

void test3() {
	vector<AssetLogicalPath> TMPFont_AssetLogicalPaths = GetFontAssetListAsNameAndContainer();
	vector<AssetLogicalReplacer> TMPFont_AssetLogicalContainerReplacer;
	AssetLogicalPath AssetLogicalPath1 = { "sharedassets0.assets","Baloo-Regular SDF","fonts & materials/baloo/baloo-regular sdf",true };
	AssetLogicalReplacer AssetLogicalReplacer1 = { AssetLogicalPath1 ,"Resource\\OpenSans SDF Replacer.json",false };
	AssetLogicalPath AssetLogicalPath2 = { "resources.assets","NotoSansCJKsc-Regular SDF","fonts & materials/google noto/simplified chinese/notosanscjksc-regular sdf",true };
	AssetLogicalReplacer AssetLogicalReplacer2 = { AssetLogicalPath2 ,"Resource\\OpenSans SDF Replacer.json",false };
	TMPFont_AssetLogicalContainerReplacer.push_back(AssetLogicalReplacer1);
	TMPFont_AssetLogicalContainerReplacer.push_back(AssetLogicalReplacer2);
	MakeAndAddReplacerForContainer(TMPFont_AssetLogicalContainerReplacer);
}

int main()
{

	wchar_t WcharCurrentDirectory[255] = {};
	_wgetcwd(WcharCurrentDirectory, 255);
	wstring _currentDirectory(WcharCurrentDirectory);
	wstring classDatabaseFileName = L"\\Resource\\U2017.1.0f3";
	IAssetsReader* classDatabaseReader = Create_AssetsReaderFromFile((_currentDirectory + classDatabaseFileName).c_str(), true, RWOpenFlags_None);
	classDatabaseFile = new ClassDatabaseFile();
	classDatabaseFile->Read(classDatabaseReader);
	for (size_t i = 0; i < classDatabaseFile->classes.size(); i++)
	{
		int classid = classDatabaseFile->classes[i].classId;
		findByClassID.insert(map<int, unsigned int>::value_type(classid, (unsigned int)i));
	}

	wstring firstAssetsFileName = L"globalgamemanagers";
	//wstring _gameFolderPath = L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Slime Rancher\\SlimeRancher_Data\\";
	wstring _gameFolderPath = L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Two Point Hospital\\TPH_Data";
	LoadAssetsFile(_gameFolderPath, firstAssetsFileName);
	/*test();
	test2();*/
	test3();

	return 0;
}



// 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
// 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

// 시작을 위한 팁: 
//   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
//   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
//   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
//   4. [오류 목록] 창을 사용하여 오류를 봅니다.
//   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, [프로젝트] > [기존 항목 추가]로 이동하여 기존 코드 파일을 프로젝트에 추가합니다.
//   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고 .sln 파일을 선택합니다.
