#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <vector>
#include <windows.h>
#include <tlhelp32.h>
#include <zlib.h>
#include <cstdint>
#include <thread>
#include <yy981/dll.h>
#include <yy981/andor.h>
#include <yy981/path.h>


namespace fs = std::filesystem;
std::string temp(normalize_path(std::getenv("temp"))), currentPath,
			option, exename, sourceFile, include_qt6, addlibrary, addlibrary_o, original, icon, icon_obj, dllCompileCMD;
std::string mkpath = temp + "/_.mk";
bool enableOriginal_qt(false), enableIcon(false), isCompiling(false), compilerCPPMode(true);
int cmd(const std::string& command) {return std::system(command.c_str());}
std::string compilerM() {if (compilerCPPMode) return "g++"; else return "gcc";}
int taskLevel = 3;
uint32_t hash;
HMODULE dll = LoadLibrary("compiler.dll");
DLL<uint32_t, const std::string&> calculateCRC32(dll, "calculateCRC32");
DLL<bool, const std::string&, DWORD> SetProcessPriority(dll, "SetProcessPriority");

inline void flags() {
	if (enableIcon) {
		// std::cout << "icon_ff: " << icon << "\n";
		std::ofstream(temp+"/icon.rc") << "IDI_ICON1 ICON \"" << currentPath << "/" << icon << "\"";
		cmd("windres.exe -i " + temp+"/icon.rc" + " -o " + temp+"/icon.o");
		icon_obj = temp+"/icon.o";
		// std::cout << "icon_fb: " << icon << "\n";
	}
}

void readOldYBP() {
    std::string line;
	std::ifstream file(sourceFile+".ybp");
    int loop = 0;

    // ファイルから1行ずつ読み込む
    while (std::getline(file, line)) {
        loop++;
        switch (loop) {
            case 1: if (line=="null") exename = sourceFile; else exename = line; break;
            case 2: if (line=="null") addlibrary = ""; else addlibrary = line; break;
            case 3: if (line=="null") option = ""; else option = line; break;
            case 4: if (line=="null") original = ""; else original = line; break;
            default:
                std::cout << "ybp読み取りエラー: 必要な行数以上の行が存在しますが無視します\n";
                return;  // 4行読み込んだら終了
        }
    }

    if (loop < 4) {
        std::cerr << "ybp読み取りエラー: 行数不足 " << loop << "行しかありません\n";
        exit(4);
    }
}

void upgradeYBP() {
	readOldYBP();
	std::string original_uy;
	if (original.contains("qt")) original_uy += "/qt ";
	if (original.contains("nocp932")) original_uy += "/nocp932 ";
	std::ofstream(sourceFile + ".ybp2") << (sourceFile.empty()? "null" : sourceFile) << "\n" 
										<< (exename.empty()? "null" : exename) << "\n" 
										<< (addlibrary.empty()? "null" : addlibrary) << "\n" 
										<< (option.empty()? "null" : option) << "\n" 
										<< (original.empty()? "null" : original_uy);
}

void task() {
    DWORD priorityClass;
    switch (taskLevel) {
        case 1: priorityClass = HIGH_PRIORITY_CLASS; break;
        case 2: priorityClass = ABOVE_NORMAL_PRIORITY_CLASS; break;
        case 3: return;
        case 4: priorityClass = BELOW_NORMAL_PRIORITY_CLASS; break;
        case 5: priorityClass = IDLE_PRIORITY_CLASS; break;
    }

    // プロセスの優先度を変更
	std::thread([priorityClass]{
		bool continueLoop = true;
		while (continueLoop&&isCompiling) if (SetProcessPriority("cc1plus.exe", priorityClass)) {continueLoop = false;std::cout << "y\n";}
	}).detach();
}

void userInput() {
	std::cout << "独自オプション: ";
	std::getline(std::cin,original);
	if (original == "skip") {
		original.clear();
		exename.clear();
		addlibrary.clear();
		option.clear();
		return;
	}
	std::cout << "実行ファイル名(拡張子除く): ";
	std::getline(std::cin,exename);
	std::cout << "追加ライブラリ(各ファイル名の前に-lを追加): ";
	std::getline(std::cin,addlibrary);
	std::cout << "オプション: ";
	std::getline(std::cin,option);
}

void readYBP(char* argv[]) {
    std::string line;
	std::ifstream file(argv[1]);
    int loop = 0;

    // ファイルから1行ずつ読み込む
    while (std::getline(file, line)) {
        loop++;
        
        // std::cout << "読み込んだ行[" << loop << "]: " << line << '\n';  

        switch (loop) {
            case 1: if (line=="null") {std::cerr << "必須要素のSourceFileがnullです";exit(3);} else sourceFile = line; break;
            case 2: if (line=="null") exename = sourceFile; else exename = line; break;
            case 3: if (line=="null") addlibrary = ""; else addlibrary = line; break;
            case 4: if (line=="null") option = ""; else option = line; break;
            case 5: if (line=="null") original = ""; else original = line; break;
            default:
                std::cout << "ybp読み取りエラー: 必要な行数以上の行が存在しますが無視します\n";
                return;  // 5行読み込んだら終了
        }
    }

    if (loop < 5) {
        std::cerr << "ybp読み取りエラー: 行数不足 " << loop << "行しかありません\n";
        exit(3);
    }
}

