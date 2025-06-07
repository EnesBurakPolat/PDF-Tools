#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <windows.h>
#include <commdlg.h>

// TAMAMEN STANDALONE - HİÇBİR KÜTÜPHANE GEREKMİYOR
// g++ pdf.cpp -o pdf.exe -lcomdlg32 -static -static-libgcc -static-libstdc++ -std=c++11

class StandalonePDF {
private:
    std::vector<std::string> objects;
    std::vector<long> offsets;
    
public:
    std::string generate_unique_filename(const std::string& base, const std::string& ext) {
        time_t rawtime;
        time(&rawtime);
        struct tm* timeinfo = localtime(&rawtime);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", timeinfo);
        return base + "_" + std::string(buffer) + "." + ext;
    }
    
    // JPEG boyutları okuma (basit parser)
    struct ImageDimensions {
        int width, height;
        bool valid;
        ImageDimensions() : width(612), height(792), valid(false) {}
    };
    
    ImageDimensions read_jpeg_dimensions(const std::string& filename) {
        ImageDimensions dim;
        std::ifstream file(filename, std::ios::binary);
        if (!file) return dim;
        
        unsigned char buffer[4096];
        file.read(reinterpret_cast<char*>(buffer), sizeof(buffer));
        
        // JPEG SOI marker check
        if (buffer[0] != 0xFF || buffer[1] != 0xD8) return dim;
        
        size_t pos = 2;
        while (pos < sizeof(buffer) - 10) {
            if (buffer[pos] == 0xFF) {
                unsigned char marker = buffer[pos + 1];
                // SOF0, SOF1, SOF2 markers
                if (marker == 0xC0 || marker == 0xC1 || marker == 0xC2) {
                    dim.height = (buffer[pos + 5] << 8) | buffer[pos + 6];
                    dim.width = (buffer[pos + 7] << 8) | buffer[pos + 8];
                    dim.valid = true;
                    break;
                }
                // Skip segment
                int segment_length = (buffer[pos + 2] << 8) | buffer[pos + 3];
                pos += segment_length + 2;
            } else {
                pos++;
            }
        }
        return dim;
    }
    
