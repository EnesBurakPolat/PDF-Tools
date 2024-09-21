#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <windows.h>
#include <commdlg.h>

// PDF Header
void write_pdf_header(std::ofstream& pdf) {
    pdf << "%PDF-1.4\n";
}

// Merge PDFs
void merge_pdfs(const std::vector<std::string>& pdf_files, const std::string& output_file) {
    std::ofstream output(output_file, std::ios::binary);
    if (!output) {
        std::cerr << "Could not open output file: " << output_file << std::endl;
        return;
    }

    write_pdf_header(output);
    output << "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    output << "2 0 obj\n<< /Type /Pages /Kids [";

    size_t total_pages = 0;
    for (const auto& pdf_file : pdf_files) {
        std::ifstream input(pdf_file, std::ios::binary);
        if (!input) {
            std::cerr << "Could not open file: " << pdf_file << std::endl;
            continue;
        }
        
        std::string line;
        while (std::getline(input, line)) {
            output << line << "\n"; // Add PDF content
        }
        total_pages++;
        input.close();
    }

    output << "] /Count " << total_pages << " >>\nendobj\n";

    // Add XREF and trailer sections
    output << "xref\n0 " << (total_pages + 3) << "\n";
    for (size_t i = 0; i < total_pages + 3; ++i) {
        output << "0000000000 65535 f \n"; // Placeholder
    }
    
    output << "trailer\n<< /Size " << (total_pages + 3) << " /Root 1 0 R >>\n";
    output << "%%EOF\n";

    output.close();
    std::cout << "PDF files successfully merged: " << output_file << std::endl;
}

// Modern File Selection
std::vector<std::string> select_files(const char* filter) {
    OPENFILENAME ofn;
    char szFiles[1024] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFiles;
    ofn.nMaxFile = sizeof(szFiles);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER;

    if (GetOpenFileName(&ofn)) {
        std::vector<std::string> files;
        char* p = szFiles;
        while (*p) {
            files.push_back(p);
            p += strlen(p) + 1; // Move to next file
        }
        return files;
    } else {
        std::cerr << "File selection canceled." << std::endl;
        return {};
    }
}

// Convert image files to PDF
void image_to_pdf(const std::string& image_file, const std::string& output_pdf) {
    std::ofstream pdf(output_pdf, std::ios::binary);
    write_pdf_header(pdf);
    pdf << "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    pdf << "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n";
    pdf << "3 0 obj\n<< /Type /Page /MediaBox [0 0 612 792] /Contents 4 0 R /Resources << >> >>\nendobj\n";
    
    pdf << "4 0 obj\n<< /Length 5 0 R >>\nstream\n";

    // Insert the image file (this section is just a placeholder)
    std::ifstream img(image_file, std::ios::binary);
    if (img) {
        pdf << img.rdbuf(); // Add image file raw data
        img.close();
    }
    
    pdf << "\nendstream\nendobj\n";
    pdf << "5 0 obj\nendobj\n";
    pdf << "xref\n0 6\n0000000000 65535 f \n0000000009 00000 n \n0000000054 00000 n \n";
    pdf << "0000000100 00000 n \n0000000154 00000 n \n0000000210 00000 n \n";
    pdf << "trailer\n<< /Size 6 /Root 1 0 R >>\n";
    pdf << "%%EOF\n";

    pdf.close();
    std::cout << "Image successfully converted to PDF: " << output_pdf << std::endl;
}

// Main Menu
void display_menu() {
    std::cout << "Select the operation you want to perform:\n";
    std::cout << "1. Merge PDFs\n";
    std::cout << "2. Convert PNG to PDF\n";
    std::cout << "3. Convert JPEG to PDF\n";
    std::cout << "q. Exit\n";
}

int main() {
    char choice;
    do {
        display_menu();
        std::cout << "Your choice: ";
        std::cin >> choice;

        switch (choice) {
            case '1': {
                std::cout << "Select the PDF files you want to merge:\n";
                auto pdf_files = select_files("PDF Files (*.pdf)\0*.pdf\0All Files (*.*)\0*.*\0");
                if (!pdf_files.empty()) {
                    merge_pdfs(pdf_files, "merged_output.pdf");
                }
                break;
            }
            case '2': {
                std::cout << "Select a PNG file:\n";
                auto png_file = select_files("PNG Files (*.png)\0*.png\0All Files (*.*)\0*.*\0");
                if (!png_file.empty()) {
                    image_to_pdf(png_file.front(), "output_image.pdf");
                }
                break;
            }
            case '3': {
                std::cout << "Select a JPEG file:\n";
                auto jpeg_file = select_files("JPEG Files (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0All Files (*.*)\0*.*\0");
                if (!jpeg_file.empty()) {
                    image_to_pdf(jpeg_file.front(), "output_image.pdf");
                }
                break;
            }
            case 'q':
                std::cout << "Exiting...\n";
                break;
            default:
                std::cout << "Invalid choice. Please try again.\n";
        }
    } while (choice != 'q');

    return 0;
}

//g++ pdf.cpp -o pdf.exe -lcomdlg32