void createYBP() {
	std::cout << "[ybp生成モード]\nソースコードファイル名(拡張子除く): ";
	std::getline(std::cin,sourceFile);
	userInput();
	std::ofstream(sourceFile + ".ybp2") << (sourceFile.empty()? "null" : sourceFile) << "\n" 
										<< (exename.empty()? "null" : exename) << "\n" 
										<< (addlibrary.empty()? "null" : addlibrary) << "\n" 
										<< (option.empty()? "null" : option) << "\n" 
										<< (original.empty()? "null" : original);
}

void writeMK() {
	std::string stdcpp23, cp932;
	if (exename.empty()) exename=sourceFile;
	// 入力処理
	if (!original.contains("/nocp932")) cp932 = "-fexec-charset=cp932";
	if (original.contains("/qt")) {
		addlibrary_o = addlibrary + " -lQt6Core -lQt6Widgets -lQt6Gui";
		include_qt6 = "-Ic:/msys64/mingw64/include/qt6";
		enableOriginal_qt = true;
	} else addlibrary_o = addlibrary;
	if (!original.contains("/nocpp23")) stdcpp23 = "-std=c++23";
	if (original.contains("/icon")) {
		if (original.substr(5,1) != "=") std::cerr << "使い方が違います icon=<ファイルパス>";
		icon = original.substr(original.find("icon")+5);
		// std::cout << "icon_ori: " << icon << "\n";
	}
	if (original.contains("/nog++")) compilerCPPMode = false; else compilerCPPMode = true;
	if (original.contains("/dll")) {
		dllCompileCMD =
			compilerM() + " -shared -o " + exename + " " + sourceFile + " " + 
			cp932 + " " + stdcpp23 + " " + option + " -Ic:/msys64/mingw64/include " + include_qt6 + " -Lc:/msys64/mingw64/lib " + addlibrary_o;
		return;
	}

	// mk出力
	std::ofstream(mkpath)
		<< "CC  = " << compilerM()
		<< "\nCFLAGS  = " << cp932 << " " << stdcpp23 << " " << option
		<< "\nTARGET  = " << exename << ".exe"
		<< "\nSRCS    = " << sourceFile << ".cpp"
		<< "\nOBJS    = " << sourceFile << ".o" << (enableIcon? " " + icon_obj : "")
		<< "\nINCDIR  = -Ic:/msys64/mingw64/include " << include_qt6
		<< "\nLIBDIR  = -Lc:/msys64/mingw64/lib"
		<< "\nLIBS    = " << addlibrary_o
		<< "\n$(TARGET): $(OBJS)"
		<< "\n\t$(CC) -o $@ $^ $(LIBDIR) $(LIBS)"
		<< "\n%.o: %.cpp"
		<< "\n\t$(CC) $(CFLAGS) $(INCDIR) -c $< -o $@";
		hash = calculateCRC32(mkpath);
}

inline void core() {
	task();
	flags();
	/*if (calculateCRC32(mkpath) != hash) 未来の自分へ 効率化頼んだ*/writeMK();
	int returnCode = cmd((dllCompileCMD.empty()? "c:/GnuWin32/bin/make.exe -B -f " + mkpath : dllCompileCMD)); // 核
	if (returnCode==0) std::cout << "成功\n"; else std::cout << "エラー[" << returnCode << "]\n\n";
}