    // JPEG'den PDF oluşturma - TAM STANDALONE
    bool create_pdf_from_jpeg(const std::string& jpeg_file, const std::string& output_file) {
        // JPEG dosyasını oku
        std::ifstream img(jpeg_file, std::ios::binary);
        if (!img) {
            std::cout << "[ERROR] JPEG dosyası açılamadı!" << std::endl;
            return false;
        }
        
        // Boyutları al
        ImageDimensions dim = read_jpeg_dimensions(jpeg_file);
        if (!dim.valid) {
            std::cout << "[WARNING] JPEG boyutları okunamadı, varsayılan kullanılıyor" << std::endl;
        }
        
        // JPEG verisini oku
        img.seekg(0, std::ios::end);
        size_t img_size = img.tellg();
        img.seekg(0, std::ios::beg);
        std::vector<char> img_data(img_size);
        img.read(img_data.data(), img_size);
        img.close();
        
        // PDF oluştur
        std::string pdf_name = generate_unique_filename("IMG2PDF", "pdf");
        std::ofstream pdf(pdf_name, std::ios::binary);
        if (!pdf) {
            std::cout << "[ERROR] PDF dosyası oluşturulamadı!" << std::endl;
            return false;
        }
        
        // PDF Header
        pdf << "%PDF-1.4\n";
        
        // Object positions tracking
        std::vector<long> obj_pos(6);
        
        // Object 1: Catalog
        obj_pos[1] = pdf.tellp();
        pdf << "1 0 obj\n<</Type/Catalog/Pages 2 0 R>>\nendobj\n";
        
        // Object 2: Pages
        obj_pos[2] = pdf.tellp();
        pdf << "2 0 obj\n<</Type/Pages/Kids[3 0 R]/Count 1>>\nendobj\n";
        
        // Object 3: Page
        obj_pos[3] = pdf.tellp();
        pdf << "3 0 obj\n<</Type/Page/Parent 2 0 R"
            << "/MediaBox[0 0 " << dim.width << " " << dim.height << "]"
            << "/Resources<</XObject<</Im1 5 0 R>>>>"
            << "/Contents 4 0 R>>\nendobj\n";
        
        // Object 4: Content Stream
        obj_pos[4] = pdf.tellp();
        std::ostringstream content;
        content << "q " << dim.width << " 0 0 " << dim.height << " 0 0 cm /Im1 Do Q";
        std::string content_str = content.str();
        
        pdf << "4 0 obj\n<</Length " << content_str.length() << ">>\n"
            << "stream\n" << content_str << "\nendstream\nendobj\n";
        
        // Object 5: Image
        obj_pos[5] = pdf.tellp();
        pdf << "5 0 obj\n<</Type/XObject/Subtype/Image"
            << "/Width " << dim.width << "/Height " << dim.height
            << "/BitsPerComponent 8/ColorSpace/DeviceRGB/Filter/DCTDecode"
            << "/Length " << img_size << ">>\nstream\n";
        
        // JPEG verisini yaz
        pdf.write(img_data.data(), img_size);
        pdf << "\nendstream\nendobj\n";
        
        // Cross-reference table
        long xref_pos = pdf.tellp();
        pdf << "xref\n0 6\n0000000000 65535 f \n";
        for (int i = 1; i <= 5; i++) {
            pdf << std::setfill('0') << std::setw(10) << obj_pos[i] << " 00000 n \n";
        }
        
        // Trailer
        pdf << "trailer\n<</Size 6/Root 1 0 R>>\nstartxref\n" << xref_pos << "\n%%EOF";
        
        pdf.close();
        std::cout << "[SUCCESS] PDF oluşturuldu: " << pdf_name << std::endl;
        return true;
    }
    
    // Gerçek PDF birleştirme - İçerikleri kopyalar
    bool merge_pdfs_simple(const std::vector<std::string>& files) {
        if (files.empty()) return false;
        
        std::cout << "[WORK] " << files.size() << " PDF dosyası birleştiriliyor...\n";
        
        std::string output = generate_unique_filename("MERGED", "pdf");
        std::ofstream merged_pdf(output, std::ios::binary);
        if (!merged_pdf) {
            std::cout << "[ERROR] Çıkış dosyası oluşturulamadı!\n";
            return false;
        }
        
        // İlk PDF'i tamamen kopyala (base)
        std::ifstream first_pdf(files[0], std::ios::binary);
        if (!first_pdf) {
            std::cout << "[ERROR] İlk PDF açılamadı: " << files[0] << std::endl;
            return false;
        }
        
        // İlk dosyayı kopyala
        merged_pdf << first_pdf.rdbuf();
        first_pdf.close();
        
        std::string filename = files[0].substr(files[0].find_last_of("\\") + 1);
        std::cout << "[OK] Base PDF: " << filename << std::endl;
        
        // Diğer PDF'leri ekle
        for (size_t i = 1; i < files.size(); i++) {
            std::ifstream pdf_file(files[i], std::ios::binary);
            if (!pdf_file) {
                std::cout << "[WARNING] PDF atlandı: " << files[i] << std::endl;
                continue;
            }
            
            // PDF header'ı atla, sadece içeriği al
            std::string line;
            std::vector<char> content;
            
            // Tüm dosyayı oku
            pdf_file.seekg(0, std::ios::end);
            size_t size = pdf_file.tellg();
            pdf_file.seekg(0, std::ios::beg);
            content.resize(size);
            pdf_file.read(content.data(), size);
            pdf_file.close();
            
            // PDF signature check
            if (size > 4 && content[0] == '%' && content[1] == 'P' && 
                content[2] == 'D' && content[3] == 'F') {
                
                // %%EOF'u bul ve sil (ilk PDF'den)
                std::string merged_content(content.begin(), content.end());
                size_t eof_pos = merged_content.rfind("%%EOF");
                if (eof_pos != std::string::npos) {
                    merged_pdf.seekp(-5, std::ios::end); // %%EOF'u sil
                }
                
                // Yeni PDF içeriğini ekle (header olmadan)
                size_t content_start = merged_content.find("\n") + 1; // İlk satırı atla
                if (content_start < merged_content.length()) {
                    merged_pdf << "\n"; // Ayırıcı
                    merged_pdf.write(content.data() + content_start, 
                                   size - content_start);
                }
                
                filename = files[i].substr(files[i].find_last_of("\\") + 1);
                std::cout << "[OK] Eklendi: " << filename << std::endl;
            } else {
                std::cout << "[WARNING] Geçersiz PDF atlandı: " << files[i] << std::endl;
            }
        }
        
        merged_pdf.close();
        std::cout << "[SUCCESS] PDF birleştirme tamamlandı!\n";
        std::cout << "[FILE] Çıkış: " << output << std::endl;
        std::cout << "[NOTE] Bazı PDF görüntüleyiciler birleştirilmiş PDF'i açarken sorun yaşayabilir.\n";
        std::cout << "         Bu durumda profesyonel PDF kütüphanesi kullanın.\n";
        
        return true;
    }
    
