// Project1.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include <iostream>
#include <map>
#include <codecvt> // for std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

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



void test() {
	map<unsigned __int64, string> TMPro_TMP_FontAssetList;
	AssetsFileTable* shared0AssetsFileTable = mapAssetsFileTables.find(L"sharedassets0.assets")->second;
	AssetsFile* shared0AssetsFile = shared0AssetsFileTable->getAssetsFile();
	
	for (unsigned int i = 0; i < shared0AssetsFileTable->assetFileInfoCount; i++) {
		AssetFileInfoEx* assetFileInfoEx = &shared0AssetsFileTable->pAssetFileInfo[i];
		int classId;
		WORD monoClassId; // same as monoScriptIndex in AssetsReplacer
		if (shared0AssetsFile->header.format >= 0x10) {
			classId = shared0AssetsFile->typeTree.pTypes_Unity5[assetFileInfoEx->curFileTypeOrIndex].classId;
			if (classId == 0x72) {
				monoClassId = (WORD)(0xFFFFFFFF - assetFileInfoEx->curFileType);
				AssetTypeTemplateField* baseAssetTypeTemplateField = new AssetTypeTemplateField;
				baseAssetTypeTemplateField->FromClassDatabase(classDatabaseFile, &classDatabaseFile->classes[findByClassID[classId]], (DWORD)0, false);
				AssetTypeInstance baseAssetTypeInstance(
					(DWORD)1,
					&baseAssetTypeTemplateField,
					assetFileInfoEx->curFileSize,
					shared0AssetsFileTable->getReader(),
					shared0AssetsFile->header.endianness ? true : false,
					assetFileInfoEx->absolutePos);
				AssetTypeValueField* baseAssetTypeValueField = baseAssetTypeInstance.GetBaseField();
				if (baseAssetTypeValueField) {
					string m_Name = baseAssetTypeValueField->Get("m_Name")->GetValue()->AsString();
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
		int classId;
		WORD monoClassId; // same as monoScriptIndex in AssetsReplacer
		if (shared0AssetsFile->header.format >= 0x10) {
			classId = shared0AssetsFile->typeTree.pTypes_Unity5[assetFileInfoEx->curFileTypeOrIndex].classId;
			if (classId == 0x72) {
				monoClassId = (WORD)(0xFFFFFFFF - assetFileInfoEx->curFileType);
				AssetTypeTemplateField* baseAssetTypeTemplateField = new AssetTypeTemplateField;
				baseAssetTypeTemplateField->FromClassDatabase(classDatabaseFile, &classDatabaseFile->classes[findByClassID[classId]], (DWORD)0, false);
				AssetTypeInstance baseAssetTypeInstance(
					(DWORD)1,
					&baseAssetTypeTemplateField,
					assetFileInfoEx->curFileSize,
					shared0AssetsFileTable->getReader(),
					shared0AssetsFile->header.endianness ? true : false,
					assetFileInfoEx->absolutePos);
				AssetTypeValueField* baseAssetTypeValueField = baseAssetTypeInstance.GetBaseField();
				if (baseAssetTypeValueField) {
					string m_Name = baseAssetTypeValueField->Get("m_Name")->GetValue()->AsString();
					TMPro_TMP_FontAssetList.insert(pair<unsigned __int64, string>(assetFileInfoEx->index, m_Name));

					AssetTypeValueField* m_ScriptATVF = baseAssetTypeValueField->Get("m_Script");
					if (m_ScriptATVF) {
						int m_FileId = m_ScriptATVF->Get("m_FileID")->GetValue()->AsInt();
						unsigned __int64 m_PathID = m_ScriptATVF->Get("m_PathID")->GetValue()->AsUInt64();
						string monoScriptFullName = findMonoScriptFullNameByPathID.find(m_PathID)->second;
						if (monoScriptFullName == "TMPro.TMP_FontAsset") {
							int indexOfMonoclass = findMonoByName.find(findMonoScriptFullNameByPathID.find(m_PathID)->second)->second;
							AssetTypeTemplateField* baseMonoTypeTemplateField = new AssetTypeTemplateField;
							baseMonoTypeTemplateField->FromClassDatabase(monoDatabaseFile, &monoDatabaseFile->classes[indexOfMonoclass], (DWORD)0, true);
							int prevBaseAssetTypeTemplateFieldChildrenCount = baseAssetTypeTemplateField->childrenCount;
							int prevBaseMonoTypeTemplateFieldChildrenCount = baseMonoTypeTemplateField->childrenCount;
							baseAssetTypeTemplateField->AddChildren(baseMonoTypeTemplateField->childrenCount);
							for (int i = 0; i < prevBaseMonoTypeTemplateFieldChildrenCount; i++) {
								baseAssetTypeTemplateField->children[prevBaseAssetTypeTemplateFieldChildrenCount + i] =
									baseMonoTypeTemplateField->children[i];
							}
							AssetTypeInstance baseMonoTypeInstance(
								(DWORD)1,
								&baseAssetTypeTemplateField,
								assetFileInfoEx->curFileSize,
								shared0AssetsFileTable->getReader(),
								shared0AssetsFile->header.endianness ? true : false,
								assetFileInfoEx->absolutePos);
							AssetTypeValueField* baseMonoTypeValueField = baseMonoTypeInstance.GetBaseField();
							if (baseMonoTypeValueField) {
								string m_Name = baseMonoTypeValueField->Get("m_Name")->GetValue()->AsString();
								TMPro_TMP_FontAssetList.insert(pair<unsigned __int64, string>(assetFileInfoEx->index, m_Name));
								GetJsonFromAssetTypeValueField(baseMonoTypeValueField);
							}
						}
					}
				}
			}
		}
	}

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
	wstring _gameFolderPath = L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Slime Rancher\\SlimeRancher_Data\\";
	LoadAssetsFile(_gameFolderPath, firstAssetsFileName);
	test();
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