inline void compile() {
	core();
	std::string input;
	while(true) {
		std::cout << "> ";
		std::getline(std::cin,input); //指示
		if (input == "help")
			std::cout << "moc:	.mocファイルを生成\n 空白を入れてその後にファイル名を入力も可"
					  << "write:	.mkファイルを生成\n"
					  << "exit:	終了\n"
					  << "set:	変数を変更(詳細はsetと入力してください)\n"
					  << "		例:   set sou test\n"
					  << "task:	コンパイル処理の優先度\n"
					  << "flag:	フラグを設定\n"
					  << "cmd:	入力されたコンソールコマンドを実行\n"
					  << "run:	コンパイル済みアプリを実行";
		else if (input.starts_with("moc")) {
			if (input == "moc") cmd("c:/msys64/mingw64/share/qt6/bin/moc.exe " + sourceFile + ".cpp -o _.moc");
			else if (input.size() == 4) std::cerr << "第二層入力エラー";
			else {
				std::string i_moc = fs::path(input.substr(4)).stem().string();
				std::cout << "c:/msys64/mingw64/share/qt6/bin/moc.exe " + input.substr(4) + " -o " + i_moc + ".moc\n";
				cmd("c:/msys64/mingw64/share/qt6/bin/moc.exe " + input.substr(4) + " -o" + i_moc + ".moc");
			}
		}
		else if (input == "write") writeMK();
		else if (input.starts_with("icon")) {
			if (input.size()==4) {
				std::cout << "Icon: 現在" << (enableIcon? "有効" : "無効") << "です フラグの変更を行いたい場合は、後ろにONやOFFをつけたコマンドを使用してください"
						  << "\nIcon = " << icon;
				writeMK();
			} else {
				input.erase(0,5);
				if (input=="ON"||input=="on") enableIcon = true;
				else if (input=="OFF"||input=="off") enableIcon = false;
				else std::cerr << "第二層入力エラー";
			}
		}
		else if (input.starts_with("task")) {
			if (input == "task") input += " help";
			if (input == "task ") input += "help";
			input.erase(0,5);
			if (input.substr(0,4)=="help") std::cout << "例: task [1:高 2:通常以上 3:通常 4:通常以下 5:低]\n現在: " << taskLevel;
			else taskLevel = std::stoi(input.substr(0,1));
		}
		else if (input.starts_with("cmd") && input.size()>=5) cmd(input.substr(4)); // 空白分を考慮して4(3ではない) で、cmdの内容がなければ意味ないから5
		else if (input == "run") cmd("call " + sourceFile + ".exe");
		else if (input.size() >= 3 && input.starts_with("set")) { // set.begin
			if (input == "set") input += " help";
			if (input == "set ") input += "help";
			input.erase(0,4);
			char windowTitle[256];
			GetConsoleTitle(windowTitle,256);
			if (input.substr(0,4)=="help") {
				std::cout << "        sou: ソースファイル名 (現在: " << sourceFile << " )\n"
						  << "        exe: 実行ファイル名 (現在: " << exename << " )\n"
						  << "        lib: 追加ライブラリ (現在: I:" << addlibrary << " O: " << addlibrary_o << " )\n"
						  << "        opt: オプション (現在: " << option << " )\n"
						  << "        ori: 独自オプション (現在: " << original << " )\n"
						  << "        tit: ウィンドウタイトル (現在: " << windowTitle << " )\n"
						  << "     例: set sou test";
			} else if (input.substr(3,1)==" ") {
				if (input.substr(0,3)=="sou") sourceFile = input.substr(4);
				else if (input.substr(0,3)=="exe") exename = input.substr(4);
				else if (input.substr(0,3)=="opt") option = input.substr(4);
				else if (input.substr(0,3)=="lib") addlibrary = input.substr(4);
				else if (input.substr(0,3)=="ori") original = input.substr(4);
				else if (input.substr(0,3)=="tit") SetConsoleTitle(input.substr(4).c_str());
				else std::cerr << "第三層入力エラー";
				writeMK();
			} else std::cerr << "第二層入力エラー";
		} // set.end
		else if (input == "exit") break;
		else if (input == "") core();
		else std::cerr << "有効なコマンドではありません help と打ってコマンドを確認してください";
		std::cout << "\n";
	}
}

inline void end(){if (fs::exists(sourceFile+".o")) fs::remove(sourceFile+".o");}


int main(int argc, char *argv[]) {
/*	std::thread([]{
		while(true) {
			std::this_thread::sleep_for(std::chrono::seconds(10));
			std::cout << std::boolalpha << enableIcon << "\n";
		}
	}).detach();
*/	sourceFile = exename = addlibrary = option = original = "null";
	currentPath = normalize_path(fs::current_path().string());
	dll = LoadLibrary("compiler.dll");
	
	if (argc == 1) {
		SetConsoleTitle("Compiler-YBP");
		createYBP();
		return 0;
	}
	// set paths
	sourceFile = fs::path(argv[1]).stem().string();
	SetConsoleTitle(std::string("[" + sourceFile + "] MSYS2-Mingw64_New-Compiler").c_str());

	bool inputMK = false;
	if (argc > 2) {std::cerr << "コマンドライン引数エラー\n";return 1;}
	std::string inputExtension = fs::path(argv[1]).extension().string();
	if (inputExtension == ".ybp") {
		upgradeYBP();
		return 0;
	}
	if (is_or(inputExtension, ".cpp", ".c", ".ybp2", ".mk", ".png")) {
		if (inputExtension == ".ybp2") readYBP(argv);
		else if (inputExtension == ".mk") inputMK=true;
		else if (inputExtension == ".png") {
			DLL<int, const std::string&, const std::string&> pngToIco(dll, "pngToIco");
			pngToIco(sourceFile+".png", sourceFile+".ico");
			return 0;
		}
		else {
			if (inputExtension == ".c") compilerCPPMode = false;
			userInput();
		}
	} else {
		std::cout << "[警告] C(.c) もしくは C++(.cpp) もしくは BuildProfile2(.ybp2) もしくは MakeFile(.mk) ではありませんが、続行しますか?\n"
				  << "[Y:ソースファイルとして続行 N:終了 B:BuildProfileとして続行 M:MakeFileとして続行]: ";
		char input;
		std::cin.get() >> input;
		switch(input) {
			case 'Y': case 'y': userInput(); break;
			case 'N': case 'n': return 0;
			case 'B': case 'b': readYBP(argv); break;
			case 'M': case 'm': inputMK=true; break;
			default: return 2;
		}
	}
	
	if (!inputMK) writeMK();
	
	compile();
	
	// 終了
	end();
}