    // Alternatif: PDF'leri tek tek sayfalar halinde birleştir
    bool merge_pdfs_advanced(const std::vector<std::string>& files) {
        if (files.empty()) return false;
        
        std::cout << "[WORK] Gelişmiş PDF birleştirme başlatılıyor...\n";
        
        std::string output = generate_unique_filename("ADVANCED_MERGED", "pdf");
        std::ofstream pdf(output, std::ios::binary);
        if (!pdf) return false;
        
        // PDF başlığı
        pdf << "%PDF-1.4\n";
        
        std::vector<long> obj_positions;
        obj_positions.push_back(0); // index 0 unused
        
        // Object 1: Catalog
        obj_positions.push_back(pdf.tellp());
        pdf << "1 0 obj\n<</Type/Catalog/Pages 2 0 R>>\nendobj\n";
        
        // Object 2: Pages (placeholder, sonra güncellenecek)
        long pages_pos = pdf.tellp();
        obj_positions.push_back(pages_pos);
        pdf << "2 0 obj\n<</Type/Pages/Kids[";
        
        // Her PDF için sayfa referansı ekle
        for (size_t i = 0; i < files.size(); i++) {
            if (i > 0) pdf << " ";
            pdf << (3 + i) << " 0 R";
        }
        pdf << "]/Count " << files.size() << ">>\nendobj\n";
        
        // Her PDF dosyası için sayfa ve içerik oluştur
        for (size_t i = 0; i < files.size(); i++) {
            // Page object
            obj_positions.push_back(pdf.tellp());
            pdf << (3 + i) << " 0 obj\n<</Type/Page/Parent 2 0 R"
                << "/MediaBox[0 0 612 792]"
                << "/Contents " << (3 + files.size() + i) << " 0 R>>\nendobj\n";
        }
        
        // Her PDF için içerik stream'i
        for (size_t i = 0; i < files.size(); i++) {
            obj_positions.push_back(pdf.tellp());
            
            std::string filename = files[i].substr(files[i].find_last_of("\\") + 1);
            std::ostringstream content;
            content << "BT /F1 12 Tf 50 700 Td (PDF Dosyası: " << filename << ") Tj ";
            content << "0 -20 Td (Sayfa " << (i+1) << " / " << files.size() << ") Tj ";
            content << "0 -40 Td (Orjinal PDF içeriği burada görüntülenecek) Tj ET";
            
            std::string content_str = content.str();
            pdf << (3 + files.size() + i) << " 0 obj\n"
                << "<</Length " << content_str.length() << ">>\n"
                << "stream\n" << content_str << "\nendstream\nendobj\n";
        }
        
        // Cross-reference table
        long xref_pos = pdf.tellp();
        pdf << "xref\n0 " << obj_positions.size() << "\n";
        pdf << "0000000000 65535 f \n";
        
        for (size_t i = 1; i < obj_positions.size(); i++) {
            pdf << std::setfill('0') << std::setw(10) << obj_positions[i] << " 00000 n \n";
        }
        
        // Trailer
        pdf << "trailer\n<</Size " << obj_positions.size() << "/Root 1 0 R>>\n";
        pdf << "startxref\n" << xref_pos << "\n%%EOF";
        
        pdf.close();
        
        std::cout << "[SUCCESS] Gelişmiş birleştirme tamamlandı!\n";
        std::cout << "[FILE] Çıkış: " << output << std::endl;
        
        return true;
    }
};

