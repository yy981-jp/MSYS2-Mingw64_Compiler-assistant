#include <iostream>
#include <string>
#include <fstream>
#include <cstdint>
#include <cstdio>
#include <windows.h>
#include <tlhelp32.h>
#include <zlib.h>
#define BUILD
#include <yy981/tools/dll.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>


extern "C" {

// ICO変換処理を関数化
DLL int pngToIco(const std::string& in_s, const std::string& ou_s) {
	const char* inputFile = in_s.c_str();
	const char* outputFile = ou_s.c_str();
    // 画像を読み込む
    int width, height, channels;
    unsigned char* data = stbi_load(inputFile, &width, &height, &channels, 4); // 4 = RGBA
    if (!data) {
        std::cerr << "Failed to load image: " << inputFile << std::endl;
        return 1;
    }

    // ICOヘッダを書き込む
    FILE* output = fopen(outputFile, "wb");
    if (!output) {
        std::cerr << "Failed to open output file: " << outputFile << std::endl;
        stbi_image_free(data);
        return 1;
    }

    // ICOファイルのヘッダ (1枚の画像のみ)
    unsigned char icoHeader[6] = {0, 0, 1, 0, 1, 0};
    fwrite(icoHeader, sizeof(icoHeader), 1, output);

    // 画像エントリ情報
    unsigned char entry[16] = {};
    entry[0] = static_cast<unsigned char>(width);  // 幅
    entry[1] = static_cast<unsigned char>(height); // 高さ
    entry[2] = 0; // 色数 (0=256色以上)
    entry[3] = 0; // 予約 (0)
    entry[4] = 1; // 色深度 (1 = RGBA)
    entry[5] = 32; // ビット深度 (32bit)
    uint32_t imageSize = width * height * 4; // RGBAのデータサイズ
    entry[8] = imageSize & 0xFF;
    entry[9] = (imageSize >> 8) & 0xFF;
    entry[10] = (imageSize >> 16) & 0xFF;
    entry[11] = (imageSize >> 24) & 0xFF;
    uint32_t offset = sizeof(icoHeader) + sizeof(entry);
    entry[12] = offset & 0xFF;
    entry[13] = (offset >> 8) & 0xFF;
    entry[14] = (offset >> 16) & 0xFF;
    entry[15] = (offset >> 24) & 0xFF;
    fwrite(entry, sizeof(entry), 1, output);

    // PNGデータをICOに埋め込む (簡略化)
    fwrite(data, imageSize, 1, output);

    // 終了処理
    fclose(output);
    stbi_image_free(data);

    return 0;
}

// ファイルハッシュ計算
DLL uint32_t calculateCRC32(const std::string &filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
        return 0;
    }

    uint32_t crc = crc32(0L, Z_NULL, 0); // 初期化
    char buffer[4096];

    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        crc = crc32(crc, reinterpret_cast<const Bytef *>(buffer), file.gcount());
    }

    return crc;
}

// プロセス名からプロセスIDを取得
DWORD GetProcessIDByName(const std::string& processName) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(snapshot, &entry)) {
        do {
            if (processName == entry.szExeFile) {
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return 0;
}

// プロセス優先度を変更
DLL bool SetProcessPriority(const std::string& processName, DWORD priorityClass) {
    DWORD processID = GetProcessIDByName(processName);
    if (processID == 0) {
        std::cout << "プロセスが見つかりません" << std::endl;
        return false;
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (hProcess == NULL) {
        std::cout << "プロセスを開けません" << std::endl;
        return false;
    }

    if (!SetPriorityClass(hProcess, priorityClass)) {
        std::cout << "優先度を変更できません" << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    std::cout << "優先度を正常に変更しました" << std::endl;
    CloseHandle(hProcess);
    return true;
}

}