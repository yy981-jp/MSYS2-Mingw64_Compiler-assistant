#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <filesystem>
#include <cstdlib>


namespace fs = std::filesystem;
std::string option, exename, sourceFile, mocFilePath, include_qt6, addlibrary, original;
std::string temp(std::string(std::getenv("temp")));
bool original_qt = false;
int cmd(const std::string& command) {return std::system(command.c_str());}


void userInput() {
	std::cout << "独自オプション: ";
	std::getline(std::cin,original);
	std::cout << "実行ファイル名(拡張子除く): ";
	std::getline(std::cin,exename);
	std::cout << "追加するライブラリ(各ファイル名の前に-lを追加): ";
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
	std::ofstream(sourceFile + ".ybp") << sourceFile << "\n" << exename << "\n" << addlibrary << "\n" << option << "\n" << original;
}

void writeMK() {
	std::string stdcpp23, cp932, icon;
	if (exename.empty()) exename=sourceFile;
	// 入力処理
	if (!original.contains("nocp932")) cp932 = "-fexec-charset=cp932";
	if (original.contains("qt")) {
		addlibrary = addlibrary + " -lQt6Core -lQt6Widgets -lQt6Gui";
		include_qt6 = "-Ic:/msys64/mingw64/include/qt6";
		original_qt = true;
	}
	if (!original.contains("nocpp23")) stdcpp23 = "-std=c++23";
	if (original.contains("icon")) {
		if (original.substr(4,1) != "=") std::cerr << "使い方が違います icon=<ファイルパス>";
		original.erase(0,original.find("icon")+5);
		std::string iconFileName = temp + "/Compiler_icon";
		std::ofstream(temp+".rc") << "IDI_ICON1 ICON \"" << original << "\"";
		cmd("windres.exe -i " + temp+".rc" + " -o " + temp+".o");
		icon = temp+".o";
	}
	
	std::ofstream(temp + "/_.mk")
		<< "CC  = g++"
		<< "\nCFLAGS  = " << cp932 << " " << stdcpp23 << " " << option
		<< "\nTARGET  = " << exename << ".exe"
		<< "\nSRCS    = " << sourceFile << ".cpp " << (original_qt? mocFilePath : "") << icon
		<< "\nOBJS    = " << sourceFile << ".o"
		<< "\nINCDIR  = -Ic:/msys64/mingw64/include " << include_qt6
		<< "\nLIBDIR  = -Lc:/msys64/mingw64/lib"
		<< "\nLIBS    = " << addlibrary
		<< "\n$(TARGET): $(OBJS)"
		<< "\n\t$(CC) -o $@ $^ $(LIBDIR) $(LIBS)"
		<< "\n%.o: %.cpp"
		<< "\n\t$(CC) $(CFLAGS) $(INCDIR) -c $< -o $@";
}

inline void core() {
	int returnCode = cmd("c:/GnuWin32/bin/make.exe -B -f " + temp + "/_.mk"); // 核
	if (returnCode==0) std::cout << "成功\n"; else std::cout << "エラー[" << returnCode << "]\n\n";
}


int main(int argc, char *argv[]) {
	// set paths
	sourceFile = exename = addlibrary = option = original = "null";
	sourceFile = fs::path(argv[1]).stem().string();
	mocFilePath = sourceFile + ".cpp -o " + temp + "/" + sourceFile + ".moc";

	bool inputMK = false;
	if (argc == 1) {createYBP();return 0;}
	if (argc > 2) {std::cerr << "コマンドライン引数エラー\n";return 1;}
	std::string inputExtention = fs::path(argv[1]).extension().string();
	if (inputExtention == ".cpp" || inputExtention == ".c" || inputExtention == ".ybp" || inputExtention == ".mk") {
		if (inputExtention == ".ybp") readYBP(argv);
		else if (inputExtention == ".mk") inputMK=true;
		else userInput();
	} else {
		std::cout << "[警告] C(.c) もしくは C++(.cpp) もしくは BuildProfile(.ybp) もしくは MakeFile(.mk) ではありませんが、続行しますか?\n"
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
	
	// コンパイル
	core();
	std::string input;
	while(true) {
		std::cout << "> ";
		std::getline(std::cin,input); //指示
		if (input == "help")
			std::cout << "moc: .mocファイルを生成\n"
					  << "write: .mkファイルを生成\n"
					  << "exit: 終了\n"
					  << "custom: 変数を変更\n"
					  << "        sou: ソースファイル名\n"
					  << "        exe: 実行ファイル名\n"
					  << "        opt: オプション\n"
					  << "        ori: 独自オプション\n"
					  << "        tit: ウィンドウタイトル\n"
					  << "     例: custom sou test";
		else if (input == "moc") cmd("c:/msys64/mingw64/share/qt6/bin/moc.exe " + mocFilePath);
		else if (input == "write") writeMK();
		else if (input.size() >= 6 && input.substr(0,6) == "custom") { // custom.begin
			if (input == "custom") input += " help";
			input.erase(0,7);
			char windowTitle[256];
			GetConsoleTitle(windowTitle,256);
			if (input.substr(0,4)=="help") {
				std::cout << "        sou: ソースファイル名 (現在: " << sourceFile << " )\n"
						  << "        exe: 実行ファイル名 (現在: " << exename << " )\n"
						  << "        opt: オプション (現在: " << option << " )\n"
						  << "        ori: 独自オプション (現在: " << original << " )\n"
						  << "        tit: ウィンドウタイトル (現在: " << windowTitle << " )\n"
						  << "     例: custom sou test";
			}
			else if (input.substr(3,1)==" ") {
				if (input.substr(0,3)=="sou") sourceFile = input.substr(4);
				else if (input.substr(0,3)=="exe") exename = input.substr(4);
				else if (input.substr(0,3)=="opt") sourceFile = input.substr(4);
				else if (input.substr(0,3)=="ori") original = input.substr(4);
				else if (input.substr(0,3)=="tit") SetConsoleTitle(input.substr(4).c_str());
				else std::cerr << "入力エラー";
				writeMK();
			} else std::cerr << "入力エラー";
		} // custom.end
		else if (input == "exit") break;
		else if (input == "") core();
		else std::cerr << "有効なコマンドではありません help と打ってコマンドを確認してください";
		std::cout << "\n";
	}
	
	// 終了
	if (fs::exists(sourceFile+".o")) fs::remove(sourceFile+".o");
}