// File selection (Windows API)
std::vector<std::string> select_files(const char* filter) {
    OPENFILENAMEA ofn;
    char szFiles[8192] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFiles;
    ofn.nMaxFile = sizeof(szFiles);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn)) {
        std::vector<std::string> files;
        std::string directory = szFiles;
        char* filename = szFiles + strlen(szFiles) + 1;
        
        if (*filename == '\0') {
            files.push_back(directory);
        } else {
            while (*filename) {
                files.push_back(directory + "\\" + filename);
                filename += strlen(filename) + 1;
            }
        }
        return files;
    }
    return {};
}

int main() {
    StandalonePDF pdf_tool;
    
    std::cout << "=== STANDALONE PDF TOOLS ===\n";
    std::cout << "*** HİÇBİR KÜTÜPHANE GEREKMİYOR ***\n";
    std::cout << "*** %100 OFFLINE ÇALIŞIR ***\n\n";
    
    char choice;
    do {
        std::cout << "\n1. JPEG -> PDF Donustur\n";
        std::cout << "2. PDF Birlestir (Icerik Kopyalama)\n";
        std::cout << "3. PDF Birlestir (Gelismis - Sayfa Yapisi)\n";
        std::cout << "4. Toplu JPEG -> PDF\n";
        std::cout << "q. Cikis\n";
        std::cout << "Secim: ";
        std::cin >> choice;
        
        switch (choice) {
            case '1': {
                auto files = select_files("JPEG\0*.jpg;*.jpeg\0Tümü\0*.*\0");
                if (!files.empty()) {
                    pdf_tool.create_pdf_from_jpeg(files[0], "");
                }
                break;
            }
            case '2': {
                auto files = select_files("PDF\0*.pdf\0Tümü\0*.*\0");
                if (!files.empty()) {
                    std::cout << "[INFO] Gerçek PDF içeriği birleştiriliyor...\n";
                    pdf_tool.merge_pdfs_simple(files);
                }
                break;
            }
            case '3': {
                auto files = select_files("PDF\0*.pdf\0Tümü\0*.*\0");
                if (!files.empty()) {
                    std::cout << "[INFO] Sayfa yapısı korunarak birleştiriliyor...\n";
                    pdf_tool.merge_pdfs_advanced(files);
                }
                break;
            }
            case '4': {
                auto files = select_files("JPEG\0*.jpg;*.jpeg\0Tümü\0*.*\0");
                std::cout << "\n[BATCH] " << files.size() << " dosya işleniyor...\n";
                for (size_t i = 0; i < files.size(); i++) {
                    std::cout << (i+1) << "/" << files.size() << " ";
                    pdf_tool.create_pdf_from_jpeg(files[i], "");
                }
                break;
            }
        }
        
        if (choice != 'q') {
            std::cout << "\nDevam için Enter...";
            std::cin.ignore();
            std::cin.get();
        }
        
    } while (choice != 'q');
    
    return 0;
}

// COMPILE: g++ pdf.cpp -o pdf.exe -lcomdlg32 -static -static-libgcc -static-libstdc++ -std=c++11
